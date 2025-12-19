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
    
    private LogDelegate logHandler;

    void Start()
    {
        logHandler = (msg) => Debug.Log($"<color=cyan>[C++ Engine]</color> {msg}");
        RegisterLogCallback(logHandler);
        Debug.Log("Attempting to connect to Purpose Server...");
        if (ConnectToServer())
        {
            Debug.Log("<color=green>Purpose Engine Connected Successfully!</color>");
        }
        else
        {
            Debug.LogError("Failed to connect to Server. Is it running?");
        }
    }

    void Update()
    {
        ServiceNetwork();
    }

    private void OnApplicationQuit()
    {
        Debug.Log("Cleaning up Purpose Engine...");
        DisconnectFromServer();
    }
}