#include <iostream>
#include <map>
#include <enet/enet.h>
#include "Protocol.h"

struct ServerPlayer {
    uint32_t id;
    float x, y, z;
};

std::map<ENetPeer*, ServerPlayer> g_activePlayers;
uint32_t g_nextID = 1001;

int main() {
    if (enet_initialize() != 0) return EXIT_FAILURE;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    ENetHost* server = enet_host_create(&address, 32, Purpose::CHANNEL_COUNT, 0, 0);
    std::cout << "[Server] Waiting for Purpose Engine clients..." << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    uint32_t newID = g_nextID++;
                    g_activePlayers[event.peer] = { newID, 0.0f, 0.0f, 0.0f };

                    std::cout << "[Connect] Assigned ID: " << newID << std::endl;

                    Purpose::WelcomePacket welcome;
                    welcome.playerID = newID;
                    ENetPacket* wPacket = enet_packet_create(&welcome, sizeof(welcome), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, Purpose::CHANNEL_RELIABLE, wPacket);
                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[Disconnect] ID " << g_activePlayers[event.peer].id << " left." << std::endl;
                    g_activePlayers.erase(event.peer);
                    break;
            }
        }

        for (auto const& [peer, data] : g_activePlayers) {
            Purpose::EntityState state;
            state.type = Purpose::PACKET_ENTITY_UPDATE;
            state.networkID = data.id;
            state.posX = data.x; state.posY = data.y; state.posZ = data.z;

            ENetPacket* packet = enet_packet_create(&state, sizeof(state), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
            enet_host_broadcast(server, Purpose::CHANNEL_UNRELIABLE, packet);

            g_activePlayers[peer].x += 0.01f;
        }
    }
}