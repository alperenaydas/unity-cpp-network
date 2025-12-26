#pragma once
#include <enet/enet.h>
#include <cstdint>
#include <string>
#include "Protocol.h"

using PacketCallback = void(*)(ENetPeer* peer, uint16_t type, void* data);
using ConnectCallback = void(*)(ENetPeer* peer);
using DisconnectCallback = void(*)(ENetPeer* peer);

class NetworkServer {
public:
    explicit NetworkServer(uint16_t port = Purpose::SERVER_PORT);
    ~NetworkServer();

    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;

    bool Initialize();
    void Shutdown();

    void PollEvents();
    void Flush();

    void Broadcast(const void* data, size_t size, bool reliable = false);
    void SendToPeer(ENetPeer* peer, const void* data, size_t size, bool reliable = true);

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