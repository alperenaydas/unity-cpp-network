using System.Collections.Generic;
using UnityEngine;

public class EntityInterpolator : MonoBehaviour
{
    private struct StateSnapshot
    {
        public Vector3 Position;
        public Quaternion Rotation;
        public float TickTime;
    }

    private List<StateSnapshot> _snapshotBuffer = new List<StateSnapshot>();
    
    public float GlobalDelay = 0.1f; 

    private bool _isLocalPlayer = false;
    
    private float _clientRenderTime = 0;
    private bool _hasStarted = false;

    public void Initialize(bool isLocal)
    {
        _isLocalPlayer = isLocal;
        enabled = !isLocal;
    }
    
    public void ClearBuffer()
    {
        _snapshotBuffer.Clear();
        _hasStarted = false;
    }

    public void PushState(uint serverTick, Vector3? pos, Quaternion? rot)
    {
        float snapshotTime = serverTick * PurposeProtocol.TICK_DELTA;

        Vector3 newPos;
        Quaternion newRot;

        if (_snapshotBuffer.Count > 0)
        {
            var last = _snapshotBuffer[_snapshotBuffer.Count - 1];
            newPos = pos ?? last.Position;
            newRot = rot ?? last.Rotation;
        }
        else
        {
            newPos = pos ?? transform.position;
            newRot = rot ?? transform.rotation;
        }

        _snapshotBuffer.Add(new StateSnapshot
        {
            Position = newPos,
            Rotation = newRot,
            TickTime = snapshotTime
        });

        if (_snapshotBuffer.Count > 20) _snapshotBuffer.RemoveAt(0);
        
        if (!_hasStarted && _snapshotBuffer.Count >= 2)
        {
            _hasStarted = true;
            _clientRenderTime = snapshotTime - GlobalDelay;
        }
    }

    void Update()
    {
        if (_isLocalPlayer || !_hasStarted || _snapshotBuffer.Count < 2) return;

        _clientRenderTime += Time.deltaTime;

        float latestServerTime = _snapshotBuffer[_snapshotBuffer.Count - 1].TickTime;
        float targetTime = latestServerTime - GlobalDelay;
        float diff = targetTime - _clientRenderTime;

        if (Mathf.Abs(diff) > 1.0f) _clientRenderTime = targetTime;
        else _clientRenderTime += diff * 0.1f * Time.deltaTime;

        bool foundWindow = false;
        for (int i = 0; i < _snapshotBuffer.Count - 1; i++)
        {
            StateSnapshot from = _snapshotBuffer[i];
            StateSnapshot to = _snapshotBuffer[i + 1];

            if (_clientRenderTime >= from.TickTime && _clientRenderTime <= to.TickTime)
            {
                float duration = to.TickTime - from.TickTime;
                if (duration < 0.001f) duration = 0.02f;

                float t = (_clientRenderTime - from.TickTime) / duration;

                transform.position = Vector3.Lerp(from.Position, to.Position, t);
                transform.rotation = Quaternion.Slerp(from.Rotation, to.Rotation, t);
                foundWindow = true;
                break;
            }
        }
        
        if (!foundWindow && _snapshotBuffer.Count > 0)
        {
            var last = _snapshotBuffer[_snapshotBuffer.Count - 1];
            transform.position = last.Position;
            transform.rotation = last.Rotation;
        }
    }
}