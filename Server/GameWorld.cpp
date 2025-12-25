#include "GameWorld.h"
#include <iostream>

class NetworkServer;

void GameWorld::OnClientConnect(ENetPeer* peer, NetworkServer* server) {
    const uint32_t newID = nextID++;
    
    Player newPlayer;
    newPlayer.id = newID;
    newPlayer.x = 0; newPlayer.y = 0; newPlayer.z = 0;
    newPlayer.lastProcessedTick = 0;
    newPlayer.lastW = newPlayer.lastA = newPlayer.lastS = newPlayer.lastD = false;
    
    players[peer] = newPlayer;

    std::cout << "[Game] Player joined. Assigned ID: " << newID << std::endl;

    Purpose::WelcomePacket welcome;
    welcome.type = Purpose::PACKET_WELCOME;
    welcome.playerID = newID;
    server->SendToPeer(peer, &welcome, sizeof(welcome), true);
}

void GameWorld::OnClientDisconnect(ENetPeer* peer, NetworkServer* server) {
    if (players.count(peer) == 0) return;

    const uint32_t id = players[peer].id;
    std::cout << "[Game] Player " << id << " disconnected." << std::endl;

    Purpose::EntityDespawn despawn;
    despawn.type = Purpose::PACKET_ENTITY_DESPAWN;
    despawn.networkID = id;
    server->SendToPeer(peer, &despawn, sizeof(despawn), true);

    players.erase(peer);
}

void GameWorld::OnPacketReceived(ENetPeer* peer, const uint16_t type, void* data) {
    if (type == Purpose::PACKET_CLIENT_INPUT) {
        const auto* input = static_cast<Purpose::ClientInput*>(data);
        
        if (players.find(peer) != players.end()) {
            Player& p = players[peer];

            if (input->tick > p.lastProcessedTick) {
                p.inputQueue.push_back(*input);
            }
        }
    }
}

void GameWorld::UpdatePhysics(const float deltaTime) {
    const float moveDistance = 5.0f * deltaTime;

    for (auto& [peer, player] : players) {
        if (!player.inputQueue.empty()) {
            const auto input = player.inputQueue.front();
            player.inputQueue.pop_front();

            player.lastW = input.w;
            player.lastA = input.a;
            player.lastS = input.s;
            player.lastD = input.d;
            player.lastProcessedTick = input.tick;
            player.yaw = input.mouseYaw;

            if (input.fire) {
                // Shooting logic will go here
            }
        }

        if (player.lastW) player.z += moveDistance;
        if (player.lastS) player.z -= moveDistance;
        if (player.lastA) player.x -= moveDistance;
        if (player.lastD) player.x += moveDistance;
    }
}

std::vector<Purpose::EntityState> GameWorld::GetSnapshot() {
    std::vector<Purpose::EntityState> snapshot;
    snapshot.reserve(players.size());

    for (auto const& [peer, p] : players) {
        Purpose::EntityState state;
        state.type = Purpose::PACKET_ENTITY_UPDATE;
        state.networkID = p.id;
        state.lastProcessedTick = p.lastProcessedTick;
        state.posX = p.x;
        state.posY = p.y;
        state.posZ = p.z;
        state.rotationYaw = p.yaw;

        snapshot.push_back(state);
    }
    return snapshot;
}