#include <iostream>
#include <chrono>
#include <thread>
#include "NetworkClient.h"
#include "Protocol.h"

void BotLogger(const char* msg) {
    std::cout << "[Bot] " << msg << std::endl;
}

int main() {
    NetworkClient client;
    client.SetLogCallback(BotLogger);

    if (!client.Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT)) {
        std::cerr << "Failed to connect to server." << std::endl;
        return -1;
    }

    auto startTime = std::chrono::steady_clock::now();
    uint32_t currentTick = 0;

    bool lastW = false, lastA = false, lastS = false, lastD = false;
    int lastStep = -1;

    std::cout << "Bot running. Press Ctrl+C to stop." << std::endl;

    while (true) {
        currentTick++;

        client.ServiceNetwork();

        auto currentTime = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(currentTime - startTime).count();
        int currentStep = static_cast<int>(elapsedSeconds / 5.0) % 8;

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
            client.SendInput(currentTick, w, a, s, d);

            if (changed) {
                std::cout << "Step " << currentStep << " | Tick: " << currentTick << std::endl;
                lastStep = currentStep;
            }

            lastW = w; lastA = a; lastS = s; lastD = d;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}