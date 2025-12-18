#include <iostream>
#include <enet/enet.h>
#include "Protocol.h"

int main (int argc, char ** argv)
{
    if (enet_initialize () != 0) {
        std::cerr << "ENet failed!" << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);

    if (server == nullptr) {
        std::cerr << "Could not start server!" << std::endl;
        return 1;
    }

    std::cout << "Purpose ENet Server started on port " << Purpose::SERVER_PORT << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "Client connected from " << event.peer->address.host << std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "Packet received!" << std::endl;
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Client disconnected." << std::endl;
                    break;
                case ENET_EVENT_TYPE_NONE:
                    std::cout << "Unknown event type." << std::endl;
                    break;
            }
        }
    }
    return 0;
}
