#include <iostream>
#include <enet/enet.h>
#include "Protocol.h"

int main() {
    if (enet_initialize() != 0) return EXIT_FAILURE;
    atexit(enet_deinitialize);

    ENetHost* client = enet_host_create(NULL, 1, Purpose::CHANNEL_COUNT, 0, 0);
    if (!client) return EXIT_FAILURE;

    ENetAddress address;
    enet_address_set_host(&address, Purpose::SERVER_IP);
    address.port = Purpose::SERVER_PORT;

    ENetPeer* peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);
    if (!peer) return EXIT_FAILURE;

    std::cout << "Connecting to Purpose Server..." << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(client, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "[Client] Connected to Server!" << std::endl;
                    break;

                case ENET_EVENT_TYPE_RECEIVE: {
                    auto* state = reinterpret_cast<Purpose::EntityState*>(event.packet->data);
                    if (state->type == Purpose::PACKET_ENTITY_UPDATE) {
                        std::cout << "[Update] ID: " << state->networkID
                                  << " Pos: (" << state->posX << ", " << state->posZ << ")" << std::endl;
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[Client] Disconnected." << std::endl;
                    return 0;
            }
        }
    }
    return 0;
}