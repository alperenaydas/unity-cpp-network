using UnityEngine;

public class PurposeController : MonoBehaviour
{
    public bool IsLocalPlayer = false;
    
    private void Update()
    {
        if (!IsLocalPlayer) return;
        
        transform.rotation = Quaternion.Euler(0, PurposeInput.Instance.MouseYaw, 0);
    }
}