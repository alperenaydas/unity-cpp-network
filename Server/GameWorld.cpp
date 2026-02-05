#include "GameWorld.h"
#include "NetworkServer.h"
#include "BitStream.h"
#include <cmath>
#include <iostream>
#include <cstdlib>

static WorldSnapshot* GetSnapshot(Player& p, uint32_t tick) {
    for (auto& s : p.positionHistory) if (s.tick == tick) return &s;
    return nullptr;
}

void GameWorld::OnClientConnect(ENetPeer* peer, NetworkServer* server) {
    uint32_t id = nextID++;
    Player& p = players[id];
    p.id = id;
    p.peer = peer;

    SpawnPlayer(p);

    peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));

    Purpose::WelcomePacket w;
    w.playerID = id;
    w.spawnX = p.x; w.spawnY = p.y; w.spawnZ = p.z;
    server->SendToPeer(peer, &w, sizeof(w), true);
}

void GameWorld::OnClientDisconnect(ENetPeer* peer, NetworkServer* server) {
    if (!peer->data) return;
    auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));

    if (players.count(id)) {
        Player& p = players[id];

        std::vector<uint32_t> witnesses;
        grid.GetRelevantEntities(p.x, p.z, witnesses);

        Purpose::EntityDespawn d;
        d.networkID = id;

        for (uint32_t wid : witnesses) {
            if (wid == id) continue;
            if (players.count(wid) && !players[wid].isSpectator) {
                server->SendToPeer(players[wid].peer, &d, sizeof(d), true);
            }
        }

        for (auto& [pid, player] : players) {
            if (player.isSpectator) {
                server->SendToPeer(player.peer, &d, sizeof(d), true);
            }
        }

        grid.RemoveEntity(id);
        players.erase(id);
        std::cout << "[Game] Player " << id << " disconnected." << std::endl;
    }
}

void GameWorld::OnPacketReceived(ENetPeer* peer, uint16_t type, void* data, size_t length, NetworkServer* server) {
    if (!peer->data) return;

    auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));

    if (length < sizeof(Purpose::ClientInput)) {
        std::cerr << "[Security] Malformed Input Packet from " << id << std::endl;
        return;
    }

    auto it = players.find(id);
    if (it == players.end()) return;

    if (type == Purpose::PACKET_CLIENT_INPUT) {
        auto* in = static_cast<Purpose::ClientInput*>(data);
        if (in->tick > it->second.lastProcessedTick) it->second.inputQueue.push_back(*in);
    }
    else if (type == Purpose::PACKET_CLIENT_ACK) {
        if (length < sizeof(Purpose::ClientAck)) {
            return;
        }
        auto* ack = static_cast<Purpose::ClientAck*>(data);
        if (ack->tick > it->second.lastAckedTick) {
            it->second.lastAckedTick = ack->tick;
        }
    }
    else if (type == Purpose::PACKET_CLIENT_SPECTATOR) {
        it->second.isSpectator = true;
        it->second.isAlive = false;

        std::vector<uint32_t> witnesses;
        grid.GetRelevantEntities(it->second.x, it->second.z, witnesses);

        Purpose::EntityDespawn d;
        d.networkID = id;

        for (uint32_t wid : witnesses) {
            if (wid == id) continue;
            if (players.count(wid) && !players[wid].isSpectator) {
                server->SendToPeer(players[wid].peer, &d, sizeof(d), true);
            }
        }

        for (auto& [pid, player] : players) {
            if (player.isSpectator && pid != id) {
                server->SendToPeer(player.peer, &d, sizeof(d), true);
            }
        }

        grid.RemoveEntity(id);
        std::cout << "[Game] Player " << id << " became a spectator." << std::endl;
    }
}

void GameWorld::UpdatePhysics(float deltaTime, NetworkServer* server) {
    currentServerTick++;

    for (auto& [id, p] : players) {
        if (p.isSpectator) continue;

        if (!p.isAlive) {
            p.respawnTimer -= deltaTime;
            if (p.respawnTimer <= 0) {
                SpawnPlayer(p);
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
                p.x += (mx / mag) * Purpose::MOVE_SPEED * deltaTime;
                p.z += (mz / mag) * Purpose::MOVE_SPEED * deltaTime;
            }
            p.yaw = in.mouseYaw;
            p.lastProcessedTick = in.tick;
            if (in.fire) ProcessFire(p.id, p.yaw, server);
        }

        grid.UpdateEntity(id, p.x, p.z);
        p.SaveHistory(currentServerTick);
    }
}

void GameWorld::BroadcastWorldState(NetworkServer* server) {
    static std::vector<uint32_t> nearbyEntities;

    int sisGroupIndex = currentServerTick % Purpose::SIS_GROUPS;

    for (auto& [recipID, recipient] : players) {
        nearbyEntities.clear();

        if (recipient.isSpectator) {
            for (const auto& [id, p] : players) {
                if (p.isSpectator) continue;
                nearbyEntities.push_back(id);
            }
        }
        else {
            grid.GetRelevantEntities(recipient.x, recipient.z, nearbyEntities);
        }

        uint8_t stackBuffer[Purpose::MTU_SIZE];
        BitWriter writer(stackBuffer, Purpose::MTU_SIZE);
        writer.WriteBits(Purpose::PACKET_WORLD_STATE & 0xFF, 8);
        writer.WriteBits((Purpose::PACKET_WORLD_STATE >> 8) & 0xFF, 8);
        writer.WriteBits(currentServerTick, 32);
        writer.WriteBits(recipient.lastAckedTick, 32);

        uint32_t count = 0;
        for (uint32_t targetID : nearbyEntities) {
            if (players.find(targetID) == players.end()) continue;
            if (players[targetID].isSpectator) continue;

            if (recipient.isSpectator) {
                if (targetID % Purpose::SIS_GROUPS != sisGroupIndex) continue;
            }

            count++;
        }

        writer.WriteBits(count, 10);

        WorldSnapshot* baseline = GetSnapshot(recipient, recipient.lastAckedTick);

        for (uint32_t targetID : nearbyEntities) {
            if (players.find(targetID) == players.end()) continue;

            Player& target = players[targetID];
            if (target.isSpectator) continue;

            if (recipient.isSpectator) {
                if (targetID % Purpose::SIS_GROUPS != sisGroupIndex) continue;
            }

            writer.WriteBits(target.id, 32);

            int32_t curQX = static_cast<int32_t>(target.x * Purpose::QUANT_RES);
            int32_t curQZ = static_cast<int32_t>(target.z * Purpose::QUANT_RES);

            bool moved = true;

            writer.WriteBit(moved);
            if (moved) {
                writer.WriteInt(curQX, 32);
                writer.WriteInt(curQZ, 32);
            }
            writer.WriteFloat(target.yaw);
        }

        writer.WriteAlign();
        if (!writer.IsValid()) {
            std::cerr << "[Network] CRITICAL: Packet overflow! Dropping update." << std::endl;
            return;
        }
        server->SendToPeer(recipient.peer, writer.GetData(), writer.GetByteLength(), false);
    }
}

void GameWorld::ProcessFire(uint32_t shooterID, float yaw, NetworkServer* server) {
    auto it = players.find(shooterID);
    if (it == players.end() || !it->second.isAlive) return;

    Player& shooter = it->second;

    float rttTicks = (static_cast<float>(shooter.peer->roundTripTime) / 1000.0f) * Purpose::TICK_RATE;
    double renderTick = static_cast<double>(currentServerTick) - (5.0 + (rttTicks / 2.0));
    if (renderTick < 0) renderTick = 0;

    float rad = yaw * (Purpose::PI / 180.0f);
    Ray ray = { shooter.x, shooter.z, sinf(rad), cosf(rad) };

    uint32_t hitID = 0;
    float closest = 100.0f;
    float rewoundX = 0, rewoundZ = 0;

    for (auto& [id, target] : players) {
        if (id == shooterID || !target.isAlive || target.isSpectator) continue;

        float tx, tz;
        if (GetPositionAtTick(target, renderTick, tx, tz)) {
            float d;
            if (RayIntersectsCircle(ray, tx, tz, Purpose::PLAYER_RADIUS, d) && d < closest) {
                closest = d;
                hitID = id;
                rewoundX = tx;
                rewoundZ = tz;
            }
        }
    }

    if (hitID != 0) {
        // players[hitID].isAlive = false;
        // players[hitID].respawnTimer = 2.0f;
        // players[hitID].x = -9999.0f;
        //
        // std::cout << "[Game] Player " << shooterID << " HIT " << hitID << std::endl;
        //
        // std::vector<uint32_t> witnesses;
        // grid.GetRelevantEntities(rewoundX, rewoundZ, witnesses);
        //
        // Purpose::EntityDespawn d;
        // d.networkID = hitID;
        //
        // for (uint32_t witnessID : witnesses) {
        //     if (witnessID == hitID) continue;
        //     if (players.count(witnessID) && !players[witnessID].isSpectator) {
        //         server->SendToPeer(players[witnessID].peer, &d, sizeof(d), true);
        //     }
        // }
        //
        // BitWriter writer(64);
        // writer.WriteBits(Purpose::PACKET_DEBUG_HIT & 0xFF, 8);
        // writer.WriteBits((Purpose::PACKET_DEBUG_HIT >> 8) & 0xFF, 8);
        // writer.WriteFloat(rewoundX);
        // writer.WriteFloat(rewoundZ);
        // writer.WriteAlign();
        //
        // for (auto& [id, player] : players) {
        //     if (player.isSpectator) {
        //         server->SendToPeer(player.peer, writer.GetData(), writer.GetByteLength(), true);
        //         server->SendToPeer(player.peer, &d, sizeof(d), true);
        //     }
        // }
        //
        // grid.RemoveEntity(hitID);
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

void GameWorld::SpawnPlayer(Player& p) {
    p.isAlive = true;
    p.respawnTimer = 0.0f;

    float range = Purpose::SPAWN_RANGE;
    float rX = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (2.0f * range) - range;
    float rZ = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (2.0f * range) - range;

    p.x = rX;
    p.y = 0.0f;
    p.z = rZ;

    grid.UpdateEntity(p.id, p.x, p.z);

    std::cout << "[Game] Player " << p.id << " spawned at " << p.x << ", " << p.z << std::endl;
}