#include "NetworkClient.h"
#include <memory>
#include <iostream>
#include <algorithm>

#include "BitStream.h"

NetworkClient::NetworkClient() {
    enet_initialize();
}

NetworkClient::~NetworkClient() {
    Disconnect();
    enet_deinitialize();
}

void NetworkClient::Log(const char* msg) const {
    if (logger) logger(msg);
}

bool NetworkClient::Connect(const char* ip, uint16_t port) {
    if (clientHost) Disconnect();

    assignedPlayerID = 0;
    totalBytesReceived = 0;
    totalBytesSent = 0;

    uint32_t tempID;
    while (despawnQueue.Pop(tempID));

    if (packetMemory.empty()) {
        packetMemory.resize(MAX_PACKET_POOL_SIZE);
    }

    GamePacket* temp = nullptr;
    while (packetQueue.Pop(temp));
    while (freePacketPool.Pop(temp));

    for (int i = 0; i < MAX_PACKET_POOL_SIZE; i++) {
        freePacketPool.Push(&packetMemory[i]);
    }

    clientHost = enet_host_create(nullptr, 1, Purpose::CHANNEL_COUNT, 0, 0);
    if (!clientHost) {
        Log("[Client] Failed to create ENet host.");
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, ip);
    address.port = port;

    serverPeer = enet_host_connect(clientHost, &address, Purpose::CHANNEL_COUNT, 0);
    if (!serverPeer) return false;

    ENetEvent event;
    if (enet_host_service(clientHost, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        Log("[Client] Connected to server.");
        lastMetricTime = std::chrono::steady_clock::now();
        return true;
    }

    Log("[Client] Connection timed out.");
    enet_peer_reset(serverPeer);
    return false;
}

void NetworkClient::Disconnect() {
    if (serverPeer) {
        enet_peer_disconnect(serverPeer, 0);
        enet_host_flush(clientHost);
        serverPeer = nullptr;
    }
    if (clientHost) {
        enet_host_destroy(clientHost);
        clientHost = nullptr;
    }
}

void NetworkClient::ServiceNetwork() {
    if (!clientHost) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMetricTime).count();

    if (elapsed >= 1000) {
        currentInKBps = (bytesReceivedThisSecond.load() / 1024.0f);
        currentOutKBps = (bytesSentThisSecond.load() / 1024.0f);

        if (packetsExpected > 0) {
            float lossFraction = 1.0f - ((float)packetsReceived / (float)packetsExpected);
            manualPacketLoss = (uint32_t)(std::max(0.0f, lossFraction) * 100.0f);
        }

        bytesReceivedThisSecond = 0;
        bytesSentThisSecond = 0;
        packetsExpected = 0;
        packetsReceived = 0;
        lastMetricTime = now;
    }

    ENetEvent event;
    while (enet_host_service(clientHost, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength < sizeof(uint16_t)) {
                    enet_packet_destroy(event.packet);
                    break;
                }

                bytesReceivedThisSecond += event.packet->dataLength;
                totalBytesReceived += event.packet->dataLength;

                uint16_t type = *reinterpret_cast<uint16_t*>(event.packet->data);

                if (type == Purpose::PACKET_WELCOME) {
                    auto* p = reinterpret_cast<Purpose::WelcomePacket*>(event.packet->data);
                    assignedPlayerID = p->playerID;
                    Log("[Client] Assigned ID received.");
                }
                else if (type == Purpose::PACKET_WORLD_STATE || type == Purpose::PACKET_DEBUG_HIT) {
                    if (type == Purpose::PACKET_WORLD_STATE) packetsReceived++;

                    GamePacket* pkt = nullptr;

                    if (freePacketPool.Pop(pkt)) {
                        size_t copyLen = event.packet->dataLength;
                        if (copyLen > Purpose::MTU_SIZE) copyLen = Purpose::MTU_SIZE;

                        memcpy(pkt->data, event.packet->data, copyLen);
                        pkt->length = copyLen;

                        if (!packetQueue.Push(pkt)) {
                            freePacketPool.Push(pkt);
                            Log("[Client] Queue Full! Dropping packet.");
                        }
                    } else {
                        Log("[Client] Packet Pool Exhausted! Dropping packet.");
                    }


                    if (type == Purpose::PACKET_WORLD_STATE && event.packet->dataLength >= 6) {
                        uint8_t* ptr = event.packet->data + 2;
                        uint32_t receivedTick = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

                        if (lastReceivedTick != 0 && receivedTick > lastReceivedTick) {
                            uint32_t gap = receivedTick - lastReceivedTick - 1;
                            packetsExpected += gap;
                        }

                        packetsExpected++;
                        lastReceivedTick = receivedTick;

                        Purpose::ClientAck ack;
                        ack.tick = receivedTick;
                        ENetPacket* ackPacket = enet_packet_create(&ack, sizeof(ack), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                        enet_peer_send(serverPeer, Purpose::CHANNEL_UNRELIABLE, ackPacket);
                    }
                }
                else if (type == Purpose::PACKET_ENTITY_DESPAWN) {
                    auto* p = reinterpret_cast<Purpose::EntityDespawn*>(event.packet->data);
                    if (!despawnQueue.Push(p->networkID)) {
                        Log("[Client] Despawn Queue Full! Event dropped.");
                    }
                }

                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                Log("[Client] Disconnected from server.");
                serverPeer = nullptr;
                assignedPlayerID = 0;
                lastReceivedTick = 0;
                break;

            default:
                break;
        }
    }
}

void NetworkClient::SendInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw) {
    if (!serverPeer) return;

    Purpose::ClientInput input;
    input.tick = tick;
    input.w = w ? 1 : 0;
    input.a = a ? 1 : 0;
    input.s = s ? 1 : 0;
    input.d = d ? 1 : 0;
    input.fire = fire ? 1 : 0;
    input.mouseYaw = yaw;

    bytesSentThisSecond += sizeof(input);
    totalBytesSent += sizeof(input);

    ENetPacket* packet = enet_packet_create(&input, sizeof(input), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(serverPeer, Purpose::CHANNEL_UNRELIABLE, packet);
}

void NetworkClient::SendBecomeSpectatorRequest() {
    if (!serverPeer) return;

    uint8_t buffer[16];
    BitWriter writer(buffer, 16);

    writer.WriteBits(Purpose::PACKET_CLIENT_SPECTATOR & 0xFF, 8);
    writer.WriteBits((Purpose::PACKET_CLIENT_SPECTATOR >> 8) & 0xFF, 8);

    ENetPacket* packet = enet_packet_create(
        writer.GetData(),
        writer.GetByteLength(),
        ENET_PACKET_FLAG_RELIABLE
    );

    enet_peer_send(serverPeer, Purpose::CHANNEL_RELIABLE, packet);
}

void NetworkClient::SendSpectateTarget(uint32_t targetID) {
    if (!serverPeer) return;

    Purpose::ClientSpectateTarget req;
    req.targetID = targetID;

    ENetPacket* packet = enet_packet_create(&req, sizeof(req), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(serverPeer, Purpose::CHANNEL_RELIABLE, packet);
}

int NetworkClient::CopyLatestBitstream(uint8_t* outBuffer, int maxLen) {
    GamePacket* pkt = nullptr;

    if (packetQueue.Pop(pkt)) {

        int len = static_cast<int>(pkt->length);
        if (len > maxLen) len = maxLen;
        memcpy(outBuffer, pkt->data, len);

        freePacketPool.Push(pkt);

        return len;
    }

    return 0;
}

uint32_t NetworkClient::PopDespawnID() {
    uint32_t id = 0;
    if (despawnQueue.Pop(id)) {
        return id;
    }
    return 0;
}

Purpose::NetworkMetrics NetworkClient::GetMetrics() const {
    Purpose::NetworkMetrics m = {};
    if (!serverPeer) return m;

    m.ping = serverPeer->roundTripTime;
    m.packetLoss = manualPacketLoss.load();
    m.totalBytesSent = totalBytesSent.load();
    m.totalBytesReceived = totalBytesReceived.load();
    m.incomingBandwidth = currentInKBps.load();
    m.outgoingBandwidth = currentOutKBps.load();

    return m;
}