using UnityEngine;

public class PurposePlayer : MonoBehaviour
{
    [SerializeField] private MeshRenderer _playerMesh;
    [SerializeField] private Material _playerMaterial;
    [SerializeField] private Material _enemyMaterial;
    
    
    [SerializeField] private PurposeController _purposeController;

    public void InitializePlayer(bool isLocal, uint id)
    {
        _purposeController.IsLocalPlayer = isLocal;
        _playerMesh.material = isLocal ? _playerMaterial : _enemyMaterial;
        name = isLocal ? "MyPlayer" : $"RemotePlayer_{id}";
    }
}
