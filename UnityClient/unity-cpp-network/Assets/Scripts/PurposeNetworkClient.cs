using UnityEngine;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct EntityState {
    public ushort type;
    public uint networkID;
    public float posX, posY, posZ;
    public float rotX, rotY, rotZ;
}

public class PurposeNetworkClient : MonoBehaviour {
    private const string DLL_NAME = "PurposeClient";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogDelegate([MarshalAs(UnmanagedType.LPStr)] string message);

    [DllImport(DLL_NAME)] private static extern void RegisterLogCallback(LogDelegate callback);
    [DllImport(DLL_NAME)] private static extern bool ConnectToServer();
    [DllImport(DLL_NAME)] private static extern void ServiceNetwork();
    [DllImport(DLL_NAME)] private static extern void DisconnectFromServer();
    [DllImport(DLL_NAME)] private static extern uint GetAssignedPlayerID();
    [DllImport(DLL_NAME)] private static extern bool GetNextEntityUpdate(out EntityState outState);

    private LogDelegate _logHandler;
    private uint _myID = 0;

    void Start() {
        _logHandler = (msg) => Debug.Log($"<color=cyan>[Native]</color> {msg}");
        RegisterLogCallback(_logHandler);
        
        if (!ConnectToServer()) Debug.LogError("Purpose: Connection Failed.");
    }

    void Update() {
        ServiceNetwork();

        uint serverID = GetAssignedPlayerID();
        if (_myID != serverID && serverID != 0) {
            _myID = serverID;
            Debug.Log($"<color=yellow>Purpose: Assigned ID is {_myID}</color>");
        }

        while (GetNextEntityUpdate(out EntityState update)) {
            Debug.Log($"<color=orange>Purpose: ReceivedUpdateData Id: {update.networkID}, Pos: {update.posX} {update.posY} {update.posZ}</color>");
        }
    }

    private void OnApplicationQuit() => DisconnectFromServer();
}