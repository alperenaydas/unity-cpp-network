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
        string inKB = (m.IncomingBandwidth / 1024f).ToString("F2");
        string outKB = (m.OutgoingBandwidth / 1024f).ToString("F2");

        _displayText.text = 
            $"<color=green>RTT:</color> {m.Ping}ms\n" +
            $"<color=red>LOSS:</color> {m.PacketLoss}%\n" +
            $"<color=yellow>PLAYERS:</color> {_networkManager.PlayerCount}\n" +
            $"<color=cyan>DL:</color> {m.IncomingBandwidth:F2} KB/s\n" +
            $"<color=cyan>UL:</color> {m.OutgoingBandwidth:F2} KB/s";
    }
}