using UnityEngine;
using UnityEngine.UI;

public class DebugGraph : MonoBehaviour
{
    [Header("UI Setup")]
    public RawImage TargetImage;
    public Color GoodColor = Color.green;
    public Color CautionColor = Color.yellow;
    public Color CriticalColor = Color.red;

    [Header("Settings")]
    public float MaxValue = 200f; // e.g., 200ms for Ping
    public int Resolution = 128;  // Width of the graph
    public int Height = 64;       // Height of the graph

    private Texture2D _texture;
    private Color[] _clearColors;
    private float[] _history;
    private int _headIndex = 0;

    void Start()
    {
        if (TargetImage == null) TargetImage = GetComponent<RawImage>();

        // Initialize Texture
        _texture = new Texture2D(Resolution, Height, TextureFormat.RGBA32, false);
        _texture.filterMode = FilterMode.Point; // Keep it crisp (pixelated style)
        _texture.wrapMode = TextureWrapMode.Clamp;
        TargetImage.texture = _texture;

        // Initialize Data
        _history = new float[Resolution];
        
        // Optimization: Cache clear colors
        _clearColors = new Color[Resolution * Height];
        Color transparent = new Color(0, 0, 0, 0.5f); // Semi-transparent background
        for (int i = 0; i < _clearColors.Length; i++) _clearColors[i] = transparent;
    }

    public void PushValue(float value)
    {
        // 1. Store Value (Circular Buffer style, but simplified for Texture: we shift visually)
        // Actually, for a rolling graph, it's cheaper to just rewrite the texture data based on a fixed array
        
        // Shift history left
        System.Array.Copy(_history, 1, _history, 0, _history.Length - 1);
        _history[_history.Length - 1] = value;

        DrawGraph();
    }

    private void DrawGraph()
    {
        // Clear background
        _texture.SetPixels(_clearColors);

        for (int x = 0; x < Resolution; x++)
        {
            float val = _history[x];
            // Normalize height (0.0 to 1.0)
            float normalized = Mathf.Clamp01(val / MaxValue);
            int columnHeight = Mathf.RoundToInt(normalized * Height);

            // Determine Color based on severity
            Color c = GoodColor;
            if (val > MaxValue * 0.5f) c = CautionColor;
            if (val > MaxValue * 0.8f) c = CriticalColor;

            // Draw vertical bar for this column
            for (int y = 0; y < columnHeight; y++)
            {
                _texture.SetPixel(x, y, c);
            }
        }
        
        _texture.Apply();
    }
}