#include <iostream>
#include <map>
#include <enet/enet.h>
#include "Protocol.h"

struct ServerPlayer {
    uint32_t id;
    float x, y, z;
    bool w, a, s, d;
};

std::map<ENetPeer *, ServerPlayer> g_serverWorldState;
uint32_t g_nextID = 1001;

int main() {
    if (enet_initialize() != 0) return EXIT_FAILURE;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    const float TICK_RATE = 1.0f / 60.0f;

    ENetHost *server = enet_host_create(&address, 32, Purpose::CHANNEL_COUNT, 0, 0);
    std::cout << "[Server] Waiting for Purpose Engine clients..." << std::endl;

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    uint32_t newID = g_nextID++;
                    g_serverWorldState[event.peer] = {newID, 0.0f, 0.0f, 0.0f};

                    std::cout << "[Connect] Assigned ID: " << newID << std::endl;

                    Purpose::WelcomePacket welcome;
                    welcome.playerID = newID;
                    ENetPacket *wPacket = enet_packet_create(&welcome, sizeof(welcome), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, Purpose::CHANNEL_RELIABLE, wPacket);
                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT: {
                    uint32_t leftID = g_serverWorldState[event.peer].id;
                    std::cout << "[Disconnect] ID " << leftID << " left." << std::endl;
                    Purpose::EntityDespawn despawn;
                    despawn.networkID = leftID;

                    ENetPacket *dPacket = enet_packet_create(&despawn, sizeof(despawn), ENET_PACKET_FLAG_RELIABLE);
                    enet_host_broadcast(server, Purpose::CHANNEL_RELIABLE, dPacket);

                    g_serverWorldState.erase(event.peer);
                    break;
                }

                case ENET_EVENT_TYPE_RECEIVE: {
                    uint16_t packetType = *reinterpret_cast<uint16_t *>(event.packet->data);
                    if (packetType == Purpose::PACKET_CLIENT_INPUT) {
                        auto *input = reinterpret_cast<Purpose::ClientInput *>(event.packet->data);
                        auto &playerState = g_serverWorldState[event.peer];
                        playerState.w = input->w;
                        playerState.a = input->a;
                        playerState.s = input->s;
                        playerState.d = input->d;
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
            }
        }

        // for (auto const& [peer, data] : g_serverWorldState) {
        //     Purpose::EntityState state;
        //     state.type = Purpose::PACKET_ENTITY_UPDATE;
        //     state.networkID = data.id;
        //     state.posX = data.x; state.posY = data.y; state.posZ = data.z;
        //
        //     ENetPacket* packet = enet_packet_create(&state, sizeof(state), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        //     enet_host_broadcast(server, Purpose::CHANNEL_UNRELIABLE, packet);
        //
        //     g_activePlayers[peer].x += 0.01f;
        // }

        float moveSpeed = 5.0f * TICK_RATE;
        for (auto &[peer, playerState]: g_serverWorldState) {
            if (playerState.w) playerState.z += moveSpeed;
            if (playerState.s) playerState.z -= moveSpeed;
            if (playerState.a) playerState.x -= moveSpeed;
            if (playerState.d) playerState.x += moveSpeed;

            Purpose::EntityState state;
            state.type = Purpose::PACKET_ENTITY_UPDATE;
            state.networkID = playerState.id;
            state.posX = playerState.x;
            state.posY = playerState.y;
            state.posZ = playerState.z;

            ENetPacket *packet = enet_packet_create(&state, sizeof(state), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
            enet_host_broadcast(server, Purpose::CHANNEL_UNRELIABLE, packet);
        }
    }
}
