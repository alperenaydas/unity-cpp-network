using UnityEngine;
using TMPro;

public class PurposeNetworkUI : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI _displayText;
    [SerializeField] private NetworkManager _networkManager;

    private float _updateInterval = 0.5f;
    private float _timer;

    void Update()
    {
        _timer += Time.deltaTime;
        if (_timer >= _updateInterval)
        {
            PurposeInterop.GetNetworkMetrics(out var metrics);
            UpdateDisplay(metrics);
            _timer = 0;
        }
    }

    private void UpdateDisplay(NetworkMetrics m)
    {
        // Format bytes to KB/MB for readability
        string inKB = (m.incomingBandwidth / 1024f).ToString("F2");
        string outKB = (m.outgoingBandwidth / 1024f).ToString("F2");

        _displayText.text = 
            $"<color=green>RTT:</color> {m.ping}ms\n" +
            $"<color=red>LOSS:</color> {m.packetLoss}%\n" +
            $"<color=yellow>PLAYERS:</color> {_networkManager.PlayerCount}\n" +
            $"<color=cyan>DL:</color> {m.incomingBandwidth:F2} KB/s\n" +
            $"<color=cyan>UL:</color> {m.outgoingBandwidth:F2} KB/s";
    }
}