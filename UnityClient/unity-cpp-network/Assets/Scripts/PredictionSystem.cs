using System.Collections.Generic;
using UnityEngine;

public class PredictionSystem
{
    private struct HistoryState
    {
        public uint tick;
        public Vector3 position;
        public bool w, a, s, d;
    }

    private List<HistoryState> _history = new();
    private const float TOLERANCE = 0.05f;

    private Transform _targetTransform;

    public PredictionSystem(Transform target)
    {
        _targetTransform = target;
    }

    public void RecordState(uint tick, Vector3 pos, bool w, bool a, bool s, bool d)
    {
        _history.Add(new HistoryState {
            tick = tick, position = pos,
            w = w, a = a, s = s, d = d
        });
    }

    public void OnServerReconciliation(uint serverTick, Vector3 serverPos)
    {
        int index = _history.FindIndex(s => s.tick == serverTick);
        if (index == -1) return;

        if (Vector3.Distance(_history[index].position, serverPos) > TOLERANCE)
        {
            Debug.LogWarning($"[Reconciliation] Desync at tick {serverTick}. Correcting...");
            
            _targetTransform.position = serverPos;

            for (int i = index; i < _history.Count; i++)
            {
                var snapshot = _history[i];
                _targetTransform.position = SimulateMovement(_targetTransform.position, snapshot.w, snapshot.a, snapshot.s, snapshot.d, Time.fixedDeltaTime);
                
                snapshot.position = _targetTransform.position;
                _history[i] = snapshot;
            }
        }
        
        _history.RemoveRange(0, index);
    }

    public static Vector3 SimulateMovement(Vector3 start, bool w, bool a, bool s, bool d, float dt)
    {
        float speed = 5.0f * dt;
        Vector3 pos = start;
        if (w) pos.z += speed;
        if (s) pos.z -= speed;
        if (a) pos.x -= speed;
        if (d) pos.x += speed;
        return pos;
    }
}