#include <enet/enet.h>
#include "Protocol.h"
#include <iostream>

#define EXPORT_API __declspec(dllexport)

static ENetHost* client = nullptr;
static ENetPeer* peer = nullptr;

extern "C" {
    EXPORT_API bool ConnectToServer() {
        if (enet_initialize() != 0) return false;

        client = enet_host_create(NULL, 1, Purpose::CHANNEL_COUNT, 0, 0);
        if (client == nullptr) return false;

        ENetAddress address;
        enet_address_set_host(&address, Purpose::SERVER_IP);
        address.port = Purpose::SERVER_PORT;

        peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);
        return peer != nullptr;
    }

    EXPORT_API void ServiceNetwork() {
        if (client == nullptr) return;

        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "[Plugin] Connected to Server!" << std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "[Plugin] Packet Received!" << std::endl;
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[Plugin] Disconnected!" << std::endl;
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