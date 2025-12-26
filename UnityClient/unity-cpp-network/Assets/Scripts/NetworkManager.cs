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

    public int PlayerCount => _remotePlayers.Count;

    void Start()
    {
        _logHandler = (msg) => Debug.Log($"<color=cyan>[Native]</color> {msg}");
        PurposeInterop.RegisterLogCallback(_logHandler);

        _connected = PurposeInterop.ConnectToServer();
        if (!_connected) Debug.LogError("Purpose Server Connection Failed.");
    }

    void Update()
    {
        if (!_connected) return;

        PurposeInterop.ServiceNetwork();

        if (_myID == 0)
        {
            _myID = PurposeInterop.GetAssignedPlayerID();
        }

        while (PurposeInterop.GetNextEntityUpdate(out EntityData update))
        {
            if (!_remotePlayers.ContainsKey(update.networkID))
            {
                SpawnEntity(update);
            }
            else
            {
                UpdateEntityState(update);
            }
        }

        uint despawnID;
        while ((despawnID = PurposeInterop.GetNextDespawnID()) != 0)
        {
            if (_remotePlayers.TryGetValue(despawnID, out var player))
            {
                Debug.Log($"<color=red>[Network]</color> Despawning Entity {despawnID}");
                Destroy(player.gameObject);
                _remotePlayers.Remove(despawnID);
            }
        }
    }

    void FixedUpdate()
    {
        if (!_connected || _myID == 0) return;

        _currentTick++;
        var input = PurposeInput.Instance;

        PurposeInterop.SendMovementInput(_currentTick, input.W, input.A, input.S, input.D, input.Fire, input.MouseYaw);

        if (_remotePlayers.TryGetValue(_myID, out var myPlayer))
        {
            if (_predictor == null) _predictor = new PredictionSystem(myPlayer.transform);

            Vector3 predictedPos = PredictionSystem.SimulateMovement(
                myPlayer.transform.position, 
                input.W, input.A, input.S, input.D, 
                Time.fixedDeltaTime
            );

            myPlayer.transform.position = predictedPos;
            myPlayer.transform.rotation = Quaternion.Euler(0, input.MouseYaw, 0);

            _predictor.RecordState(_currentTick, predictedPos, input.W, input.A, input.S, input.D);
        }
    }

    private void SpawnEntity(EntityData state)
    {
        Vector3 spawnPos = new Vector3(state.posX, state.posY, state.posZ);
        var playerInstance = Instantiate(PlayerPrefab, spawnPos, Quaternion.identity);
    
        bool isLocal = (state.networkID == _myID);
        playerInstance.InitializePlayer(isLocal, state.networkID);
    
        if (isLocal)
        {
            PurposeInput.Instance.RegisterLocalPlayer(playerInstance.transform);
        }
    
        _remotePlayers.Add(state.networkID, playerInstance);
    }

    private void UpdateEntityState(EntityData state)
    {
        if (!_remotePlayers.TryGetValue(state.networkID, out var player)) return;

        if (state.networkID != _myID) 
        {
            player.transform.position = new Vector3(state.posX, state.posY, state.posZ);
            player.transform.rotation = Quaternion.Euler(0, state.rotationYaw, 0);
        }
        else 
        {
            _predictor?.OnServerReconciliation(state.lastProcessedTick, new Vector3(state.posX, state.posY, state.posZ));
        }
    }

    private void OnApplicationQuit() 
    {
        if (_connected) PurposeInterop.DisconnectFromServer();
    }
}