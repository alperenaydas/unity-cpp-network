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

    private void Awake() => Instance = this;

    private void Update()
    {
        Plane groundPlane = new Plane(Vector3.up, Vector3.zero); 
        Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);

        if (groundPlane.Raycast(ray, out float enter))
        {
            Vector3 hitPoint = ray.GetPoint(enter);
            GameObject localPlayer = GameObject.Find("MyPlayer"); 
            if (localPlayer != null)
            {
                Vector3 direction = hitPoint - localPlayer.transform.position;
                direction.y = 0; // Keep it on the horizontal plane
                
                if (direction.sqrMagnitude > 0.1f)
                {
                    MouseYaw = Quaternion.LookRotation(direction).eulerAngles.y;
                }
            }
        }
    }
}