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

    void SendInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw) const;

    bool PopEntityUpdate(Purpose::EntityState& outState);
    uint32_t PopDespawnID();
    uint32_t GetAssignedID() const { return assignedPlayerID; }
    
    void SetLogCallback(LogCallback cb) { logger = cb; }

private:
    void Log(const char* msg) const;

    ENetHost* clientHost = nullptr;
    ENetPeer* serverPeer = nullptr;
    std::atomic<uint32_t> assignedPlayerID{ 0 };

    static const int BUFFER_SIZE = 1024;
    Purpose::EntityState entityBuffer[BUFFER_SIZE];
    int head = 0;
    int tail = 0;
    std::mutex bufferMutex;

    std::deque<uint32_t> despawnQueue;
    std::mutex despawnMutex;

    LogCallback logger = nullptr;
};