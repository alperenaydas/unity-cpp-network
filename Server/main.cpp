#include <iostream>
#include <cstdlib>
#include <ctime>
#include <enet/enet.h>
#include "Protocol.h"

void SendPacket(ENetPeer *peer, const void *data, size_t size, bool reliable = true) {
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket *packet = enet_packet_create(data, size, flags);
    enet_peer_send(peer, Purpose::CHANNEL_RELIABLE, packet);
}

int main(int argc, char **argv) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (enet_initialize() != 0) {
        std::cerr << "[-] ENet failed to initialize!" << std::endl;
        return EXIT_FAILURE;
    }
    std::atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    ENetHost *server = enet_host_create(&address, 32, Purpose::CHANNEL_COUNT, 0, 0);
    if (!server) {
        std::cerr << "[-] Could not start server on port " << Purpose::SERVER_PORT << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[+] Purpose Server started on port " << Purpose::SERVER_PORT << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    std::cout << "[Connect] Client from " << event.peer->address.host << ":" << event.peer->address.port
                            << std::endl;

                    Purpose::WelcomePacket welcome;
                    welcome.playerID = 777;
                    welcome.spawnX = 0.0f;
                    welcome.spawnY = 10.0f;
                    welcome.spawnZ = 0.0f;
                    SendPacket(event.peer, &welcome, sizeof(welcome));

                    Purpose::EntityState entity;
                    entity.type = Purpose::PACKET_ENTITY_UPDATE;
                    entity.networkID = 777;
                    entity.posX = static_cast<float>(std::rand() % 100);
                    entity.posY = welcome.spawnY;
                    entity.posZ = static_cast<float>(std::rand() % 100);
                    SendPacket(event.peer, &entity, sizeof(entity));

                    break;
                }

                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[Disconnect] Client left." << std::endl;
                    event.peer->data = nullptr;
                    break;

                default: break;
            }
        }
    }

    enet_host_destroy(server);
    return 0;
}
