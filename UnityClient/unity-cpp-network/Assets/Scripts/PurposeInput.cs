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
    
    private bool _inDeadzone = false;
    
    private const float DIST_ENTER_DEADZONE = 0.5f;
    private const float DIST_EXIT_DEADZONE = 2.0f;

    private void Awake() => Instance = this;

    public void RegisterLocalPlayer(Transform playerTransform)
    {
        _localPlayerTransform = playerTransform;
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
            
            float distSq = direction.sqrMagnitude;

            if (_inDeadzone) {
                if (distSq > DIST_EXIT_DEADZONE) {
                    _inDeadzone = false;
                }
            }
            else {
                if (distSq < DIST_ENTER_DEADZONE) {
                    _inDeadzone = true;
                }
            }

            if (!_inDeadzone)
            {
                MouseYaw = Quaternion.LookRotation(direction).eulerAngles.y;
            }
        }
    }
}