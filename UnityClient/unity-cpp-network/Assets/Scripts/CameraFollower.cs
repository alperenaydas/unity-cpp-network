using UnityEngine;

public class CameraFollower : MonoBehaviour
{
    public Transform Target { get; set; }
    
    [Header("Settings")]
    public Vector3 Offset = new Vector3(0, 8, -10);
    public float SmoothSpeed = 5.0f;

    private void LateUpdate()
    {
        if (Target == null) return;

        Vector3 desiredPosition = Target.position + Offset;

        Vector3 smoothedPosition = Vector3.Lerp(transform.position, desiredPosition, SmoothSpeed * Time.deltaTime);
        transform.position = smoothedPosition;

        transform.LookAt(Target);
    }
}