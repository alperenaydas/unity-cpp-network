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
    
    {
        std::lock_guard lock(bufferMutex);
        head = tail = 0;
    }
    {
        std::lock_guard lock(despawnMutex);
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

    ENetEvent event;
    while (enet_host_service(clientHost, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength < sizeof(uint16_t)) break;

                uint16_t type = *reinterpret_cast<uint16_t*>(event.packet->data);

                if (type == Purpose::PACKET_WELCOME) {
                    auto* p = reinterpret_cast<Purpose::WelcomePacket*>(event.packet->data);
                    assignedPlayerID = p->playerID;
                    Log("[Client] Assigned ID received.");
                }
                else if (type == Purpose::PACKET_ENTITY_UPDATE) {
                    auto* p = reinterpret_cast<Purpose::EntityState*>(event.packet->data);
                    
                    std::lock_guard lock(bufferMutex);
                    entityBuffer[head] = *p;
                    head = (head + 1) % BUFFER_SIZE;
                    if (head == tail) tail = (tail + 1) % BUFFER_SIZE;
                }
                else if (type == Purpose::PACKET_ENTITY_DESPAWN) {
                    auto* p = reinterpret_cast<Purpose::EntityDespawn*>(event.packet->data);
                    
                    std::lock_guard lock(despawnMutex);
                    despawnQueue.push_back(p->networkID);
                }

                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                Log("[Client] Disconnected from server.");
                serverPeer = nullptr;
                break;
            case ENET_EVENT_TYPE_CONNECT:
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

void NetworkClient::SendInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw) const {
    if (!serverPeer) return;

    Purpose::ClientInput input;
    input.tick = tick;
    input.w = w; input.a = a; input.s = s; input.d = d;
    input.fire = fire;
    input.mouseYaw = yaw;

    ENetPacket* packet = enet_packet_create(&input, sizeof(input), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(serverPeer, Purpose::CHANNEL_UNRELIABLE, packet);
}

bool NetworkClient::PopEntityUpdate(Purpose::EntityState& outState) {
    std::lock_guard lock(bufferMutex);
    if (head == tail) return false;

    outState = entityBuffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return true;
}

uint32_t NetworkClient::PopDespawnID() {
    std::lock_guard lock(despawnMutex);
    if (despawnQueue.empty()) return 0;

    uint32_t id = despawnQueue.front(); // FIFO
    despawnQueue.pop_front();
    return id;
}