#include <iostream>
#include <winsock2.h>
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Purpose::SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(Purpose::SERVER_IP);

    Purpose::HandshakePacket packet;
    sendto(s, (const char*)&packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    std::cout << "Message sent to Server!" << std::endl;

    closesocket(s);
    WSACleanup();
    return 0;
}