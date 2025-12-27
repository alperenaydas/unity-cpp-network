#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include <vector>
#include "NetworkClient.h"
#include "Protocol.h"
#include "BitStream.h" // Required for the new protocol

struct BotState {
    float x = 0, z = 0;
    bool active = false;
};

// --- YOUR AI HELPER FUNCTIONS ---
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

    // Buffer for the new BitStream protocol (Safe size)
    std::vector<uint8_t> bitBuffer(4096);

    std::cout << "[Bot] Started Hunter-Killer Logic Loop..." << std::endl;

    while (true) {
        currentTick++;
        client.ServiceNetwork();

        if (myID == 0) {
            myID = client.GetAssignedID();
            if (myID != 0) std::cout << "[Bot] Assigned ID: " << myID << std::endl;
        }

        // --- 1. NETWORK UPDATE (Adapted for BitStream) ---
        int bytesRead = client.CopyLatestBitstream(bitBuffer.data(), (int)bitBuffer.size());
        if (bytesRead > 0) {
            BitReader reader(bitBuffer.data(), bytesRead);

            // Read Header (Little Endian fix included)
            uint16_t typeLo = (uint16_t)reader.ReadBits(8);
            uint16_t typeHi = (uint16_t)reader.ReadBits(8);
            uint16_t type = typeLo | (typeHi << 8);

            if (type == Purpose::PACKET_WORLD_STATE) {
                uint32_t serverTick = reader.ReadBits(32);
                uint32_t baselineTick = reader.ReadBits(32);
                uint32_t entityCount = reader.ReadBits(8);

                for (uint32_t i = 0; i < entityCount; i++) {
                    uint32_t id = reader.ReadBits(32);
                    bool moved = reader.ReadBit();

                    int32_t qX = 0, qZ = 0;
                    if (moved) {
                        qX = reader.ReadInt(32);
                        qZ = reader.ReadInt(32);
                    }
                    float yaw = reader.ReadFloat(); // Read but ignore for AI logic

                    // Update World View
                    BotState& b = worldView[id];
                    b.active = true;
                    if (moved) {
                        b.x = (float)qX / Purpose::QUANT_RES;
                        b.z = (float)qZ / Purpose::QUANT_RES;
                    }
                }
            }
        }

        // --- 2. AI LOGIC (Your Restore Request) ---

        // Wait until we exist in the world
        if (myID == 0 || worldView.find(myID) == worldView.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        BotState& me = worldView[myID];
        uint32_t targetID = 0;
        float closestDist = 9999.0f;

        // Find closest target
        for (auto const& [id, entity] : worldView) {
            if (id == myID || !entity.active) continue; // Don't target self or dead
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

            // Aim at target
            yaw = CalculateYaw(dx, dz);

            // Fire if close enough (Burst fire logic)
            if (closestDist < 15.0f) {
                fire = (currentTick % 10 == 0);
            }

            // Move closer if too far
            if (closestDist > 3.0f) {
                NavigateTo(me.x, me.z, target.x, target.z, w, a, s, d);
            }
        } else {
            // No targets? Return to center (0,0) to find people
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