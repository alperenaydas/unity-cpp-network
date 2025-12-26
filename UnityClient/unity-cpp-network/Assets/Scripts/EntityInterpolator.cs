using System.Collections.Generic;
using UnityEngine;

public class EntityInterpolator : MonoBehaviour
{
    private struct StateSnapshot
    {
        public Vector3 position;
        public Quaternion rotation;
        public float timestamp;
    }

    private List<StateSnapshot> _snapshotBuffer = new List<StateSnapshot>();
    
    [Header("Settings")]
    [SerializeField] private float _interpolationDelay = 0.1f;
    
    private bool _isLocalPlayer = false;

    public void Initialize(bool isLocal)
    {
        _isLocalPlayer = isLocal;
        enabled = !isLocal; 
    }

    public void PushState(Vector3 pos, Quaternion rot)
    {
        _snapshotBuffer.Add(new StateSnapshot {
            position = pos,
            rotation = rot,
            timestamp = Time.time
        });

        if (_snapshotBuffer.Count > 10) _snapshotBuffer.RemoveAt(0);
    }

    void Update()
    {
        if (_isLocalPlayer || _snapshotBuffer.Count < 2) return;

        float renderTime = Time.time - _interpolationDelay;

        for (int i = 0; i < _snapshotBuffer.Count - 1; i++)
        {
            StateSnapshot from = _snapshotBuffer[i];
            StateSnapshot to = _snapshotBuffer[i + 1];

            if (renderTime >= from.timestamp && renderTime <= to.timestamp)
            {
                float t = (renderTime - from.timestamp) / (to.timestamp - from.timestamp);
                
                transform.position = Vector3.Lerp(from.position, to.position, t);
                transform.rotation = Quaternion.Slerp(from.rotation, to.rotation, t);
                return;
            }
        }
    }
}