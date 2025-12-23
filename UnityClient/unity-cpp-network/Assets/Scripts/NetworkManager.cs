using System.Collections.Generic;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    public GameObject EntityPrefab;
    
    private Dictionary<uint, GameObject> _remoteEntities = new();
    private PredictionSystem _predictor;
    private PurposeInterop.LogDelegate _logHandler;
    
    private uint _myID = 0;
    private bool _connected;
    private uint _currentTick = 0;

    private bool _w, _a, _s, _d;

    void Start()
    {
        _logHandler = (msg) => Debug.Log($"<color=cyan>[Native]</color> {msg}");
        PurposeInterop.RegisterLogCallback(_logHandler);

        _connected = PurposeInterop.ConnectToServer();
        if (!_connected) Debug.LogError("Connection Failed.");
    }

    void Update()
    {
        if (!_connected) return;

        PurposeInterop.ServiceNetwork();

        uint serverID = PurposeInterop.GetAssignedPlayerID();
        if (_myID != serverID && serverID != 0) _myID = serverID;

        while (PurposeInterop.GetNextEntityUpdate(out EntityState update))
        {
            if (!_remoteEntities.ContainsKey(update.networkID))
            {
                SpawnEntity(update);
            }
            else if (update.networkID == _myID)
            {
                if (_predictor != null) 
                    _predictor.HandleServerReconciliation(update, Time.fixedDeltaTime);
            }
            else
            {
                UpdateRemoteEntity(update);
            }
        }

        while ((serverID = PurposeInterop.GetNextDespawnID()) != 0)
        {
            if (_remoteEntities.TryGetValue(serverID, out var go))
            {
                Destroy(go);
                _remoteEntities.Remove(serverID);
            }
        }

        _w = Input.GetKey(KeyCode.W);
        _a = Input.GetKey(KeyCode.A);
        _s = Input.GetKey(KeyCode.S);
        _d = Input.GetKey(KeyCode.D);
        
        if (Input.GetKeyDown(KeyCode.T) && _remoteEntities.TryGetValue(_myID, out var entity))
            entity.transform.position += Vector3.right * 2.0f;
    }

    void FixedUpdate()
    {
        if (!_connected) return;
        _currentTick++;

        PurposeInterop.SendMovementInput(_currentTick, _w, _a, _s, _d);

        if (_remoteEntities.TryGetValue(_myID, out var myGO))
        {
            if (_predictor == null) _predictor = new PredictionSystem(myGO.transform);

            Vector3 newPos = PredictionSystem.SimulateMovement(myGO.transform.position, _w, _a, _s, _d, Time.fixedDeltaTime);
            
            myGO.transform.position = newPos;

            _predictor.RecordState(_currentTick, newPos, _w, _a, _s, _d);
        }
    }

    private void SpawnEntity(EntityState state)
    {
        var go = Instantiate(EntityPrefab, new Vector3(state.posX, state.posY, state.posZ), Quaternion.identity);
        go.name = (state.networkID == _myID) ? "MyPlayer" : $"Remote_{state.networkID}";
        _remoteEntities.Add(state.networkID, go);
    }

    private void UpdateRemoteEntity(EntityState state)
    {
        if (_remoteEntities.TryGetValue(state.networkID, out var go))
        {
            go.transform.position = new Vector3(state.posX, state.posY, state.posZ);
        }
    }

    private void OnApplicationQuit() => PurposeInterop.DisconnectFromServer();
}