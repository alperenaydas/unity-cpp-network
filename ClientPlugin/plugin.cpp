#include "NetworkClient.h"
#include <memory>

#define EXPORT_API __declspec(dllexport)

static std::unique_ptr<NetworkClient> g_Client;

extern "C" {

    EXPORT_API void RegisterLogCallback(LogCallback callback) {
        if (!g_Client) g_Client = std::make_unique<NetworkClient>();
        g_Client->SetLogCallback(callback);
    }

    EXPORT_API bool ConnectToServer() {
        if (!g_Client) g_Client = std::make_unique<NetworkClient>();
        return g_Client->Connect(Purpose::SERVER_IP, Purpose::SERVER_PORT);
    }

    EXPORT_API void DisconnectFromServer() {
        if (g_Client) g_Client->Disconnect();
    }

    EXPORT_API void ServiceNetwork() {
        if (g_Client) g_Client->ServiceNetwork();
    }

    EXPORT_API uint32_t GetAssignedPlayerID() {
        return g_Client ? g_Client->GetAssignedID() : 0;
    }

    EXPORT_API bool GetNextEntityUpdate(Purpose::EntityState* outState) {
        return g_Client ? g_Client->PopEntityUpdate(*outState) : false;
    }

    EXPORT_API uint32_t GetNextDespawnID() {
        return g_Client ? g_Client->PopDespawnID() : 0;
    }

    EXPORT_API void SendMovementInput(uint32_t tick, bool w, bool a, bool s, bool d) {
        if (g_Client) g_Client->SendInput(tick, w, a, s, d);
    }
}