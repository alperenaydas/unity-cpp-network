#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <map>
#include <vector>
#include "NetworkClient.h"
#include "Protocol.h"
#include "BitStream.h"

struct BotState {
    float x = 0, z = 0;
    bool active = false;
};

// --- HELPER FUNCTIONS ---
float GetDistance(float x1, float z1, float x2, float z2) {
    return sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
}

float CalculateYaw(float dx, float dz) {
    return atan2f(dx, dz) * (180.0f / 3.14159265f);
}

// Updated NavigateTo to be slightly more aggressive for the sine wave
void NavigateTo(float myX, float myZ, float targetX, float targetZ, bool& w, bool& a, bool& s, bool& d) {
    float threshold = 0.2f; // Slightly larger threshold to prevent jitter at the precise target point

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

    std::cout << "[Bot] Started SINE WAVE pattern..." << std::endl;

    while (true) {
        currentTick++;
        client.ServiceNetwork();

        if (myID == 0) {
            myID = client.GetAssignedID();
            if (myID != 0) std::cout << "[Bot] Assigned ID: " << myID << std::endl;
        }

        // --- 1. NETWORK UPDATE (DRAIN THE QUEUE) ---
        // Change 'if' to 'while' to process ALL pending packets
        int bytesRead = 0;
        while ((bytesRead = client.CopyLatestBitstream(bitBuffer.data(), (int)bitBuffer.size())) > 0) {
            BitReader reader(bitBuffer.data(), bytesRead);

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
                    float yaw = reader.ReadFloat();

                    BotState& b = worldView[id];
                    b.active = true;
                    if (moved) {
                        b.x = (float)qX / Purpose::QUANT_RES;
                        b.z = (float)qZ / Purpose::QUANT_RES;
                    }
                }
            }
        }
        // At this point, 'worldView' contains the absolute latest state from the server.

        // --- 2. AI LOGIC ---
        if (myID == 0 || worldView.find(myID) == worldView.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        BotState& me = worldView[myID];

        bool w = false, a = false, s = false, d = false, fire = false;
        float yaw = 0;

        float timeVal = currentTick * 0.05f;
        float desiredX = sinf(timeVal) * 8.0f;
        float desiredZ = 10.0f;

        NavigateTo(me.x, me.z, desiredX, desiredZ, w, a, s, d);
        yaw = CalculateYaw(desiredX - me.x, desiredZ - me.z);

        client.SendInput(currentTick, w, a, s, d, fire, yaw);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}