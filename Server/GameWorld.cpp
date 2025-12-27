#include "GameWorld.h"
#include "NetworkServer.h"
#include "BitStream.h"
#include <cmath>
#include <iostream>
#include <cstdlib>

static float GetRandomCoord() {
    return -50.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100.0f));
}

static WorldSnapshot* GetSnapshot(Player& p, uint32_t tick) {
    for (auto& s : p.positionHistory) if (s.tick == tick) return &s;
    return nullptr;
}

void GameWorld::OnClientConnect(ENetPeer* peer, NetworkServer* server) {
    uint32_t id = nextID++;
    Player& p = players[id];
    p.id = id;
    p.peer = peer;

    p.x = GetRandomCoord();
    p.y = 0;
    p.z = GetRandomCoord();

    peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));

    std::cout << "[Game] Player " << id << " joined from "
              << peer->address.host << ":" << peer->address.port << std::endl;

    Purpose::WelcomePacket w;
    w.playerID = id;
    w.spawnX = p.x; w.spawnY = p.y; w.spawnZ = p.z;
    server->SendToPeer(peer, &w, sizeof(w), true);
}

void GameWorld::OnClientDisconnect(ENetPeer* peer, NetworkServer* server) {
    if (!peer->data) return;
    auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));

    if (players.erase(id)) {
        std::cout << "[Game] Player " << id << " disconnected." << std::endl;

        Purpose::EntityDespawn d;
        d.networkID = id;
        server->Broadcast(&d, sizeof(d), true);
    }
}

void GameWorld::OnPacketReceived(ENetPeer* peer, uint16_t type, void* data) {
    if (!peer->data) return;
    auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
    auto it = players.find(id);
    if (it == players.end()) return;

    if (type == Purpose::PACKET_CLIENT_INPUT) {
        auto* in = static_cast<Purpose::ClientInput*>(data);
        if (in->tick > it->second.lastProcessedTick) it->second.inputQueue.push_back(*in);
    }
    else if (type == Purpose::PACKET_CLIENT_ACK) {
        auto* ack = static_cast<Purpose::ClientAck*>(data);
        if (ack->tick > it->second.lastAckedTick) {
            it->second.lastAckedTick = ack->tick;
        }
    }
}

void GameWorld::UpdatePhysics(float deltaTime) {
    currentServerTick++;

    for (auto& [id, p] : players) {
        if (!p.isAlive) {
            p.respawnTimer -= deltaTime;
            if (p.respawnTimer <= 0) {
                p.isAlive = true;
                p.x = GetRandomCoord();
                p.z = GetRandomCoord();
                std::cout << "[Game] Player " << id << " respawned at " << p.x << ", " << p.z << std::endl;
            }
            p.inputQueue.clear();
            p.SaveHistory(currentServerTick);
            continue;
        }

        while (!p.inputQueue.empty()) {
            Purpose::ClientInput in = p.inputQueue.front();
            p.inputQueue.pop_front();

            float mx = 0, mz = 0;
            if (in.w) mz += 1.0f; if (in.s) mz -= 1.0f;
            if (in.a) mx -= 1.0f; if (in.d) mx += 1.0f;

            float mag = sqrtf(mx * mx + mz * mz);
            if (mag > 0) {
                p.x += (mx / mag) * MOVE_SPEED * deltaTime;
                p.z += (mz / mag) * MOVE_SPEED * deltaTime;
            }
            p.yaw = in.mouseYaw;
            p.lastProcessedTick = in.tick;
            if (in.fire) ProcessFire(p.id, p.yaw);
        }
        p.SaveHistory(currentServerTick);
    }
}

void GameWorld::BroadcastWorldState(NetworkServer* server) {
    for (auto& [recipID, recipient] : players) {
        BitWriter writer(1400);

        writer.WriteBits(Purpose::PACKET_WORLD_STATE & 0xFF, 8);
        writer.WriteBits((Purpose::PACKET_WORLD_STATE >> 8) & 0xFF, 8);

        writer.WriteBits(currentServerTick, 32);
        writer.WriteBits(recipient.lastAckedTick, 32);
        writer.WriteBits(static_cast<uint32_t>(players.size()), 8);

        WorldSnapshot* baseline = GetSnapshot(recipient, recipient.lastAckedTick);

        for (auto& [targetID, target] : players) {
            writer.WriteBits(target.id, 32);

            int32_t curQX = static_cast<int32_t>(target.x * Purpose::QUANT_RES);
            int32_t curQZ = static_cast<int32_t>(target.z * Purpose::QUANT_RES);

            bool moved = true;
            if (baseline && targetID == recipID) {
                moved = (curQX != baseline->qx || curQZ != baseline->qz);
            }

            writer.WriteBit(moved);
            if (moved) {
                writer.WriteInt(curQX, 32);
                writer.WriteInt(curQZ, 32);
            }
            writer.WriteFloat(target.yaw);
        }
        writer.WriteAlign();
        server->SendToPeer(recipient.peer, writer.GetData(), writer.GetByteLength(), false);
    }
}

void GameWorld::ProcessFire(uint32_t shooterID, float yaw) {
    auto it = players.find(shooterID);
    if (it == players.end() || !it->second.isAlive) return;

    Player& shooter = it->second;
    float rttTicks = (static_cast<float>(shooter.peer->roundTripTime) / 1000.0f) * 50.0f;
    double renderTick = static_cast<double>(currentServerTick) - (5.0 + (rttTicks / 2.0));
    if (renderTick < 0) renderTick = 0;

    float rad = yaw * (3.14159f / 180.0f);
    Ray ray = { shooter.x, shooter.z, sinf(rad), cosf(rad) };

    uint32_t hitID = 0;
    float closest = 100.0f;

    for (auto& [id, target] : players) {
        if (id == shooterID || !target.isAlive) continue;
        float tx, tz;
        if (GetPositionAtTick(target, renderTick, tx, tz)) {
            float d;
            if (RayIntersectsCircle(ray, tx, tz, 0.5f, d) && d < closest) {
                closest = d; hitID = id;
            }
        }
    }

    if (hitID != 0) {
        std::cout << "[Game] Player " << shooterID << " HIT " << hitID << std::endl;
        players[hitID].isAlive = false;
        players[hitID].respawnTimer = 2.0f;
        players[hitID].x = -9999.0f;

        Purpose::EntityDespawn d;
        d.networkID = hitID;
    }
}

bool GameWorld::GetPositionAtTick(const Player& t, double tick, float& ox, float& oz) {
    if (t.positionHistory.size() < 2) return false;
    for (size_t i = 0; i < t.positionHistory.size() - 1; i++) {
        auto& n = t.positionHistory[i];
        auto& o = t.positionHistory[i + 1];
        if (n.tick >= tick && o.tick <= tick) {
            double a = (tick - static_cast<double>(o.tick)) / (static_cast<double>(n.tick) - static_cast<double>(o.tick));
            ox = (static_cast<float>(o.qx) / Purpose::QUANT_RES) + ((static_cast<float>(n.qx) / Purpose::QUANT_RES) - (static_cast<float>(o.qx) / Purpose::QUANT_RES)) * static_cast<float>(a);
            oz = (static_cast<float>(o.qz) / Purpose::QUANT_RES) + ((static_cast<float>(n.qz) / Purpose::QUANT_RES) - (static_cast<float>(o.qz) / Purpose::QUANT_RES)) * static_cast<float>(a);
            return true;
        }
    }
    return false;
}

bool GameWorld::RayIntersectsCircle(Ray r, float cx, float cz, float rad, float& dist) {
    float ox = cx - r.originX, oz = cz - r.originZ;
    float dot = ox * r.dirX + oz * r.dirZ;
    if (dot < 0) return false;
    float d2 = (ox * ox + oz * oz) - (dot * dot);
    if (d2 > rad * rad) return false;
    dist = dot; return true;
}