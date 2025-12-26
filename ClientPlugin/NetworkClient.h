#pragma once
#include <enet/enet.h>
#include <deque>
#include <mutex>
#include <atomic>
#include "Protocol.h"

using LogCallback = void(*)(const char*);

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Connect(const char* ip, uint16_t port);
    void Disconnect();
    void ServiceNetwork();

    void SendInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw);

    bool PopEntityData(Purpose::EntityData& outData);
    uint32_t PopDespawnID();

    uint32_t GetAssignedID() const { return assignedPlayerID.load(); }
    Purpose::NetworkMetrics GetMetrics() const;

    void SetLogCallback(LogCallback cb) { logger = cb; }

private:
    void Log(const char* msg) const;

    ENetHost* clientHost = nullptr;
    ENetPeer* serverPeer = nullptr;
    std::atomic<uint32_t> assignedPlayerID{ 0 };

    static const int ENTITY_BUFFER_SIZE = 4096;
    Purpose::EntityData entityBuffer[ENTITY_BUFFER_SIZE];
    std::atomic<int> head{ 0 };
    std::atomic<int> tail{ 0 };

    std::deque<uint32_t> despawnQueue;
    std::mutex despawnMutex;

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
};