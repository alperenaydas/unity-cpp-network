#include <vector>
#include <enet/enet.h>
#include "EntityManager.h"
#include "Protocol.h"
#include "RingBuffer.h"

#define EXPORT_API __declspec(dllexport)

static ENetHost *g_client = nullptr;
static ENetPeer *g_peer = nullptr;
static uint32_t g_assignedPlayerID = 0;

static EntityBuffer g_entityBuffer;
static EntityManager g_entityManager;
static std::vector<uint32_t> g_despawnQueue;

typedef void (*LogCallback)(const char *message);

static LogCallback g_unityLogger = nullptr;

void PurposeLog(const char *message) {
    if (g_unityLogger) g_unityLogger(message);
}

extern "C" {
EXPORT_API void RegisterLogCallback(LogCallback callback) {
    g_unityLogger = callback;
}

EXPORT_API uint32_t GetAssignedPlayerID() {
    return g_assignedPlayerID;
}

EXPORT_API bool GetNextEntityUpdate(Purpose::EntityState *outState) {
    return g_entityBuffer.Pop(*outState);
}

EXPORT_API bool ConnectToServer() {
    g_assignedPlayerID = 0;
    g_entityBuffer.Clear();
    g_entityManager.Clear();

    PurposeLog("[Plugin] Connecting to server (State Reset)...");

    if (enet_initialize() != 0) return false;

    g_client = enet_host_create(nullptr, 1, Purpose::CHANNEL_COUNT, 0, 0);
    if (!g_client) return false;

    ENetAddress address;
    enet_address_set_host(&address, Purpose::SERVER_IP);
    address.port = Purpose::SERVER_PORT;

    g_peer = enet_host_connect(g_client, &address, Purpose::CHANNEL_COUNT, 0);
    if (!g_peer) return false;

    ENetEvent event;
    if (enet_host_service(g_client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        PurposeLog("[Plugin] Connection Established.");
        return true;
    }

    enet_peer_reset(g_peer);
    return false;
}

EXPORT_API void ServiceNetwork() {
    if (!g_client) return;

    ENetEvent event;
    while (enet_host_service(g_client, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            uint16_t packetType = *reinterpret_cast<uint16_t *>(event.packet->data);

            if (packetType == Purpose::PACKET_WELCOME) {
                auto *msg = reinterpret_cast<Purpose::WelcomePacket *>(event.packet->data);
                g_assignedPlayerID = msg->playerID;
            } else if (packetType == Purpose::PACKET_ENTITY_UPDATE) {
                auto *msg = reinterpret_cast<Purpose::EntityState *>(event.packet->data);

                g_entityBuffer.Push(*msg);
                g_entityManager.UpdateEntity(*msg);
            } else if (packetType == Purpose::PACKET_ENTITY_DESPAWN) {
                auto *msg = reinterpret_cast<Purpose::EntityDespawn *>(event.packet->data);
                g_despawnQueue.push_back(msg->networkID);
                g_entityManager.RemoveEntity(msg->networkID);
            }
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            PurposeLog("[Plugin] Disconnected.");
            g_peer = nullptr;
        }
    }
}

EXPORT_API void DisconnectFromServer() {
    if (g_peer) {
        enet_peer_disconnect(g_peer, 0);
        enet_host_flush(g_client);
    }
    if (g_client) {
        enet_host_destroy(g_client);
        g_client = nullptr;
    }
    enet_deinitialize();
}

EXPORT_API int GetInternalEntityCount() {
    return g_entityManager.GetCount();
}

EXPORT_API uint32_t GetNextDespawnID() {
    if (g_despawnQueue.empty()) return 0;
    uint32_t id = g_despawnQueue.back();
    g_despawnQueue.pop_back();
    return id;
}

EXPORT_API void SendMovementInput(int tick, bool w, bool a, bool s, bool d) {
    if (!g_peer) return;

    Purpose::ClientInput input;
    input.tick = tick;
    input.w = w;
    input.a = a;
    input.s = s;
    input.d = d;
    ENetPacket *packet = enet_packet_create(&input, sizeof(input), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(g_peer, Purpose::CHANNEL_UNRELIABLE, packet);
    enet_host_flush(g_client);
}
}
