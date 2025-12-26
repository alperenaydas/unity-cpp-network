#pragma once
#include <map>
#include <deque>
#include <enet/enet.h>

#include "Protocol.h"

class NetworkServer;

struct WorldSnapshot {
    uint32_t tick;
    float x, y, z;
};

struct Ray {
    float originX, originZ;
    float dirX, dirZ;
};

struct Player {
    uint32_t id;
    ENetPeer* peer;

    float x, y, z;
    float yaw;

    uint32_t lastProcessedTick = 0;
    Purpose::ClientInput lastInput;

    std::deque<Purpose::ClientInput> inputQueue;
    std::deque<WorldSnapshot> positionHistory;

    void SaveHistory(uint32_t tick) {
        positionHistory.push_front({tick, x, y, z});
        if (positionHistory.size() > 64) positionHistory.pop_back();
    }
};

class GameWorld {
public:
    void OnClientConnect(ENetPeer* peer, NetworkServer* server);
    void OnClientDisconnect(ENetPeer* peer, NetworkServer* server);
    void OnPacketReceived(ENetPeer* peer, uint16_t type, void* data);

    void UpdatePhysics(float deltaTime);
    void ProcessFire(uint32_t shooterID, float yaw);

    Purpose::WorldStatePacket GenerateWorldState();

private:
    std::map<uint32_t, Player> players;
    uint32_t nextID = 1001;

    const float MOVE_SPEED = 5.0f;

    bool RayIntersectsCircle(Ray ray, float cx, float cz, float radius, float& distance);
};