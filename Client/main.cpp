#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include <vector>
#include <random>
#include "NetworkClient.h"
#include "Protocol.h"
#include "BitStream.h"

// Simple struct to keep track of the world
struct BotState {
    float x = 0, z = 0;
    float yaw = 0; // Added Yaw history
    uint32_t lastUpdateTick = 0;
};

// --- MATH HELPERS ---
float GetDistance(float x1, float z1, float x2, float z2) {
    return sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
}

float CalculateYaw(float dx, float dz) {
    return atan2f(dx, dz) * (180.0f / Purpose::PI);
}

// Returns 0 if no enemy found
uint32_t GetClosestEnemy(uint32_t myID, const std::map<uint32_t, BotState>& world, float myX, float myZ) {
    uint32_t targetID = 0;
    float minDist = 10000.0f; // Max range

    for (const auto& [id, bot] : world) {
        if (id == myID) continue;

        float dist = GetDistance(myX, myZ, bot.x, bot.z);
        if (dist < minDist) {
            minDist = dist;
            targetID = id;
        }
    }
    return targetID;
}

void NavigateTo(float myX, float myZ, float targetX, float targetZ, bool& w, bool& a, bool& s, bool& d) {
    // Keep distance (3 meters)
    float dist = GetDistance(myX, myZ, targetX, targetZ);
    if (dist < 3.0f) return;

    float threshold = 0.5f;
    if (targetZ > myZ + threshold) w = true;
    else if (targetZ < myZ - threshold) s = true;

    if (targetX > myX + threshold) d = true;
    else if (targetX < myX - threshold) a = true;
}

// --------------------------------

int main() {
    // Connect to Server
    NetworkClient client;
    if (!client.Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT)) {
        std::cerr << "[Bot] Failed to connect." << std::endl;
        return -1;
    }

    // State
    uint32_t currentTick = 0;
    std::map<uint32_t, BotState> worldView;
    uint32_t myID = 0;
    std::vector<uint8_t> bitBuffer(4096);

    // AI State
    float wanderX = 0, wanderZ = 0;
    bool hasWanderTarget = false;

    // Random Number Gen for Wander
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> rangeDist(-Purpose::SPAWN_RANGE, Purpose::SPAWN_RANGE);

    std::cout << "[Bot] ONLINE. Waiting for ID..." << std::endl;

    while (true) {
        currentTick++;
        client.ServiceNetwork();

        // 1. Get ID
        if (myID == 0) {
            myID = client.GetAssignedID();
            if (myID != 0) std::cout << "[Bot] Assigned ID: " << myID << " - ENGAGING." << std::endl;
        }

        // 2. PROCESS PACKETS
        int bytesRead = 0;
        while ((bytesRead = client.CopyLatestBitstream(bitBuffer.data(), (int)bitBuffer.size())) > 0) {
            BitReader reader(bitBuffer.data(), bytesRead * 8); // SIZE IN BITS!

            uint16_t typeLo = (uint16_t)reader.ReadBits(8);
            uint16_t typeHi = (uint16_t)reader.ReadBits(8);
            uint16_t type = typeLo | (typeHi << 8);

            if (type == Purpose::PACKET_WORLD_STATE) {
                uint32_t serverTick = reader.ReadBits(32);
                uint32_t baselineTick = reader.ReadBits(32);
                uint32_t entityCount = reader.ReadBits(10);

                for (uint32_t i = 0; i < entityCount; i++) {
                    uint32_t id = reader.ReadBits(32);

                    // --- DECODE POSITION ---
                    bool posChanged = reader.ReadBit();
                    int32_t qX = 0, qZ = 0;
                    if (posChanged) {
                        qX = reader.ReadInt(32);
                        qZ = reader.ReadInt(32);
                    }

                    // --- DECODE ROTATION ---
                    bool rotChanged = reader.ReadBit();
                    float yaw = 0;
                    if (rotChanged) {
                        yaw = reader.ReadFloat();
                    }

                    // --- UPDATE STATE ---
                    BotState& b = worldView[id];
                    b.lastUpdateTick = currentTick;

                    if (posChanged) {
                        b.x = (float)qX / Purpose::QUANT_RES;
                        b.z = (float)qZ / Purpose::QUANT_RES;
                    }
                    if (rotChanged) {
                        b.yaw = yaw;
                    }
                }
            }
        }

        // 3. CLEANUP GHOSTS (2 seconds timeout)
        for (auto it = worldView.begin(); it != worldView.end(); ) {
            if (it->first != myID && currentTick > it->second.lastUpdateTick + 100) {
                it = worldView.erase(it);
            } else {
                ++it;
            }
        }

        if (myID != 0) {
            BotState& me = worldView[myID];
            bool w = false, a = false, s = false, d = false, fire = false;
            float targetYaw = me.yaw;

            // Strategy: Hunter-Killer
            uint32_t targetID = GetClosestEnemy(myID, worldView, me.x, me.z);

            if (targetID != 0) {
                BotState& target = worldView[targetID];

                // 1. Calculate Distance
                float dist = GetDistance(me.x, me.z, target.x, target.z);

                // 2. Aim
                targetYaw = CalculateYaw(target.x - me.x, target.z - me.z);

                // 3. Move (Stop if too close to avoid collision)
                NavigateTo(me.x, me.z, target.x, target.z, w, a, s, d);

                // 4. Fire Control (FIXED: Only shoot if close enough!)
                // Only fire if within 15 meters AND aim is steady
                if (dist < 15.0f && currentTick % 10 == 0) {
                    fire = true;
                }

                hasWanderTarget = false;
            }
            else {
                // Patrol (No enemies visible)
                if (!hasWanderTarget || GetDistance(me.x, me.z, wanderX, wanderZ) < 2.0f) {
                    wanderX = rangeDist(rng);
                    wanderZ = rangeDist(rng);
                    hasWanderTarget = true;
                }
                NavigateTo(me.x, me.z, wanderX, wanderZ, w, a, s, d);
                targetYaw = CalculateYaw(wanderX - me.x, wanderZ - me.z);
            }

            // Prediction (Dead Reckoning) - Makes bot movement smoother locally
            float dt = 0.02f;
            float dx = 0, dz = 0;
            if (w) dz += 1; if (s) dz -= 1;
            if (d) dx += 1; if (a) dx -= 1;
            float len = sqrtf(dx*dx + dz*dz);
            if (len > 0) {
                me.x += (dx/len) * Purpose::MOVE_SPEED * dt;
                me.z += (dz/len) * Purpose::MOVE_SPEED * dt;
            }
            me.yaw = targetYaw;

            // Send
            client.SendInput(currentTick, w, a, s, d, fire, targetYaw);
        }

        // 5. TICK RATE CONTROL
        // Sleep 20ms (+/- random jitter to simulate real clients)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}