using System.Collections.Generic;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    public PurposePlayer PlayerPrefab;
    public PurposeSpectator SpectatorPrefab;

    private Dictionary<uint, PurposePlayer> _remotePlayers = new();
    private PredictionSystem _predictor;
    private PurposeInterop.LogDelegate _logHandler;

    private uint _myID = 0;
    private bool _connected;
    private uint _currentTick = 0;

    private byte[] _bitBuffer = new byte[4096];
    public int PlayerCount => _remotePlayers.Count;

    private List<DebugHit> _debugHits = new();

    private float _timeoutDuration = 1.0f;

    private bool _isSpectatorMode = false;

    void Start()
    {
        _logHandler = (msg) => Debug.Log($"<color=cyan>[Native]</color> {msg}");
        PurposeInterop.RegisterLogCallback(_logHandler);

        _connected = PurposeInterop.ConnectToServer();
        if (!_connected) Debug.LogError("Purpose Server Connection Failed.");
        JoinAsSpectator();
    }

    void Update()
    {
        if (!_connected) return;

        PurposeInterop.ServiceNetwork();

        if (_myID == 0) _myID = PurposeInterop.GetAssignedPlayerID();

        int bytesRead;
        while ((bytesRead = PurposeInterop.GetLatestBitstream(_bitBuffer, _bitBuffer.Length)) > 0)
        {
            ProcessPacket(_bitBuffer, bytesRead);
        }

        ProcessDespawns();

        _debugHits.RemoveAll(x => Time.time > x.expireTime);

        CheckTimeouts();
    }

    private void ProcessPacket(byte[] buffer, int length)
    {
        var reader = new BitReader(_bitBuffer, length * 8);

        var typeLo = (ushort)reader.ReadBits(8);
        var typeHi = (ushort)reader.ReadBits(8);
        var type = (PurposeProtocol.PacketType)(typeLo | (typeHi << 8));

        switch (type)
        {
            case PurposeProtocol.PacketType.WorldState:
                OnWorldState(reader);
                break;

            case PurposeProtocol.PacketType.DebugHit:
                var hitX = reader.ReadFloat();
                var hitZ = reader.ReadFloat();
                _debugHits.Add(new DebugHit { pos = new Vector3(hitX, 0, hitZ), expireTime = Time.time + 2.0f });
                break;
        }
    }

    private void OnWorldState(BitReader reader)
    {
        var serverTick = reader.ReadBits(32);
        var baselineTick = reader.ReadBits(32);
        var entityCount = (int)reader.ReadBits(10);
        
        for (int i = 0; i < entityCount; i++)
        {
            var id = reader.ReadBits(32);
            var posChanged = reader.ReadBit();
            int qX = 0, qZ = 0;
            if (posChanged)
            {
                qX = reader.ReadInt(32);
                qZ = reader.ReadInt(32);
            }

            bool rotChanged = reader.ReadBit();
            float yaw = 0;
            if (rotChanged)
            {
                yaw = reader.ReadFloat();
            }

            ProcessNetworkEntity(serverTick, id, posChanged, qX, qZ, rotChanged, yaw);
        }
    }

    private void ProcessNetworkEntity(uint serverTick, uint id, bool posChanged, int qX, int qZ, bool rotChanged,
        float yaw)
    {
        if (_isSpectatorMode && id == _myID) return;

        if (!_remotePlayers.TryGetValue(id, out var player))
        {
            var spawnPos = posChanged
                ? new Vector3(qX / PurposeProtocol.QUANT_RES, 0, qZ / PurposeProtocol.QUANT_RES)
                : Vector3.zero;

            var isLocal = id == _myID;
            player = Instantiate(PlayerPrefab, spawnPos, Quaternion.identity);
            player.InitializePlayer(isLocal, id);

            if (isLocal)
            {
                PurposeInput.Instance.RegisterLocalPlayer(player.transform);
                var cam = Camera.main?.GetComponent<CameraFollower>();
                if (cam) 
                {
                    var smoother = player.GetComponent<LocalSmoother>();
                    if (smoother != null && smoother.VisualRoot != null)
                    {
                        cam.Target = smoother.VisualRoot;
                    }
                    else
                    {
                        cam.Target = player.transform;
                    }
                }
            }

            _remotePlayers.Add(id, player);
        }

        Vector3? newPos = null;
        if (posChanged)
        {
            newPos = new Vector3(qX / PurposeProtocol.QUANT_RES, 0, qZ / PurposeProtocol.QUANT_RES);
        }

        Quaternion? newRot = null;
        if (rotChanged)
        {
            newRot = Quaternion.Euler(0, yaw, 0);
        }

        if (id == _myID)
        {
            if (posChanged)
            {
                _predictor.OnServerReconciliation(serverTick, newPos.Value);
            }
        }
        else
        {
            player.ApplyNetworkUpdate(serverTick, newPos, newRot);
            
            if (posChanged && newPos.HasValue)
            {
                float dist = Vector3.Distance(player.transform.position, newPos.Value);
                if (dist > 5.0f) 
                {
                    player.transform.position = newPos.Value;
                    player.ResetInterpolation(); 
                }
            }
        }

        player.LastUpdateTime = Time.time;
    }

    private void ProcessDespawns()
    {
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

    private void CheckTimeouts()
    {
        List<uint> toRemove = new List<uint>();
        foreach (var kvp in _remotePlayers)
        {
            if (Time.time - kvp.Value.LastUpdateTime > _timeoutDuration)
            {
                toRemove.Add(kvp.Key);
            }
        }

        foreach (uint id in toRemove)
        {
            Destroy(_remotePlayers[id].gameObject);
            _remotePlayers.Remove(id);
        }
    }

    void FixedUpdate()
    {
        if (!_connected || _myID == 0) return;

        _currentTick++;

        if (_isSpectatorMode)
        {
            return;
        }

        var input = PurposeInput.Instance;

        PurposeInterop.SendMovementInput(_currentTick, input.W, input.A, input.S, input.D, input.Fire, input.MouseYaw);

        if (_remotePlayers.TryGetValue(_myID, out var myPlayer))
        {
            if (_predictor == null) _predictor = new PredictionSystem(myPlayer.transform);

            var predictedPos = PredictionSystem.SimulateMovement(
                myPlayer.transform.position,
                input.W, input.A, input.S, input.D,
                Time.fixedDeltaTime
            );

            myPlayer.transform.position = predictedPos;
            myPlayer.transform.rotation = Quaternion.Euler(0, input.MouseYaw, 0);

            _predictor.RecordState(_currentTick, predictedPos, input.W, input.A, input.S, input.D);
        }
    }

    private void JoinAsSpectator()
    {
        _isSpectatorMode = true;
        PurposeInterop.SendBecomeSpectatorRequest();

        if (_remotePlayers.TryGetValue(_myID, out var myPlayer))
        {
            Destroy(myPlayer.gameObject);
            _remotePlayers.Remove(_myID);
        }

        Instantiate(SpectatorPrefab, new Vector3(0, 20f, 0), Quaternion.identity);
        Debug.Log("<color=yellow>[Spectator]</color> Mode Enabled. Use WASD + Shift to fly.");
    }

    private void OnDestroy()
    {
        if (_connected) PurposeInterop.DisconnectFromServer();
    }

    void OnDrawGizmos()
    {
        Gizmos.color = Color.red;
        foreach (var hit in _debugHits)
        {
            Gizmos.DrawWireSphere(hit.pos, 0.5f);
            Gizmos.DrawLine(hit.pos, hit.pos + Vector3.up * 2);
        }
    }

    private struct DebugHit
    {
        public Vector3 pos;
        public float expireTime;
    }
}