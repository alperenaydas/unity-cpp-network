using System.Collections.Generic;
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
    
    public GameObject EntityPrefab;
    private Dictionary<uint, GameObject> _remoteEntities = new ();

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

        while (GetNextEntityUpdate(out EntityState update))
        {
            if (!_remoteEntities.ContainsKey(update.networkID))
            {
                SpawnEntity(update);
            }
            else
            {
                UpdateEntityPosition(update);
            }
        }
    }
    
    private void SpawnEntity(EntityState state)
    {
        var go = Instantiate(EntityPrefab);
        go.name = $"Entity_{state.networkID}";
        _remoteEntities.Add(state.networkID, go);
        
        UpdateEntityPosition(state);
        Debug.Log($"<color=green>Spawned Shadow Entity: {state.networkID}</color>");
    }

    private void UpdateEntityPosition(EntityState state)
    {
        if (_remoteEntities.TryGetValue(state.networkID, out GameObject go))
        {
            go.transform.position = new Vector3(state.posX, state.posY, state.posZ);
            // go.transform.rotation = Quaternion.Euler(state.rotX, state.rotY, state.rotZ);
        }
    }

    public int GetActiveEntityCount() => _remoteEntities.Count;

    private void OnApplicationQuit() => DisconnectFromServer();
}