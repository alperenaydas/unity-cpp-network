using UnityEngine;
using System.Runtime.InteropServices;

public class PurposeNetworkClient : MonoBehaviour
{
    private const string DLL_NAME = "PurposeClient";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogDelegate([MarshalAs(UnmanagedType.LPStr)] string message);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern bool ConnectToServer();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern void ServiceNetwork();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DisconnectFromServer();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterLogCallback(LogDelegate callback);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern uint GetAssignedPlayerID();

    private LogDelegate logHandler;
    private uint myID = 0;

    void Start()
    {
        logHandler = (msg) => Debug.Log($"<color=cyan>[C++ Engine]</color> {msg}");
        RegisterLogCallback(logHandler);
        
        if (ConnectToServer()) {
            Debug.Log("<color=green>Purpose Engine: Connected!</color>");
        } else {
            Debug.LogError("Purpose Engine: Connection Failed.");
        }
    }

    void Update()
    {
        ServiceNetwork();

        uint currentID = GetAssignedPlayerID();
        if (currentID != myID && currentID != 0) {
            myID = currentID;
            Debug.Log($"<color=yellow>Unity: My Server ID is {myID}</color>");
        }
    }

    private void OnApplicationQuit()
    {
        DisconnectFromServer();
    }
}