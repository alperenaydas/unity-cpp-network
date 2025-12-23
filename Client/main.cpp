#include <iostream>
#include <chrono>
#include <thread>
#include <enet/enet.h>
#include "Protocol.h"

void SendInput(ENetPeer *peer, uint32_t tick, bool w, bool a, bool s, bool d) {
    Purpose::ClientInput input;
    input.tick = tick;
    input.w = w;
    input.a = a;
    input.s = s;
    input.d = d;
    ENetPacket *packet = enet_packet_create(&input, sizeof(input), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(peer, Purpose::CHANNEL_UNRELIABLE, packet);
}

int main() {
    if (enet_initialize() != 0) return EXIT_FAILURE;
    atexit(enet_deinitialize);

    ENetHost *client = enet_host_create(NULL, 1, Purpose::CHANNEL_COUNT, 0, 0);
    if (!client) return EXIT_FAILURE;

    ENetAddress address;
    enet_address_set_host(&address, Purpose::SERVER_IP);
    address.port = Purpose::SERVER_PORT;

    ENetPeer *peer = enet_host_connect(client, &address, Purpose::CHANNEL_COUNT, 0);
    if (!peer) return EXIT_FAILURE;

    auto startTime = std::chrono::steady_clock::now();

    std::cout << "[Bot] Connecting to Purpose Server..." << std::endl;

    ENetEvent event;
    int lastStep = -1;

    uint32_t currentTick = 0;

    bool lastW = false, lastA = false, lastS = false, lastD = false;

    while (true) {
        currentTick++;

        auto currentTime = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(currentTime - startTime).count();
        int currentStep = static_cast<int>(elapsedSeconds / 5.0) % 8;

        while (enet_host_service(client, &event, 10) > 0) {
            if (event.type == ENET_EVENT_TYPE_CONNECT) std::cout << "[Bot] Connected!" << std::endl;
            if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
        }

        if (peer->state == ENET_PEER_STATE_CONNECTED) {
            bool w = false, a = false, s = false, d = false;

            switch (currentStep) {
                case 0: w = true; break;
                case 1: d = true; break;
                case 2: s = true; break;
                case 3: a = true; break;
                case 4: w = true; d = true; break;
                case 5: w = true; a = true; break;
                case 6: s = true; a = true; break;
                case 7: s = true; d = true; break;
            }

            bool changed = (w != lastW || a != lastA || s != lastS || d != lastD);
            bool isHeartbeat = (currentTick % 6 == 0);

            if (changed || isHeartbeat) {
                SendInput(peer, currentTick, w, a, s, d);

                if (changed) {
                    std::cout << "[Bot] Step " << currentStep << " | Tick: " << currentTick << std::endl;
                    lastStep = currentStep;
                }

                lastW = w; lastA = a; lastS = s; lastD = d;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}