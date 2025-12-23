using System;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct EntityState
{
    public ushort type;
    public uint networkID;
    public uint lastProcessedTick;
    public float posX, posY, posZ;
    public float rotX, rotY, rotZ;
}

public class PurposeNetworkClient : MonoBehaviour
{
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
    [DllImport(DLL_NAME)] private static extern void SendMovementInput(uint tick, bool w, bool a, bool s, bool d);
    
    public GameObject EntityPrefab;
    private Dictionary<uint, GameObject> _remoteEntities = new();

    private LogDelegate _logHandler;
    private uint _myID = 0;

    private bool _connectedToServer;

    private bool _currentW, _currentA, _currentS, _currentD;
    private bool _lastW, _lastA, _lastS, _lastD;

    private const int HEARTBEAT_INTERVAL_TICKS = 6;

    private uint _currentTick = 0;

    struct HistoryState
    {
        public uint tick;
        public Vector3 position;
        public bool w, a, s, d;
    }

    private List<HistoryState> _history = new();

    private const float RECONCILIATION_TOLERANCE = 0.05f;

    void Start()
    {
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
        if (_myID != serverID && serverID != 0)
        {
            _myID = serverID;
            Debug.Log($"<color=yellow>Purpose: Assigned ID is {_myID}</color>");
        }

        while (GetNextEntityUpdate(out EntityState update))
        {
            if (!_remoteEntities.ContainsKey(update.networkID))
            {
                SpawnEntity(update);
            }
            else if (update.networkID == _myID)
            {
                HandleServerReconciliation(update);
            }
            else
            {
                UpdateEntityPosition(update);
            }
        }

        uint idToDestroy;
        while ((idToDestroy = GetNextDespawnID()) != 0)
        {
            if (_remoteEntities.TryGetValue(idToDestroy, out GameObject go))
            {
                Destroy(go);
                _remoteEntities.Remove(idToDestroy);
                Debug.Log($"<color=red>Deleted Entity: {idToDestroy}</color>");
            }
        }

        _currentW = Input.GetKey(KeyCode.W);
        _currentA = Input.GetKey(KeyCode.A);
        _currentS = Input.GetKey(KeyCode.S);
        _currentD = Input.GetKey(KeyCode.D);
        
        if (Input.GetKeyDown(KeyCode.T))
        {
            if (_remoteEntities.TryGetValue(_myID, out var myGO))
            {
                myGO.transform.position += new Vector3(2.0f, 0, 0);
                Debug.Log("Forcing Desync!");
            }
        }
    }

    private void FixedUpdate()
    {
        if (!_connectedToServer) return;

        _currentTick++;

        // var changed = (_currentW != _lastW || _currentA != _lastA || _currentS != _lastS || _currentD != _lastD);
        //
        // if (changed || _currentTick % HEARTBEAT_INTERVAL_TICKS == 0)
        // {
        //     SendMovementInput(_currentTick, _currentW, _currentA, _currentS, _currentD);
        //     _lastW = _currentW;
        //     _lastA = _currentA;
        //     _lastS = _currentS;
        //     _lastD = _currentD;
        // }
        
        SendMovementInput(_currentTick, _currentW, _currentA, _currentS, _currentD);
        _lastW = _currentW;
        _lastA = _currentA;
        _lastS = _currentS;
        _lastD = _currentD;

        ProcessPredictionAndStore(_currentTick, _currentW, _currentA, _currentS, _currentD);
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
        if (state.networkID == _myID)
        {
            return; // we are already predicting movement.
        }
        if (_remoteEntities.TryGetValue(state.networkID, out var go))
        {
            go.transform.position = new Vector3(state.posX, state.posY, state.posZ);
            // go.transform.rotation = Quaternion.Euler(state.rotX, state.rotY, state.rotZ);
        }
    }

    private Vector3 SimulateMovement(Vector3 startPos, bool w, bool a, bool s, bool d, float dt)
    {
        float moveSpeed = 5.0f * dt;
        Vector3 pos = startPos;

        if (w) pos.z += moveSpeed;
        if (s) pos.z -= moveSpeed;
        if (a) pos.x -= moveSpeed;
        if (d) pos.x += moveSpeed;

        return pos;
    }

    private void ProcessPredictionAndStore(uint tick, bool w, bool a, bool s, bool d)
    {
        if (_remoteEntities.TryGetValue(_myID, out GameObject myGO))
        {
            var newPos = SimulateMovement(myGO.transform.position, w, a, s, d, Time.fixedDeltaTime);

            myGO.transform.position = newPos;

            _history.Add(new HistoryState
            {
                tick = tick,
                position = newPos,
                w = w, a = a, s = s, d = d
            });
        }
    }

    private void HandleServerReconciliation(EntityState serverState)
    {
        _history.RemoveAll(state => state.tick < serverState.lastProcessedTick);

        int historyIndex = _history.FindIndex(state => state.tick == serverState.lastProcessedTick);

        if (historyIndex != -1)
        {
            HistoryState historyState = _history[historyIndex];

            Vector3 serverPos = new Vector3(serverState.posX, serverState.posY, serverState.posZ);
            float error = Vector3.Distance(historyState.position, serverPos);

            if (error > RECONCILIATION_TOLERANCE)
            {
                Debug.LogWarning(
                    $"<color=orange>Reconciling! Error: {error:F4} at Tick {serverState.lastProcessedTick}</color>");

                if (_remoteEntities.TryGetValue(_myID, out GameObject myGO))
                {
                    myGO.transform.position = serverPos;

                    for (int i = historyIndex + 1; i < _history.Count; i++)
                    {
                        HistoryState oldState = _history[i];

                        Vector3 replayedPos = SimulateMovement(myGO.transform.position, oldState.w, oldState.a,
                            oldState.s, oldState.d, Time.fixedDeltaTime);

                        myGO.transform.position = replayedPos;

                        var updatedState = _history[i];
                        updatedState.position = replayedPos;
                        _history[i] = updatedState;
                    }
                }
            }
        }
    }

    public int GetActiveEntityCount() => _remoteEntities.Count;

    private void OnApplicationQuit() => DisconnectFromServer();
}