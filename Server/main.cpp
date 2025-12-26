#include <chrono>
#include <thread>
#include <iostream>
#include "NetworkServer.h"
#include "GameWorld.h"

static NetworkServer* g_Server = nullptr;
static GameWorld* g_World = nullptr;

void OnConnect(ENetPeer* peer) {
    if (g_World && g_Server) g_World->OnClientConnect(peer, g_Server);
}

void OnDisconnect(ENetPeer* peer) {
    if (g_World && g_Server) g_World->OnClientDisconnect(peer, g_Server);
}

void OnPacket(ENetPeer* peer, const uint16_t type, void* data) {
    if (g_World) g_World->OnPacketReceived(peer, type, data);
}

int main() {
    NetworkServer server(Purpose::SERVER_PORT);
    GameWorld world;

    g_Server = &server;
    g_World = &world;

    server.SetConnectCallback(OnConnect);
    server.SetDisconnectCallback(OnDisconnect);
    server.SetPacketCallback(OnPacket);

    if (!server.Initialize()) {
        std::cerr << "CRITICAL: Server failed to start!" << std::endl;
        return -1;
    }

    constexpr float TICK_RATE = 1.0f / 50.0f; // 20ms
    auto currentTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    std::cout << "--- Purpose Server Running (50Hz) ---" << std::endl;

    while (true) {
        auto newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime.count();

        server.PollEvents();

        while (accumulator >= TICK_RATE) {
            world.UpdatePhysics(TICK_RATE);

            Purpose::WorldStatePacket worldState = world.GenerateWorldState();

            if (worldState.entityCount > 0) {
                size_t packetSize = sizeof(uint16_t) + sizeof(uint32_t) + (sizeof(Purpose::EntityData) * worldState.entityCount);
                server.Broadcast(&worldState, packetSize, false);
            }

            accumulator -= TICK_RATE;
        }

        server.Flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    server.Shutdown();
    return 0;
}