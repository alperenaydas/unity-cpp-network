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

    private byte[] _bitBuffer = new byte[4096];
    public int PlayerCount => _remotePlayers.Count;
    
    private List<DebugHit> _debugHits = new();

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

        if (_myID == 0) _myID = PurposeInterop.GetAssignedPlayerID();

        int bytesRead;
        while ((bytesRead = PurposeInterop.GetLatestBitstream(_bitBuffer, _bitBuffer.Length)) > 0)
        {
            var reader = new BitReader(_bitBuffer, bytesRead * 8);

            var typeLo = (ushort)reader.ReadBits(8);
            var typeHi = (ushort)reader.ReadBits(8);
            var type = (ushort)(typeLo | (typeHi << 8));

            if (type == 2) // World State
            {
                uint serverTick = reader.ReadBits(32);
                uint baselineTick = reader.ReadBits(32);
                int entityCount = (int)reader.ReadBits(8);

                for (int i = 0; i < entityCount; i++)
                {
                    uint id = reader.ReadBits(32);
                    bool moved = reader.ReadBit();
                    int qX = 0, qZ = 0;
                    if (moved)
                    {
                        qX = reader.ReadInt(32);
                        qZ = reader.ReadInt(32);
                    }
                    float yaw = reader.ReadFloat();
                    ProcessNetworkEntity(id, moved, qX, qZ, yaw);
                }
            }
            else if (type == 6) // Debug Hit
            {
                float hitX = reader.ReadFloat();
                float hitZ = reader.ReadFloat();
        
                _debugHits.Add(new DebugHit { 
                    pos = new Vector3(hitX, 0, hitZ), 
                    expireTime = Time.time + 2.0f
                });
                Debug.Log($"<color=green>HIT RECEIVED</color> {hitX}, {hitZ}");
            }
            else 
            {
                Debug.LogWarning($"Received Unknown Packet Type: {type}");
            }
        }
        
        _debugHits.RemoveAll(x => Time.time > x.expireTime);

        uint despawnID;
        while ((despawnID = PurposeInterop.GetNextDespawnID()) != 0)
        {
            if (_remotePlayers.TryGetValue(despawnID, out var p))
            {
                Debug.Log($"<color=red>[Network]</color> Despawn {despawnID}");
                Destroy(p.gameObject);
                _remotePlayers.Remove(despawnID);
            }
        }
    }

    private void ProcessNetworkEntity(uint id, bool moved, int qX, int qZ, float yaw)
    {
        if (!_remotePlayers.TryGetValue(id, out var player))
        {
            Vector3 spawnPos = moved ? new Vector3(qX / 100f, 0, qZ / 100f) : Vector3.zero;

            var instance = Instantiate(PlayerPrefab, spawnPos, Quaternion.identity);

            bool isLocal = (id == _myID);
            instance.InitializePlayer(isLocal, id);

            if (isLocal) PurposeInput.Instance.RegisterLocalPlayer(instance.transform);

            _remotePlayers.Add(id, instance);
            player = instance;
        }

        if (id != _myID)
        {
            if (moved)
            {
                Vector3 newPos = new Vector3(qX / 100f, 0, qZ / 100f);
                Quaternion newRot = Quaternion.Euler(0, yaw, 0);
                player.ApplyNetworkUpdate(newPos, newRot);
            }
            else
            {
                player.ApplyNetworkUpdate(player.transform.position, Quaternion.Euler(0, yaw, 0));
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

    private void OnDestroy()
    {
        if (_connected) PurposeInterop.DisconnectFromServer();
    }
    
    void OnDrawGizmos() {
        Gizmos.color = Color.red;
        foreach (var hit in _debugHits) {
            Vector3 drawPos = hit.pos + Vector3.up * 0.5f; 
            Gizmos.DrawWireSphere(drawPos, 0.5f); 
            Gizmos.DrawLine(drawPos, drawPos + Vector3.up * 2);
        }
    }
    
    private struct DebugHit { 
        public Vector3 pos; 
        public float expireTime; 
    }
}

