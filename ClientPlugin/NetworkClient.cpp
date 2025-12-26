#include "NetworkClient.h"

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
    head = 0;
    tail = 0;
    totalBytesReceived = 0;
    totalBytesSent = 0;

    {
        std::lock_guard<std::mutex> lock(despawnMutex);
        despawnQueue.clear();
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
                else if (type == Purpose::PACKET_WORLD_STATE) {
                    auto* p = reinterpret_cast<Purpose::WorldStatePacket*>(event.packet->data);

                    if (p->entityCount > 0) {
                        uint32_t currentTick = p->entities[0].lastProcessedTick;

                        if (lastReceivedTick != 0 && currentTick > lastReceivedTick) {
                            uint32_t gap = currentTick - lastReceivedTick;
                            packetsExpected += gap;
                        } else if (lastReceivedTick == 0) {
                            packetsExpected += 1;
                        }

                        packetsReceived += 1;
                        lastReceivedTick = currentTick;
                    }

                    for (uint32_t i = 0; i < p->entityCount; ++i) {
                        int currentHead = head.load();
                        entityBuffer[currentHead] = p->entities[i];

                        int nextHead = (currentHead + 1) % ENTITY_BUFFER_SIZE;
                        head.store(nextHead);

                        if (nextHead == tail.load()) {
                            tail.store((tail.load() + 1) % ENTITY_BUFFER_SIZE);
                        }
                    }
                }
                else if (type == Purpose::PACKET_ENTITY_DESPAWN) {
                    auto* p = reinterpret_cast<Purpose::EntityDespawn*>(event.packet->data);
                    std::lock_guard<std::mutex> lock(despawnMutex);
                    despawnQueue.push_back(p->networkID);
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

bool NetworkClient::PopEntityData(Purpose::EntityData& outData) {
    int currentTail = tail.load();
    if (currentTail == head.load()) return false;

    outData = entityBuffer[currentTail];
    tail.store((currentTail + 1) % ENTITY_BUFFER_SIZE);
    return true;
}

uint32_t NetworkClient::PopDespawnID() {
    std::lock_guard<std::mutex> lock(despawnMutex);
    if (despawnQueue.empty()) return 0;

    uint32_t id = despawnQueue.front();
    despawnQueue.pop_front();
    return id;
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