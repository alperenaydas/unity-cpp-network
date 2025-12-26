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
        if (g_Client) {
            g_Client->Disconnect();
            g_Client.reset();
        }
    }

    EXPORT_API void ServiceNetwork() {
        if (g_Client) g_Client->ServiceNetwork();
    }

    EXPORT_API uint32_t GetAssignedPlayerID() {
        return g_Client ? g_Client->GetAssignedID() : 0;
    }

    EXPORT_API bool GetNextEntityUpdate(Purpose::EntityData* outData) {
        if (g_Client && outData) {
            return g_Client->PopEntityData(*outData);
        }
        return false;
    }

    EXPORT_API uint32_t GetNextDespawnID() {
        return g_Client ? g_Client->PopDespawnID() : 0;
    }

    EXPORT_API void SendMovementInput(uint32_t tick, bool w, bool a, bool s, bool d, bool fire, float yaw) {
        if (g_Client) {
            g_Client->SendInput(tick, w, a, s, d, fire, yaw);
        }
    }

    EXPORT_API void GetNetworkMetrics(Purpose::NetworkMetrics* outMetrics) {
        if (g_Client && outMetrics) {
            *outMetrics = g_Client->GetMetrics();
        }
    }
}