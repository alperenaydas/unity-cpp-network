using UnityEngine;

public class PurposeInput : MonoBehaviour
{
    public static PurposeInput Instance { get; private set; }

    public bool W => Input.GetKey(KeyCode.W);
    public bool A => Input.GetKey(KeyCode.A);
    public bool S => Input.GetKey(KeyCode.S);
    public bool D => Input.GetKey(KeyCode.D);
    public bool Fire => Input.GetMouseButton(0);
    public float MouseYaw { get; private set; }

    private Transform _localPlayerTransform;

    private void Awake() => Instance = this;

    public void RegisterLocalPlayer(Transform playerTransform)
    {
        _localPlayerTransform = playerTransform;
        Debug.Log("<color=green>[Input]</color> Local Player registered for Yaw calculation.");
    }

    private void Update()
    {
        if (_localPlayerTransform == null) return;

        Plane groundPlane = new Plane(Vector3.up, Vector3.zero); 
        Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);

        if (groundPlane.Raycast(ray, out float enter))
        {
            Vector3 hitPoint = ray.GetPoint(enter);
            
            Vector3 direction = hitPoint - _localPlayerTransform.position;
            direction.y = 0; 
            
            if (direction.sqrMagnitude > 0.01f)
            {
                MouseYaw = Quaternion.LookRotation(direction).eulerAngles.y;
            }
        }
    }
}