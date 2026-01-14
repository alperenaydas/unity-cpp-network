using UnityEngine;

public class PurposeFreeController : MonoBehaviour
{
    [Header("Settings")]
    [SerializeField] private float _moveSpeed = 20f;
    [SerializeField] private float _fastMultiplier = 3f;
    [SerializeField] private float _mouseSensitivity = 2f;
    [SerializeField] private bool _invertY = false;

    private float _rotationX = 0f;
    private float _rotationY = 0f;

    private void Start()
    {
        Vector3 rot = transform.localEulerAngles;
        _rotationY = rot.y;
        _rotationX = rot.x;

        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;
    }

    private void Update()
    {
        HandleMouseLook();
        HandleMovement();
        
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
    }

    private void HandleMouseLook()
    {
        float mouseX = Input.GetAxis("Mouse X") * _mouseSensitivity;
        float mouseY = Input.GetAxis("Mouse Y") * _mouseSensitivity;

        _rotationY += mouseX;
        _rotationX += _invertY ? mouseY : -mouseY;

        _rotationX = Mathf.Clamp(_rotationX, -90f, 90f);

        transform.localRotation = Quaternion.Euler(_rotationX, _rotationY, 0);
    }

    private void HandleMovement()
    {
        float currentSpeed = _moveSpeed;
        if (Input.GetKey(KeyCode.LeftShift)) currentSpeed *= _fastMultiplier;

        float moveX = Input.GetAxis("Horizontal");
        float moveZ = Input.GetAxis("Vertical");
        float moveY = 0f;

        if (Input.GetKey(KeyCode.Space)) moveY = 1f;
        if (Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.C)) moveY = -1f;

        Vector3 direction = (transform.right * moveX) + (transform.forward * moveZ) + (Vector3.up * moveY);
        
        transform.position += direction * (currentSpeed * Time.deltaTime);
    }
}