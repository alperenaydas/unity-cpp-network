using UnityEngine;

public class PurposePlayer : MonoBehaviour
{
    [SerializeField] private EntityInterpolator _interpolator;
    [SerializeField] private MeshRenderer _playerMesh;
    [SerializeField] private Material _playerMaterial;
    [SerializeField] private Material _enemyMaterial;
    
    
    [SerializeField] private PurposeController _purposeController;

    public void InitializePlayer(bool isLocal, uint id)
    {
        _purposeController.IsLocalPlayer = isLocal;
        _interpolator.Initialize(isLocal);
        _playerMesh.material = isLocal ? _playerMaterial : _enemyMaterial;
        name = isLocal ? "MyPlayer" : $"RemotePlayer_{id}";
    }
    
    public void ApplyNetworkUpdate(Vector3 pos, Quaternion rot)
    {
        _interpolator.PushState(pos, rot);
    }
}
