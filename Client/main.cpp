#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    const char* message = "Hello from the client!";
    sendto(s, message, strlen(message), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    std::cout << "Message sent to Server!" << std::endl;

    closesocket(s);
    WSACleanup();
    return 0;
}