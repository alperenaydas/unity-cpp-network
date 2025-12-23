#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <enet/enet.h>
#include "Protocol.h"

struct ClientInputData {
    bool w, a, s, d;
    uint32_t tick;
};

struct ServerPlayer {
    uint32_t id;
    float x, y, z;
    std::deque<ClientInputData> inputQueue;
    bool lastW, lastA, lastS, lastD;
    uint32_t lastProcessedTick = 0;
};

std::map<ENetPeer *, ServerPlayer> g_serverWorldState;
uint32_t g_nextID = 1001;

int main() {
    if (enet_initialize() != 0) return EXIT_FAILURE;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = Purpose::SERVER_PORT;

    const float TICK_RATE = 1.0f / 50.0f;
    const float MOVE_SPEED = 5.0f * (float)TICK_RATE;

    auto currentTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    ENetHost *server = enet_host_create(&address, 32, Purpose::CHANNEL_COUNT, 0, 0);
    std::cout << "[Server] Waiting for Purpose Engine clients..." << std::endl;

    ENetEvent event;
    while (true) {
        auto newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime.count();

        while (enet_host_service(server, &event, 0) > 0) {
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
                        if (input->tick > playerState.lastProcessedTick) {
                            playerState.inputQueue.push_back({input->w, input->a, input->s, input->d, input->tick});
                        }
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

        while (accumulator >= TICK_RATE) {
            for (auto& [peer, playerState] : g_serverWorldState) {

                if (!playerState.inputQueue.empty()) {
                    auto input = playerState.inputQueue.front();
                    playerState.inputQueue.pop_front();

                    playerState.lastW = input.w;
                    playerState.lastA = input.a;
                    playerState.lastS = input.s;
                    playerState.lastD = input.d;
                    playerState.lastProcessedTick = input.tick;
                }

                if (playerState.lastW) playerState.z += MOVE_SPEED;
                if (playerState.lastS) playerState.z -= MOVE_SPEED;
                if (playerState.lastA) playerState.x -= MOVE_SPEED;
                if (playerState.lastD) playerState.x += MOVE_SPEED;
            }

            for (auto const& [peer, data] : g_serverWorldState) {
                Purpose::EntityState state;
                state.type = Purpose::PACKET_ENTITY_UPDATE;
                state.networkID = data.id;
                state.lastProcessedTick = data.lastProcessedTick;
                state.posX = data.x;
                state.posY = data.y;
                state.posZ = data.z;

                ENetPacket* packet = enet_packet_create(&state, sizeof(state), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_host_broadcast(server, Purpose::CHANNEL_UNRELIABLE, packet);
            }

            enet_host_flush(server);

            accumulator -= TICK_RATE;
        }


    }
}
