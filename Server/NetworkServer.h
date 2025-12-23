#pragma once
#include <enet/enet.h>
#include <cstdint>
#include "Protocol.h"

using PacketCallback = void(*)(ENetPeer* peer, uint16_t type, void* data);
using ConnectCallback = void(*)(ENetPeer* peer);
using DisconnectCallback = void(*)(ENetPeer* peer);

class NetworkServer {
public:
    explicit NetworkServer(uint16_t port);
    ~NetworkServer();

    bool Initialize();
    void PollEvents() const;
    void Broadcast(const Purpose::EntityState& state) const;
    void SendToPeer(ENetPeer* peer, const void* data, size_t size, bool reliable);

    void Flush() const;

    void Shutdown();

    void SetConnectCallback(ConnectCallback cb) { onConnect = cb; }
    void SetDisconnectCallback(DisconnectCallback cb) { onDisconnect = cb; }
    void SetPacketCallback(PacketCallback cb) { onPacket = cb; }

private:
    ENetHost* serverHost = nullptr;
    uint16_t port;
    
    ConnectCallback onConnect = nullptr;
    DisconnectCallback onDisconnect = nullptr;
    PacketCallback onPacket = nullptr;
};