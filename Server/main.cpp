#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main (int argc, char ** argv)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    bind(s, (struct sockaddr *)&server, sizeof(server));

    std::cout << "Server is listening on Port 8888..." << std::endl;

    char buffer[512];
    sockaddr_in client;
    int client_len = sizeof(client);

    recvfrom(s, buffer, 512, 0, (struct sockaddr *)&client, &client_len);

    std::cout << "Received Message: " << buffer << std::endl;

    closesocket(s);
    WSACleanup();
    return 0;
}
