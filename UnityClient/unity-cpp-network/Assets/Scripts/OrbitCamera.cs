using UnityEngine;

public class WarzoneCamera : MonoBehaviour
{
    public float OrbitSpeed = 5f;
    void Update()
    {
        transform.RotateAround(Vector3.zero, Vector3.up, OrbitSpeed * Time.deltaTime);
        transform.LookAt(Vector3.zero);
    }
}