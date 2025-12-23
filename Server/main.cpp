#include <chrono>
#include <thread>
#include "NetworkServer.h"
#include "GameWorld.h"

NetworkServer* g_Server = nullptr;
GameWorld* g_World = nullptr;

void OnConnect(ENetPeer* peer) {
    if(g_World)
        g_World->OnClientConnect(peer, g_Server);
}

void OnDisconnect(ENetPeer* peer) {
    if(g_World)
        g_World->OnClientDisconnect(peer, g_Server);
}

void OnPacket(ENetPeer* peer, const uint16_t type, void* data) {
    if(g_World)
        g_World->OnPacketReceived(peer, type, data);
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
        return -1;
    }

    constexpr float TICK_RATE = 1.0f / 50.0f;
    auto currentTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    while (true) {
        auto newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime.count();

        server.PollEvents();

        while (accumulator >= TICK_RATE) {

            world.UpdatePhysics(TICK_RATE);

            std::vector<Purpose::EntityState> snapshot = world.GetSnapshot();
            for (const auto& state : snapshot) {
                server.Broadcast(state);
            }

            server.Flush();

            accumulator -= TICK_RATE;
        }
    }

    server.Shutdown();
    return 0;
}