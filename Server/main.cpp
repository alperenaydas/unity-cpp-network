#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include "NetworkServer.h"
#include "GameWorld.h"
#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

static NetworkServer* g_Server = nullptr;
static GameWorld* g_World = nullptr;
static std::atomic<bool> g_Running(true);

void SignalHandler(int signum) {
    std::cout << "\n[System] Interrupt signal (" << signum << ") received. Stopping..." << std::endl;
    g_Running = false;
}

void OnConnect(ENetPeer* peer) {
    if (g_World && g_Server) g_World->OnClientConnect(peer, g_Server);
}

void OnDisconnect(ENetPeer* peer) {
    if (g_World && g_Server) g_World->OnClientDisconnect(peer, g_Server);
}

void OnPacket(ENetPeer* peer, const uint16_t type, void* data, size_t length) {
    if (g_World) g_World->OnPacketReceived(peer, type, data, length, g_Server);
}

int main() {
    signal(SIGINT, SignalHandler);

#ifdef _WIN32
    timeBeginPeriod(1);
#endif

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

    auto currentTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    std::cout << "--- Purpose Server Running (50Hz) | Delta Compression Active ---" << std::endl;

    while (g_Running) {
        auto newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime.count();

        if (accumulator >= 0.25) {
            accumulator = 0.25;
        }
        server.PollEvents();

        bool physicsUpdated = false;
        while (accumulator >= Purpose::TICK_DELTA) {
            world.UpdatePhysics(Purpose::TICK_DELTA, g_Server);
            accumulator -= Purpose::TICK_DELTA;
            physicsUpdated = true;
        }

        if (physicsUpdated) {
            world.BroadcastWorldState(&server);
        }

        double timeUntilNextTick = Purpose::TICK_DELTA - accumulator;
        if (timeUntilNextTick > 0.002) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "[System] Shutting down network..." << std::endl;

#ifdef _WIN32
    timeEndPeriod(1);
#endif

    return 0;
}