#pragma once
#include <map>
#include <deque>
#include <vector>
#include <enet/enet.h>

#include "NetworkServer.h"
#include "Protocol.h"

struct WorldSnapshot {
    uint32_t tick;
    float x, y, z;
};

struct Player {
    uint32_t id;
    float x, y, z;
    float yaw;
    bool lastW, lastA, lastS, lastD;
    uint32_t lastProcessedTick;
    std::deque<Purpose::ClientInput> inputQueue;
    std::deque<WorldSnapshot> positionHistory;

    void SaveHistory(uint32_t tick) {
        positionHistory.push_front({tick, x, y, z});
        if (positionHistory.size() > 64) {
            positionHistory.pop_back();
        }
    }
};

class GameWorld {
public:
    void OnClientConnect(ENetPeer *peer, NetworkServer *server);
    void OnClientDisconnect(ENetPeer *peer, NetworkServer *server);
    void OnPacketReceived(ENetPeer* peer, uint16_t type, void* data);

    void UpdatePhysics(float deltaTime);

    std::vector<Purpose::EntityState> GetSnapshot();

private:
    std::map<ENetPeer*, Player> players;
    uint32_t nextID = 1001;
    const float MOVE_SPEED = 0.1f; // 5.0 * 1/50
};