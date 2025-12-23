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

    public void HandleServerReconciliation(EntityState serverState, float deltaTime)
    {
        _history.RemoveAll(state => state.tick < serverState.lastProcessedTick);

        int historyIndex = _history.FindIndex(state => state.tick == serverState.lastProcessedTick);

        if (historyIndex != -1)
        {
            HistoryState recordedState = _history[historyIndex];
            Vector3 serverPos = new Vector3(serverState.posX, serverState.posY, serverState.posZ);

            float error = Vector3.Distance(recordedState.position, serverPos);

            if (error > TOLERANCE)
            {
                _targetTransform.position = serverPos;

                for (int i = historyIndex + 1; i < _history.Count; i++)
                {
                    HistoryState oldInput = _history[i];
                    
                    Vector3 newPos = SimulateMovement(_targetTransform.position, oldInput.w, oldInput.a, oldInput.s, oldInput.d, deltaTime);

                    _targetTransform.position = newPos;

                    var correctedState = _history[i];
                    correctedState.position = newPos;
                    _history[i] = correctedState;
                }
            }
        }
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