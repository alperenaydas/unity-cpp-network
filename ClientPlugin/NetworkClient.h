#pragma once
#include <enet/enet.h>
#include <atomic>
#include <vector>
#include <chrono>

#include "Protocol.h"
#include "LockFreeQueue.h"

using LogCallback = void(*)(const char*);

struct GamePacket {
    uint8_t data[Purpose::MTU_SIZE];
    size_t length;
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Connect(const char* ip, uint16_t port);
    void Disconnect();
    void ServiceNetwork();

    void SendInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw);
    void SendBecomeSpectatorRequest();

    uint32_t PopDespawnID();

    int CopyLatestBitstream(uint8_t* outBuffer, int maxLen);

    uint32_t GetAssignedID() const { return assignedPlayerID.load(); }
    Purpose::NetworkMetrics GetMetrics() const;

    void SetLogCallback(LogCallback cb) { logger = cb; }

private:
    void Log(const char* msg) const;

    ENetHost* clientHost = nullptr;
    ENetPeer* serverPeer = nullptr;
    std::atomic<uint32_t> assignedPlayerID{ 0 };

    LockFreeQueue<uint32_t> despawnQueue{ 64 };

    // Metrics
    std::atomic<uint64_t> totalBytesReceived{ 0 };
    std::atomic<uint64_t> totalBytesSent{ 0 };
    LogCallback logger = nullptr;
    std::atomic<uint64_t> bytesReceivedThisSecond{ 0 };
    std::atomic<uint64_t> bytesSentThisSecond{ 0 };
    std::atomic<float> currentInKBps{ 0.0f };
    std::atomic<float> currentOutKBps{ 0.0f };
    std::chrono::steady_clock::time_point lastMetricTime;
    uint32_t lastReceivedTick = 0;
    uint32_t packetsExpected = 0;
    uint32_t packetsReceived = 0;
    std::atomic<uint32_t> manualPacketLoss{ 0 };

    static const int MAX_PACKET_POOL_SIZE = 256;
    std::vector<GamePacket> packetMemory;
    LockFreeQueue<GamePacket*> packetQueue{ 512 };
    LockFreeQueue<GamePacket*> freePacketPool{ 512 };
};