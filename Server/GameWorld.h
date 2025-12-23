#pragma once
#include <map>
#include <deque>
#include <vector>
#include <enet/enet.h>

#include "NetworkServer.h"
#include "Protocol.h"

struct Player {
    uint32_t id;
    float x, y, z;
    bool lastW, lastA, lastS, lastD;
    uint32_t lastProcessedTick;
    std::deque<Purpose::ClientInput> inputQueue;
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