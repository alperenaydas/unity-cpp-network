#include <iostream>
#include <chrono>
#include <thread>
#include <enet/enet.h>
#include "Protocol.h"

void SendInput(ENetPeer *peer, bool w, bool a, bool s, bool d) {
    Purpose::ClientInput input;
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
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(currentTime - startTime).count();
        int currentStep = static_cast<int>(elapsedSeconds / 5.0) % 8;

        while (enet_host_service(client, &event, 10) > 0) {
            if (event.type == ENET_EVENT_TYPE_CONNECT) std::cout << "[Bot] Connected!" << std::endl;
            if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
        }

        if (peer->state == ENET_PEER_STATE_CONNECTED) {
            if (currentStep != lastStep) {
                switch (currentStep) {
                    case 0: SendInput(peer, true, false, false, false);
                        break;
                    case 1: SendInput(peer, false, false, false, true);
                        break;
                    case 2: SendInput(peer, false, false, true, false);
                        break;
                    case 3: SendInput(peer, false, true, false, false);
                        break;
                    case 4: SendInput(peer, true, false, false, true);
                        break;
                    case 5: SendInput(peer, true, true, false, false);
                        break;
                    case 6: SendInput(peer, false, true, true, false);
                        break;
                    case 7: SendInput(peer, false, false, true, true);
                        break;
                }
                lastStep = currentStep;
                std::cout << "[Bot] Changed Step to: " << currentStep << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
