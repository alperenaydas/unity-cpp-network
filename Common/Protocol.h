#pragma once
#include <cstdint>

namespace Purpose {
    constexpr uint16_t SERVER_PORT = 8888;
    inline const char *SERVER_IP = "127.0.0.1";
    constexpr int MTU_SIZE = 1400;
    constexpr int MAX_CLIENTS = 1024;

    constexpr float TICK_RATE = 50.0f;
    constexpr float TICK_DELTA = 1.0f / TICK_RATE;

    constexpr float SPAWN_RANGE = 45.0f;
    constexpr float MOVE_SPEED = 10.0f;
    constexpr float PLAYER_RADIUS = 0.5f;
    constexpr float QUANT_RES = 100.0f;

    constexpr int SIS_GROUPS = 4;

    constexpr float PI = 3.14159265359f;

    constexpr int HARD_CAP_INPUT_QUEUE = 128;
    constexpr int SOFT_CAP_INPUT_QUEUE = 16;

#pragma pack(push, 1)

    enum Channels : uint8_t {
        CHANNEL_RELIABLE = 0,
        CHANNEL_UNRELIABLE = 1,
        CHANNEL_COUNT
    };

    enum PacketType : uint16_t {
        PACKET_WELCOME = 1,
        PACKET_WORLD_STATE = 2,
        PACKET_ENTITY_DESPAWN = 3,
        PACKET_CLIENT_INPUT = 4,
        PACKET_CLIENT_ACK = 5,
        PACKET_DEBUG_HIT = 6,
        PACKET_CLIENT_SPECTATOR = 7
    };

    struct NetworkMetrics {
        uint32_t ping;
        uint32_t packetLoss;
        uint64_t totalBytesSent;
        uint64_t totalBytesReceived;
        float incomingBandwidth; // KBps
        float outgoingBandwidth; // KBps
    };

    struct WelcomePacket {
        uint16_t type = PACKET_WELCOME;
        uint32_t playerID;
        float spawnX, spawnY, spawnZ;
    };

    struct EntityData {
        uint32_t networkID;
        uint32_t lastProcessedTick;
        int32_t qX, qY, qZ;
        float rotationYaw;
    };

    constexpr int MAX_ENTITIES_PER_PACKET = 128;

    struct WorldStatePacket {
        uint16_t type = PACKET_WORLD_STATE;
        uint32_t tick;
        uint32_t entityCount;
        EntityData entities[MAX_ENTITIES_PER_PACKET];
    };

    struct ClientAck {
        uint16_t type = PACKET_CLIENT_ACK;
        uint32_t tick;
    };

    struct ClientInput {
        uint16_t type = PACKET_CLIENT_INPUT;
        uint32_t tick;
        uint8_t w, a, s, d, fire;
        float mouseYaw;
    };

    struct EntityDespawn {
        uint16_t type = PACKET_ENTITY_DESPAWN;
        uint32_t networkID;
    };

    struct ClientRequestToBeSpectator {
        uint16_t type = PACKET_CLIENT_SPECTATOR;
        uint32_t tick;
    };
#pragma pack(pop)
}
