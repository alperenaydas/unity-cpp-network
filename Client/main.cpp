#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include "NetworkClient.h"
#include "Protocol.h"

struct BotState {
    float x = 0, z = 0;
    bool active = false;
};

float GetDistance(float x1, float z1, float x2, float z2) {
    return sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
}

float CalculateYaw(float dx, float dz) {
    return atan2f(dx, dz) * (180.0f / 3.14159265f);
}

void NavigateTo(float myX, float myZ, float targetX, float targetZ, bool& w, bool& a, bool& s, bool& d) {
    float threshold = 0.1f;
    if (targetZ > myZ + threshold) w = true;
    else if (targetZ < myZ - threshold) s = true;

    if (targetX > myX + threshold) d = true;
    else if (targetX < myX - threshold) a = true;
}

int main() {
    NetworkClient client;
    if (!client.Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT)) {
        std::cerr << "[Bot] Failed to connect to server." << std::endl;
        return -1;
    }

    uint32_t currentTick = 0;
    std::map<uint32_t, BotState> worldView;
    uint32_t myID = 0;

    std::cout << "[Bot] Started Stress-Test Logic Loop..." << std::endl;

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
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50Hz sleep
            continue;
        }

        BotState& me = worldView[myID];
        uint32_t targetID = 0;
        float closestDist = 9999.0f;

        for (auto const& [id, entity] : worldView) {
            if (id == myID || !entity.active) continue;
            float d = GetDistance(me.x, me.z, entity.x, entity.z);
            if (d < closestDist) {
                closestDist = d;
                targetID = id;
            }
        }

        bool w = false, a = false, s = false, d = false, fire = false;
        float yaw = 0;

        if (targetID != 0) {
            BotState& target = worldView[targetID];
            float dx = target.x - me.x;
            float dz = target.z - me.z;

            yaw = CalculateYaw(dx, dz);

            if (closestDist < 15.0f) {
                fire = (currentTick % 10 == 0);
            }

            if (closestDist > 3.0f) {
                NavigateTo(me.x, me.z, target.x, target.z, w, a, s, d);
            }
        } else {
            if (GetDistance(me.x, me.z, 0, 0) > 2.0f) {
                NavigateTo(me.x, me.z, 0, 0, w, a, s, d);
                yaw = CalculateYaw(0 - me.x, 0 - me.z);
            }
        }

        client.SendInput(currentTick, w, a, s, d, fire, yaw);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}