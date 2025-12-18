#include <cstdint>

namespace Purpose {

    constexpr std::uint16_t SERVER_PORT = 8888;
    const char* SERVER_IP = "127.0.0.1";


    struct HandshakePacket {
        uint32_t magicNumber = htonl(0x50555250);;
        uint32_t protocolVersion = 1;
    };
}
