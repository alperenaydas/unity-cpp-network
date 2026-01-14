#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include <vector>
#include <limits> // For numeric_limits
#include "NetworkClient.h"
#include "Protocol.h"
#include "BitStream.h"

struct BotState {
    float x = 0, z = 0;
    uint32_t lastUpdateTick = 0; // To track ghosts
};

// --- HELPER FUNCTIONS ---
float GetDistance(float x1, float z1, float x2, float z2) {
    return sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
}

float CalculateYaw(float dx, float dz) {
    // atan2(x, z) gives angle from the Z-axis (North)
    return atan2f(dx, dz) * (180.0f / 3.14159265f);
}

// Returns 0 if no enemy found
uint32_t GetClosestEnemy(uint32_t myID, const std::map<uint32_t, BotState>& world, float myX, float myZ) {
    uint32_t targetID = 0;
    float minDist = std::numeric_limits<float>::max();

    for (const auto& [id, bot] : world) {
        if (id == myID) continue; // Don't shoot self

        float dist = GetDistance(myX, myZ, bot.x, bot.z);
        if (dist < minDist) {
            minDist = dist;
            targetID = id;
        }
    }
    return targetID;
}

void NavigateTo(float myX, float myZ, float targetX, float targetZ, bool& w, bool& a, bool& s, bool& d) {
    // Stop moving if we are very close (2.0 meters) to avoid clipping inside them
    float dist = GetDistance(myX, myZ, targetX, targetZ);
    if (dist < 3.0f) return;

    float threshold = 0.2f;
    if (targetZ > myZ + threshold) w = true;
    else if (targetZ < myZ - threshold) s = true;

    if (targetX > myX + threshold) d = true;
    else if (targetX < myX - threshold) a = true;
}
// --------------------------------

int main() {
    NetworkClient client;
    if (!client.Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT)) {
        std::cerr << "[Bot] Failed to connect to server." << std::endl;
        return -1;
    }

    uint32_t currentTick = 0;
    std::map<uint32_t, BotState> worldView;
    uint32_t myID = 0;
    std::vector<uint8_t> bitBuffer(4096);

    // Random wander target for when no enemies are found
    float wanderX = 0, wanderZ = 0;
    bool hasWanderTarget = false;

    std::cout << "[Bot] HUNTER-KILLER MODE ENGAGED." << std::endl;

    while (true) {
        currentTick++;
        client.ServiceNetwork();

        if (myID == 0) {
            myID = client.GetAssignedID();
            if (myID != 0) std::cout << "[Bot] Assigned ID: " << myID << std::endl;
        }

        // --- 1. NETWORK UPDATE ---
        int bytesRead = 0;
        while ((bytesRead = client.CopyLatestBitstream(bitBuffer.data(), (int)bitBuffer.size())) > 0) {
            BitReader reader(bitBuffer.data(), bytesRead);
            uint16_t typeLo = (uint16_t)reader.ReadBits(8);
            uint16_t typeHi = (uint16_t)reader.ReadBits(8);
            uint16_t type = typeLo | (typeHi << 8);

            if (type == Purpose::PACKET_WORLD_STATE) {
                uint32_t serverTick = reader.ReadBits(32);
                uint32_t baselineTick = reader.ReadBits(32);
                uint32_t entityCount = reader.ReadBits(10); // Matches your 10-bit server update

                for (uint32_t i = 0; i < entityCount; i++) {
                    uint32_t id = reader.ReadBits(32);
                    bool moved = reader.ReadBit();
                    int32_t qX = 0, qZ = 0;
                    if (moved) {
                        qX = reader.ReadInt(32);
                        qZ = reader.ReadInt(32);
                    }
                    float yaw = reader.ReadFloat();

                    BotState& b = worldView[id];
                    b.lastUpdateTick = currentTick; // Refresh timestamp
                    if (moved) {
                        b.x = (float)qX / Purpose::QUANT_RES;
                        b.z = (float)qZ / Purpose::QUANT_RES;
                    }
                }
            }
        }

        // Cleanup: Remove entities we haven't seen in a while (Ghosts)
        // Since Grid management stops sending far entities, we must prune them.
        for (auto it = worldView.begin(); it != worldView.end(); ) {
            if (it->first != myID && currentTick > it->second.lastUpdateTick + 100) { // ~2 seconds timeout
                it = worldView.erase(it);
            } else {
                ++it;
            }
        }

        // --- 2. COMBAT AI ---
        if (myID == 0 || worldView.find(myID) == worldView.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        BotState& me = worldView[myID];
        bool w = false, a = false, s = false, d = false, fire = false;
        float yaw = 0;

        // A. Find Target
        uint32_t targetID = GetClosestEnemy(myID, worldView, me.x, me.z);

        if (targetID != 0) {
            // TARGET FOUND: ATTACK!
            BotState& target = worldView[targetID];

            // 1. Aim
            yaw = CalculateYaw(target.x - me.x, target.z - me.z);

            // 2. Move to engagement range
            NavigateTo(me.x, me.z, target.x, target.z, w, a, s, d);

            // 3. Fire logic (Spam fire every few ticks)
            if (currentTick % 5 == 0) fire = true;

            // Reset wander state
            hasWanderTarget = false;
        }
        else {
            // NO TARGET: PATROL / WANDER
            if (!hasWanderTarget || GetDistance(me.x, me.z, wanderX, wanderZ) < 2.0f) {
                // Pick new random spot within bounds (-40 to 40)
                wanderX = -40.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 80.0f));
                wanderZ = -40.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 80.0f));
                hasWanderTarget = true;
            }

            NavigateTo(me.x, me.z, wanderX, wanderZ, w, a, s, d);
            yaw = CalculateYaw(wanderX - me.x, wanderZ - me.z);
        }

        // --- 3. DEAD RECKONING (SELF-PREDICTION) ---
        // Crucial for smooth bot movement when server lags
        float dt = 0.02f;
        float speed = 5.0f;
        float dx = 0, dz = 0;
        if (w) dz += 1; if (s) dz -= 1;
        if (d) dx += 1; if (a) dx -= 1;

        float len = sqrtf(dx*dx + dz*dz);
        if (len > 0) {
            me.x += (dx/len) * speed * dt;
            me.z += (dz/len) * speed * dt;
        }

        // --- 4. SEND INPUT ---
        client.SendInput(currentTick, w, a, s, d, fire, yaw);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}