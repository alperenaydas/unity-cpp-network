using System.Collections.Generic;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    public PurposePlayer PlayerPrefab;
    
    private Dictionary<uint, PurposePlayer> _remotePlayers = new();
    private PredictionSystem _predictor;
    private PurposeInterop.LogDelegate _logHandler;
    
    private uint _myID = 0;
    private bool _connected;
    private uint _currentTick = 0;
    
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
            if (!_remotePlayers.ContainsKey(update.networkID))
            {
                SpawnEntity(update);
            }
            else
            {
                UpdateRemoteEntity(update);
            }
        }

        while ((serverID = PurposeInterop.GetNextDespawnID()) != 0)
        {
            if (_remotePlayers.TryGetValue(serverID, out var go))
            {
                Destroy(go);
                _remotePlayers.Remove(serverID);
            }
        }
    }

    void FixedUpdate()
    {
        if (!_connected) return;
        _currentTick++;

        var input = PurposeInput.Instance;

        PurposeInterop.SendMovementInput(_currentTick, input.W, input.A, input.S, input.D, input.Fire, input.MouseYaw);

        if (_remotePlayers.TryGetValue(_myID, out var myGO))
        {
            if (_predictor == null) _predictor = new PredictionSystem(myGO.transform);

            Vector3 newPos = PredictionSystem.SimulateMovement(myGO.transform.position, input.W, input.A, input.S, input.D, Time.fixedDeltaTime);
            myGO.transform.position = newPos;
            myGO.transform.rotation = Quaternion.Euler(0, input.MouseYaw, 0);

            _predictor.RecordState(_currentTick, newPos, input.W, input.A, input.S, input.D);
        }
    }

    private void SpawnEntity(EntityState state)
    {
        var go = Instantiate(PlayerPrefab, new Vector3(state.posX, state.posY, state.posZ), Quaternion.identity);
        go.InitializePlayer(state.networkID == _myID, state.networkID);
        _remotePlayers.Add(state.networkID, go);
    }

    private void UpdateRemoteEntity(EntityState state)
    {
        if (_remotePlayers.TryGetValue(state.networkID, out var go))
        {
            if (state.networkID != _myID) 
            {
                go.transform.position = new Vector3(state.posX, state.posY, state.posZ);
                go.transform.rotation = Quaternion.Euler(0, state.rotationYaw, 0);
            }
            else 
            {
                _predictor?.OnServerReconciliation(state.lastProcessedTick, new Vector3(state.posX, state.posY, state.posZ));
            }
        }
    }

    private void OnApplicationQuit() => PurposeInterop.DisconnectFromServer();
}