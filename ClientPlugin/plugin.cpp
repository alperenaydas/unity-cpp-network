#include <enet/enet.h>
#include "Protocol.h"
#include <cstdio>

#define EXPORT_API __declspec(dllexport)

static ENetHost* client = nullptr;
static ENetPeer* peer = nullptr;
static uint32_t assignedPlayerID = 0;

typedef void (*LogCallback)(const char* message);
static LogCallback unityLogger = nullptr;

void PurposeLog(const char* message) {
    if (unityLogger) unityLogger(message);
}

extern "C" {
    EXPORT_API void RegisterLogCallback(LogCallback callback) {
        unityLogger = callback;
    }

    EXPORT_API uint32_t GetAssignedPlayerID() {
        return assignedPlayerID;
    }

    EXPORT_API bool ConnectToServer() {
        PurposeLog("[Plugin] Connecting to server...");

        if (enet_initialize() != 0) return false;

        client = enet_host_create(nullptr, 1, Purpose::CHANNEL_COUNT, 0, 0);
        if (!client) return false;

        ENetAddress address;
        enet_address_set_host(&address, Purpose::SERVER_IP);
        address.port = Purpose::SERVER_PORT;

        peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);
        if (!peer) return false;

        ENetEvent event;
        if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
            return true;
        }

        enet_peer_reset(peer);
        return false;
    }

    EXPORT_API void ServiceNetwork() {
        if (!client) return;

        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    auto* msg = reinterpret_cast<Purpose::WelcomePacket*>(event.packet->data);
                    if (msg->type == Purpose::PACKET_WELCOME) {
                        assignedPlayerID = msg->playerID;

                        char buf[64];
                        sprintf(buf, "[Plugin] Welcome received! ID: %u", assignedPlayerID);
                        PurposeLog(buf);
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                    PurposeLog("[Plugin] Disconnected from server.");
                    peer = nullptr;
                    break;
                default: break;
            }
        }
    }

    EXPORT_API void DisconnectFromServer() {
        if (peer) {
            enet_peer_disconnect(peer, 0);
            enet_host_flush(client);
        }
        if (client) {
            enet_host_destroy(client);
            client = nullptr;
        }
        enet_deinitialize();
    }
}