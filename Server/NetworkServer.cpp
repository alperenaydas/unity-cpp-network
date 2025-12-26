#include "NetworkServer.h"
#include <iostream>

NetworkServer::NetworkServer(uint16_t port) : port(port) {}

NetworkServer::~NetworkServer() {
    Shutdown();
}

bool NetworkServer::Initialize() {
    if (enet_initialize() != 0) {
        std::cerr << "[Network] Failed to initialize ENet." << std::endl;
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    serverHost = enet_host_create(&address, 1024, Purpose::CHANNEL_COUNT, 0, 0);
    if (!serverHost) {
        std::cerr << "[Network] Failed to create ENet host." << std::endl;
        return false;
    }

    std::cout << "[Network] Server started on port " << port << std::endl;
    return true;
}

void NetworkServer::PollEvents() {
    if (!serverHost) return;

    ENetEvent event;
    while (enet_host_service(serverHost, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (onConnect) onConnect(event.peer);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (onPacket && event.packet->dataLength >= sizeof(uint16_t)) {
                    uint16_t type = *reinterpret_cast<uint16_t*>(event.packet->data);
                    onPacket(event.peer, type, event.packet->data);
                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                if (onDisconnect) onDisconnect(event.peer);
                event.peer->data = nullptr;
                break;

            default:
                break;
        }
    }
}

void NetworkServer::Broadcast(const void* data, size_t size, bool reliable) {
    if (!serverHost) return;

    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
    ENetPacket* packet = enet_packet_create(data, size, flags);

    enet_host_broadcast(serverHost, reliable ? Purpose::CHANNEL_RELIABLE : Purpose::CHANNEL_UNRELIABLE, packet);
}

void NetworkServer::SendToPeer(ENetPeer* peer, const void* data, size_t size, bool reliable) {
    if (!peer) return;

    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
    ENetPacket* packet = enet_packet_create(data, size, flags);

    enet_peer_send(peer, reliable ? Purpose::CHANNEL_RELIABLE : Purpose::CHANNEL_UNRELIABLE, packet);
}

void NetworkServer::Flush() {
    if (serverHost) enet_host_flush(serverHost);
}

void NetworkServer::Shutdown() {
    if (serverHost) {
        enet_host_destroy(serverHost);
        serverHost = nullptr;
        enet_deinitialize();
    }
}