#pragma once
#include <cstdint>

namespace Purpose {
#pragma pack(push, 1)
    constexpr uint16_t SERVER_PORT = 8888;
    inline const char* SERVER_IP = "127.0.0.1";
    constexpr int MAX_ENTITIES_PER_PACKET = 128;

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
    };

    struct WelcomePacket {
        uint16_t type = PACKET_WELCOME;
        uint32_t playerID;
        float spawnX, spawnY, spawnZ;
    };

    struct EntityData {
        uint32_t networkID;
        uint32_t lastProcessedTick;
        float posX, posY, posZ;
        float rotationYaw;
    };

    struct WorldStatePacket {
        uint16_t type = PACKET_WORLD_STATE;
        uint32_t entityCount;
        EntityData entities[MAX_ENTITIES_PER_PACKET];
    };

    struct EntityDespawn {
        uint16_t type = PACKET_ENTITY_DESPAWN;
        uint32_t networkID;
    };

    struct ClientInput {
        uint16_t type = PACKET_CLIENT_INPUT;
        uint32_t tick;
        uint8_t w, a, s, d, fire;
        float mouseYaw;
    };

    struct NetworkMetrics {
        uint32_t ping;
        uint32_t packetLoss;
        uint64_t totalBytesSent;
        uint64_t totalBytesReceived;
        float incomingBandwidth;
        float outgoingBandwidth;
    };
#pragma pack(pop)
}
