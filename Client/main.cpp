#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include "NetworkClient.h"
#include "Protocol.h"

// Constants for the visualization
const float OSCILLATION_SPEED = 2.0f; // Speed of left-right movement
const float OSCILLATION_WIDTH = 5.0f; // How far left/right it goes
const bool IS_TARGET_BOT = true;      // Set to false for the "Shooter" bot

struct BotState {
    float x = 0, z = 0;
    bool active = false;
};

int main() {
    NetworkClient client;
    if (!client.Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT)) {
        std::cerr << "[Bot] Failed to connect to server." << std::endl;
        return -1;
    }

    uint32_t currentTick = 0;
    std::map<uint32_t, BotState> worldView;
    uint32_t myID = 0;

    // Use a high-resolution clock for smooth movement math
    auto startTime = std::chrono::steady_clock::now();

    std::cout << "[Bot] Started Visualization Logic..." << std::endl;

    while (true) {
        currentTick++;
        client.ServiceNetwork();

        if (myID == 0) {
            myID = client.GetAssignedID();
            if (myID != 0) std::cout << "[Bot] My assigned ID: " << myID << std::endl;
        }

        Purpose::EntityData update;
        while (client.PopEntityData(update)) {
            worldView[update.networkID].x = update.posX;
            worldView[update.networkID].z = update.posZ;
            worldView[update.networkID].active = true;
        }

        if (myID == 0 || worldView.find(myID) == worldView.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        bool w = false, a = false, s = false, d = false, fire = false;
        float yaw = 0;

        if (IS_TARGET_BOT) {
            // Target Bot Logic: Oscillate left and right continuously
            auto currentTime = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(currentTime - startTime).count();

            // Calculate target X position based on a Sine wave
            float targetX = std::sin(elapsed * OSCILLATION_SPEED) * OSCILLATION_WIDTH;
            float currentX = worldView[myID].x;

            // Simple "P-Controller" style movement to reach the target X
            if (targetX > currentX + 0.1f) d = true;
            else if (targetX < currentX - 0.1f) a = true;

            // Look forward (assuming Z is forward)
            yaw = 0.0f;
        }
        else {
            // Your existing "Shooter" logic can stay here if you want
            // the bot to try and shoot the target bot.
        }

        client.SendInput(currentTick, w, a, s, d, fire, yaw);

        // Match server tick rate (e.g., 50Hz = 20ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}