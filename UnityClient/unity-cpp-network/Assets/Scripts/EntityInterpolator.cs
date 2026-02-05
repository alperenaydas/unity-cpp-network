using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Serialization;

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

    public void Initialize(bool isLocal)
    {
        _isLocalPlayer = isLocal;
        enabled = !isLocal;
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
    }

    void Update()
    {
        if (_isLocalPlayer || _snapshotBuffer.Count < 2) return;

        float latestServerTime = _snapshotBuffer[_snapshotBuffer.Count - 1].TickTime;
        float renderTime = latestServerTime - GlobalDelay;

        for (int i = 0; i < _snapshotBuffer.Count - 1; i++)
        {
            StateSnapshot from = _snapshotBuffer[i];
            StateSnapshot to = _snapshotBuffer[i + 1];

            if (renderTime >= from.TickTime && renderTime <= to.TickTime)
            {
                float total = to.TickTime - from.TickTime;
                float t = (renderTime - from.TickTime) / total;

                transform.position = Vector3.Lerp(from.Position, to.Position, t);
                transform.rotation = Quaternion.Slerp(from.Rotation, to.Rotation, t);
                return;
            }
        }
    }
}