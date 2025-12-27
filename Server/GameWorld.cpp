#include "GameWorld.h"
#include "NetworkServer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

void GameWorld::OnClientConnect(ENetPeer* peer, NetworkServer* server) {
    const uint32_t newID = nextID++;

    Player newPlayer;
    newPlayer.peer = peer;
    newPlayer.id = newID;
    newPlayer.x = 0;
    newPlayer.y = 0;
    newPlayer.z = 10;

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
    currentServerTick++;

    for (auto& [id, player] : players) {
        if (!player.isAlive) {
            player.respawnTimer -= deltaTime;
            if (player.respawnTimer <= 0) {
                player.isAlive = true;
                player.x = 0;
                player.z = 10;
                std::cout << "[Game] Player " << id << " respawned." << std::endl;
            }
            player.inputQueue.clear();
            player.SaveHistory(currentServerTick);
            continue;
        }

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
        }

        player.SaveHistory(currentServerTick);
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
    if (it == players.end() || !it->second.isAlive) return;

    Player& shooter = it->second;

    float rttMs = (float)shooter.peer->roundTripTime;
    float rttTicks = (rttMs / 1000.0f) * 50.0f;
    float interpDelayTicks = 5.0f;

    double renderTick = (double)currentServerTick - (interpDelayTicks + (rttTicks / 2.0));

    if (renderTick < 0) renderTick = 0;

    float rad = yaw * (3.14159f / 180.0f);
    Ray ray = { shooter.x, shooter.z, sinf(rad), cosf(rad) };

    uint32_t hitID = 0;
    float closestDist = 100.0f;

    for (auto& [id, target] : players) {
        if (id == shooterID || !target.isAlive) continue;

        GetPositionAtTick(target, renderTick, target.x, target.z);

        float dist;
        if (RayIntersectsCircle(ray, target.x, target.z, 0.5f, dist)) {
            if (dist < closestDist) {
                closestDist = dist;
                hitID = id;
            }
        }
    }

    if (hitID != 0) {
        printf("[Server] Player %u HIT Player %u (Rewind: %.2f | Curr: %u)\n",
               shooterID, hitID, renderTick, currentServerTick);

        Player& victim = players[hitID];
        victim.isAlive = false;
        victim.respawnTimer = 2.0f;
        victim.x = -9999.0f;
        victim.z = -9999.0f;
    }
}

bool GameWorld::GetPositionAtTick(const Player& target, double renderTick, float& outX, float& outZ) {
    if (target.positionHistory.size() < 2) return false;

    for (size_t i = 0; i < target.positionHistory.size() - 1; i++) {
        const auto& newer = target.positionHistory[i];
        const auto& older = target.positionHistory[i + 1];

        if (newer.tick >= renderTick && older.tick <= renderTick) {
            double delta = static_cast<double>(newer.tick) - static_cast<double>(older.tick);
            if (delta <= 0.0001) return false;

            double alpha = (renderTick - static_cast<double>(older.tick)) / delta;

            outX = older.x + (newer.x - older.x) * static_cast<float>(alpha);
            outZ = older.z + (newer.z - older.z) * static_cast<float>(alpha);
            return true;
        }
    }
    return false;
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