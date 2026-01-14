using UnityEngine;

public class PurposeSpectator : MonoBehaviour
{
    [SerializeField] private Camera _spectatorCamera;

    private void Start()
    {
        if (Camera.main != null && Camera.main != _spectatorCamera)
        {
            Camera.main.gameObject.SetActive(false);
        }
        
        _spectatorCamera.tag = "MainCamera";
        _spectatorCamera.enabled = true;
    }
}