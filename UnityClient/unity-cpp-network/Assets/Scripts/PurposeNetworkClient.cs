using System;
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
    [DllImport(DLL_NAME)] private static extern uint GetNextDespawnID();
    [DllImport(DLL_NAME)] private static extern void SendMovementInput(bool w, bool a, bool s, bool d);
    
    public GameObject EntityPrefab;
    private Dictionary<uint, GameObject> _remoteEntities = new ();

    private LogDelegate _logHandler;
    private uint _myID = 0;

    private bool _connectedToServer;
    
    private bool _lastW, _lastA, _lastS, _lastD;
    
    private float _nextHeartbeatTime = 0f;
    private const float HEARTBEAT_RATE = 0.1f;

    void Start() {
        _logHandler = (msg) => Debug.Log($"<color=cyan>[Native]</color> {msg}");
        RegisterLogCallback(_logHandler);
        _connectedToServer = ConnectToServer();
        if (!_connectedToServer) Debug.LogError("Purpose: Connection Failed.");
    }

    void Update()
    {
        if (!_connectedToServer) return;
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
        
        uint idToDestroy;
        while ((idToDestroy = GetNextDespawnID()) != 0) {
            if (_remoteEntities.TryGetValue(idToDestroy, out GameObject go)) {
                Destroy(go);
                _remoteEntities.Remove(idToDestroy);
                Debug.Log($"<color=red>Deleted Entity: {idToDestroy}</color>");
            }
        }

        var w = Input.GetKey(KeyCode.W);
        var a = Input.GetKey(KeyCode.A);
        var s = Input.GetKey(KeyCode.S);
        var d = Input.GetKey(KeyCode.D);
        
        var changed = (w != _lastW || a != _lastA || s != _lastS || d != _lastD);
    
        if (changed || Time.time >= _nextHeartbeatTime) {
            SendMovementInput(w, a, s, d);
            _lastW = w; _lastA = a; _lastS = s; _lastD = d;
            _nextHeartbeatTime = Time.time + HEARTBEAT_RATE;
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
        if (_remoteEntities.TryGetValue(state.networkID, out var go))
        {
            go.transform.position = new Vector3(state.posX, state.posY, state.posZ);
            // go.transform.rotation = Quaternion.Euler(state.rotX, state.rotY, state.rotZ);
        }
    }

    public int GetActiveEntityCount() => _remoteEntities.Count;

    private void OnApplicationQuit() => DisconnectFromServer();
}