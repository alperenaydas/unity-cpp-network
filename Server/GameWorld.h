#pragma once
#include <map>
#include <deque>
#include <enet/enet.h>

#include "Protocol.h"
#include "SpatialGrid.h"

class NetworkServer;

struct WorldSnapshot {
    uint32_t tick = 0;
    int32_t qx = 0, qz = 0;
    float yaw = 0.0f;
};

struct Ray {
    float originX, originZ;
    float dirX, dirZ;
};

struct Player {
    uint32_t id = 0;
    ENetPeer* peer = nullptr;

    float x = 0.0f, z = 0.0f;
    float yaw = 0.0f;

    uint32_t lastProcessedTick = 0;
    uint32_t lastAckedTick = 0;
    Purpose::ClientInput lastInput;

    std::deque<Purpose::ClientInput> inputQueue;
    std::deque<WorldSnapshot> positionHistory;

    bool isAlive = true;
    float respawnTimer = 0.0f;

    bool isSpectator = false;

    void SaveHistory(uint32_t tick) {
        WorldSnapshot snap;
        snap.tick = tick;
        snap.qx = static_cast<int32_t>(x * Purpose::QUANT_RES);
        snap.qz = static_cast<int32_t>(z * Purpose::QUANT_RES);
        snap.yaw = static_cast<float>(yaw);
        positionHistory.push_front(snap);
        if (positionHistory.size() > 128) positionHistory.pop_back();
    }
};

struct EntityDist {
    uint32_t id;
    float distSq;
};

class GameWorld {
public:
    void OnClientConnect(ENetPeer* peer, NetworkServer* server);
    void OnClientDisconnect(ENetPeer* peer, NetworkServer* server);
    void OnPacketReceived(ENetPeer* peer, uint16_t type, void* data, size_t length, NetworkServer* server);

    void UpdatePhysics(float deltaTime, NetworkServer* server);
    void ProcessFire(uint32_t shooterID, float yaw, NetworkServer* server);

    void BroadcastWorldState(NetworkServer* server);

private:
    std::map<uint32_t, Player> players;
    uint32_t nextID = 1001;
    uint32_t currentServerTick = 0;

    bool RayIntersectsCircle(Ray ray, float cx, float cz, float radius, float& distance);
    bool GetPositionAtTick(const Player& target, double renderTick, float& outX, float& outZ);

    void SpawnPlayer(Player& p);

    SpatialGrid grid;
};