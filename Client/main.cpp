#include <iostream>
#include <enet/enet.h>
#include "Protocol.h"


int main() {
    if (enet_initialize() != 0) {
        std::cerr << "ENet failed!" << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost* client = enet_host_create(NULL, 1, Purpose::CHANNEL_COUNT, 0, 0);

    if (client == nullptr) {
        std::cerr << "Could not create ENet client host." << std::endl;
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, Purpose::SERVER_IP);
    address.port = Purpose::SERVER_PORT;

    ENetPeer* peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);

    if (peer == nullptr) {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Connecting to Purpose Server at " << Purpose::SERVER_IP << "..." << std::endl;

    ENetEvent event;
    bool connected = false;

    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {

        std::cout << "Connection to server succeeded!" << std::endl;
        connected = true;

        Purpose::HandshakePacket packet;
        ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);

        enet_peer_send(peer, Purpose::CHANNEL_RELIABLE, enetPacket);

        enet_host_flush(client);
    } else {
        std::cerr << "Connection to server failed." << std::endl;
    }

    if (connected) {
        enet_host_service(client, &event, 1000);
    }

    enet_host_destroy(client);
    return 0;
}