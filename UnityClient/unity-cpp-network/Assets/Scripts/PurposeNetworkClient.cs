using UnityEngine;
using System.Runtime.InteropServices;

public class PurposeNetworkClient : MonoBehaviour
{
    private const string DLL_NAME = "PurposeClient";
    
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern bool ConnectToServer();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern void ServiceNetwork();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DisconnectFromServer();

    void Start()
    {
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