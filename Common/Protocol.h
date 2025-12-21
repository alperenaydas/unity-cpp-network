#pragma once
#include <cstdint>

#pragma pack(push, 1)
namespace Purpose {
    constexpr uint16_t SERVER_PORT = 8888;
    inline const char* SERVER_IP = "127.0.0.1";

    enum Channels : uint8_t {
        CHANNEL_RELIABLE = 0,
        CHANNEL_UNRELIABLE = 1,
        CHANNEL_COUNT
    };

    enum PacketType : uint16_t {
        PACKET_WELCOME = 1,
        PACKET_ENTITY_UPDATE = 2,
        PACKET_ENTITY_DESPAWN = 3,
    };

    struct WelcomePacket {
        uint16_t type = PACKET_WELCOME;
        uint32_t playerID;
        float spawnX, spawnY, spawnZ;
    };

    struct EntityState {
        uint16_t type = PACKET_ENTITY_UPDATE;
        uint32_t networkID;
        float posX, posY, posZ;
        float rotX, rotY, rotZ;
    };

    struct EntityDespawn {
        uint16_t type = PACKET_ENTITY_DESPAWN;
        uint32_t networkID;
    };

}
#pragma pack(pop)