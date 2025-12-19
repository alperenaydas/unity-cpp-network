#include <enet/enet.h>
#include "Protocol.h"
#include <iostream>

#define EXPORT_API __declspec(dllexport)

static ENetHost* client = nullptr;
static ENetPeer* peer = nullptr;

typedef void (*LogCallback)(const char* message);

static LogCallback unityLogger = nullptr;

void PurposeLog(const char* message) {
    if (unityLogger != nullptr) {
        unityLogger(message);
    }
}

extern "C" {
    EXPORT_API void RegisterLogCallback(LogCallback callback) {
        unityLogger = callback;
    }

    EXPORT_API bool ConnectToServer() {
        PurposeLog("[Plugin] ConnectToServer called from Unity!");
        if (enet_initialize() != 0) return false;

        client = enet_host_create(NULL, 1, Purpose::CHANNEL_COUNT, 0, 0);
        if (client == nullptr) return false;

        ENetAddress address;
        enet_address_set_host(&address, Purpose::SERVER_IP);
        address.port = Purpose::SERVER_PORT;

        peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);

        if (peer == nullptr) return false;

        ENetEvent event;
        if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
            return true;
        } else {
            enet_peer_reset(peer);
            return false;
        }
    }

    EXPORT_API void ServiceNetwork() {
        if (client == nullptr) return;

        ENetEvent event;
        while (enet_host_service(client, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    PurposeLog("[Plugin] Connected to Server!");
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    PurposeLog("[Plugin] Packet Received!");
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    PurposeLog("[Plugin] Disconnected!");
                    peer = nullptr;
                    break;
            }
        }
    }

    EXPORT_API void DisconnectFromServer() {
        if (peer != nullptr) {
            enet_peer_disconnect(peer, 0);
            enet_host_flush(client);
        }

        if (client != nullptr) {
            enet_host_destroy(client);
            client = nullptr;
        }
        enet_deinitialize();
    }
}