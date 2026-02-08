using UnityEngine;
using TMPro;
using System.Text; // Required for StringBuilder

public class PurposeNetworkUI : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI _displayText;
    [SerializeField] private NetworkManager _networkManager;

    private float _updateInterval = 0.5f;
    private float _timer;
    private StringBuilder _sb = new StringBuilder(500);

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
        _sb.Clear();
        _sb.Append("<color=green>RTT:</color> ").Append(m.Ping).Append("ms\n");
        _sb.Append("<color=red>LOSS:</color> ").Append(m.PacketLoss).Append("%\n");
        _sb.Append("<color=yellow>PLAYERS:</color> ").Append(_networkManager.PlayerCount).Append("\n");
        _sb.Append("<color=cyan>DL:</color> ").Append(m.IncomingBandwidth.ToString("F2")).Append(" KB/s\n");
        _sb.Append("<color=cyan>UL:</color> ").Append(m.OutgoingBandwidth.ToString("F2")).Append(" KB/s");
        _displayText.SetText(_sb);
    }
}