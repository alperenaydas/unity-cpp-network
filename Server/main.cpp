#include <iostream>
#include <enet/enet.h>
#include "Protocol.h"

int main(int argc, char** argv) {
    if (enet_initialize() != 0) {
        std::cerr << "ENet failed!" << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    ENetHost* server = enet_host_create(&address, 32, Purpose::CHANNEL_COUNT, 0, 0);
    if (!server) {
        std::cerr << "Could not start server!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Purpose Server started on port " << Purpose::SERVER_PORT << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    std::cout << "Client connected. Sending Welcome..." << std::endl;

                    Purpose::WelcomePacket welcome;
                    welcome.playerID = 777;
                    welcome.spawnX = 0.0f; welcome.spawnY = 10.0f; welcome.spawnZ = 0.0f;

                    ENetPacket* packet = enet_packet_create(&welcome, sizeof(welcome), ENET_PACKET_FLAG_RELIABLE);
                    enet_host_broadcast(server, Purpose::CHANNEL_RELIABLE, packet);
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "Packet received from client." << std::endl;
                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Client disconnected." << std::endl;
                    break;

                default: break;
            }
        }
    }
    return 0;
}