using UnityEngine;

public class LocalSmoother : MonoBehaviour
{
    [Header("Targets")]
    public Transform LogicRoot;
    public Transform VisualRoot;

    private Vector3 _localPosOffset;
    private Quaternion _localRotOffset;

    private Vector3 _prevPos;
    private Vector3 _currPos;
    private Quaternion _prevRot;
    private Quaternion _currRot;

    private void Start()
    {
        if (LogicRoot == null || VisualRoot == null)
        {
            enabled = false;
            return;
        }

        _localPosOffset = LogicRoot.InverseTransformPoint(VisualRoot.position);
        _localRotOffset = Quaternion.Inverse(LogicRoot.rotation) * VisualRoot.rotation;

        VisualRoot.SetParent(null);

        _prevPos = _currPos = GetTargetPosition();
        _prevRot = _currRot = GetTargetRotation();
    }

    private void OnDestroy()
    {
        if (VisualRoot != null && LogicRoot != null)
        {
            VisualRoot.SetParent(LogicRoot);
            VisualRoot.localPosition = _localPosOffset;
            VisualRoot.localRotation = _localRotOffset;
        }
    }

    private void FixedUpdate()
    {
        _prevPos = _currPos;
        _prevRot = _currRot;

        _currPos = GetTargetPosition();
        _currRot = GetTargetRotation();
    }

    private void Update()
    {
        if (LogicRoot == null) 
        {
            Destroy(VisualRoot.gameObject);
            return;
        }

        float factor = (Time.time - Time.fixedTime) / Time.fixedDeltaTime;

        VisualRoot.position = Vector3.Lerp(_prevPos, _currPos, factor);
        VisualRoot.rotation = Quaternion.Slerp(_prevRot, _currRot, factor);
    }

    private Vector3 GetTargetPosition()
    {
        return LogicRoot.TransformPoint(_localPosOffset);
    }

    private Quaternion GetTargetRotation()
    {
        return LogicRoot.rotation * _localRotOffset;
    }
}