using UnityEngine;

public class NetworkProfilerUI : MonoBehaviour
{
    [Header("Graphs")]
    public DebugGraph RttGraph;
    public DebugGraph BandwidthGraph; // Incoming data

    [Header("Settings")]
    public float RefreshRate = 0.05f; // Update 20 times a second for smooth visuals

    private float _timer;

    void Start()
    {
        // Configure RTT Graph (0-200ms)
        if (RttGraph != null)
        {
            RttGraph.MaxValue = 200f; 
            RttGraph.GoodColor = Color.green;     // < 100ms
            RttGraph.CautionColor = Color.yellow; // > 100ms
            RttGraph.CriticalColor = Color.red;   // > 160ms
        }

        // Configure Bandwidth Graph (0-50 KB/s)
        if (BandwidthGraph != null)
        {
            BandwidthGraph.MaxValue = 50f; 
            BandwidthGraph.GoodColor = Color.cyan;
            BandwidthGraph.CautionColor = new Color(0, 0.5f, 1f); // Darker Blue
            BandwidthGraph.CriticalColor = Color.blue;
        }
    }

    void Update()
    {
        _timer += Time.deltaTime;
        if (_timer >= RefreshRate)
        {
            _timer = 0;

            PurposeInterop.GetNetworkMetrics(out var metrics);

            if (RttGraph != null) RttGraph.PushValue(metrics.Ping);

            if (BandwidthGraph != null) BandwidthGraph.PushValue(metrics.IncomingBandwidth);
        }
    }
}