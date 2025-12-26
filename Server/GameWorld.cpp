#include "GameWorld.h"
#include "NetworkServer.h"
#include <cmath>
#include <iostream>
#include <algorithm>

void GameWorld::OnClientConnect(ENetPeer* peer, NetworkServer* server) {
    const uint32_t newID = nextID++;

    Player newPlayer;
    newPlayer.peer = peer;
    newPlayer.id = newID;
    newPlayer.x = static_cast<float>(rand() % 100 - 50);
    newPlayer.y = 0;
    newPlayer.z = static_cast<float>(rand() % 100 - 50);

    players[newID] = newPlayer;

    peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(newID));

    std::cout << "[Game] Player " << newID << " joined from "
              << peer->address.host << ":" << peer->address.port << std::endl;

    Purpose::WelcomePacket welcome;
    welcome.playerID = newID;
    welcome.spawnX = newPlayer.x; welcome.spawnY = newPlayer.y; welcome.spawnZ = newPlayer.z;
    server->SendToPeer(peer, &welcome, sizeof(welcome), true);
}

void GameWorld::OnClientDisconnect(ENetPeer* peer, NetworkServer* server) {
    if (!peer->data) return;
    uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));

    auto it = players.find(id);
    if (it != players.end()) {
        Purpose::EntityDespawn despawn;
        despawn.networkID = id;
        server->Broadcast(&despawn, sizeof(despawn), true);

        players.erase(it);
        std::cout << "[Game] Player " << id << " cleaned up." << std::endl;
    }
}

void GameWorld::OnPacketReceived(ENetPeer* peer, const uint16_t type, void* data) {
    if (!peer->data) return;
    uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));

    if (type == Purpose::PACKET_CLIENT_INPUT) {
        auto* input = static_cast<Purpose::ClientInput*>(data);
        auto it = players.find(id);
        if (it != players.end()) {
            Player& p = it->second;
            if (input->tick > p.lastProcessedTick) {
                p.inputQueue.push_back(*input);
            }
        }
    }
}

void GameWorld::UpdatePhysics(const float deltaTime) {
    for (auto& [id, player] : players) {
        while (!player.inputQueue.empty()) {
            player.lastInput = player.inputQueue.front();
            player.inputQueue.pop_front();

            float moveX = 0, moveZ = 0;
            if (player.lastInput.w) moveZ += 1.0f;
            if (player.lastInput.s) moveZ -= 1.0f;
            if (player.lastInput.a) moveX -= 1.0f;
            if (player.lastInput.d) moveX += 1.0f;

            float mag = sqrtf(moveX * moveX + moveZ * moveZ);
            if (mag > 0) {
                player.x += (moveX / mag) * MOVE_SPEED * deltaTime;
                player.z += (moveZ / mag) * MOVE_SPEED * deltaTime;
            }

            player.yaw = player.lastInput.mouseYaw;
            player.lastProcessedTick = player.lastInput.tick;

            if (player.lastInput.fire) {
                ProcessFire(player.id, player.yaw);
            }

            player.SaveHistory(player.lastProcessedTick);
        }
    }
}

Purpose::WorldStatePacket GameWorld::GenerateWorldState() {
    Purpose::WorldStatePacket packet;
    packet.entityCount = 0;

    for (auto const& [id, p] : players) {
        if (packet.entityCount >= Purpose::MAX_ENTITIES_PER_PACKET) break;

        Purpose::EntityData& data = packet.entities[packet.entityCount];
        data.networkID = p.id;
        data.lastProcessedTick = p.lastProcessedTick;
        data.posX = p.x;
        data.posY = p.y;
        data.posZ = p.z;
        data.rotationYaw = p.yaw;

        packet.entityCount++;
    }
    return packet;
}

void GameWorld::ProcessFire(uint32_t shooterID, float yaw) {
    auto it = players.find(shooterID);
    if (it == players.end()) return;
    Player& shooter = it->second;

    float rad = yaw * (3.14159f / 180.0f);
    Ray ray = { shooter.x, shooter.z, sinf(rad), cosf(rad) };

    uint32_t hitID = 0;
    float closestDist = 100.0f;

    for (auto& [id, target] : players) {
        if (id == shooterID) continue;
        if (id == 1001) continue;

        float dist;
        if (RayIntersectsCircle(ray, target.x, target.z, 0.5f, dist)) {
            if (dist < closestDist) {
                closestDist = dist;
                hitID = id;
            }
        }
    }

    if (hitID != 0) {
        printf("[Server] Player %u KILLED Player %u!\n", shooterID, hitID);
        enet_peer_disconnect(players[hitID].peer, 0);
    }
}

bool GameWorld::RayIntersectsCircle(Ray ray, float cx, float cz, float radius, float& distance) {
    float ocX = cx - ray.originX;
    float ocZ = cz - ray.originZ;
    float dot = ocX * ray.dirX + ocZ * ray.dirZ;
    if (dot < 0) return false;
    float distSq = (ocX * ocX + ocZ * ocZ) - (dot * dot);
    if (distSq > radius * radius) return false;
    distance = dot;
    return true;
}