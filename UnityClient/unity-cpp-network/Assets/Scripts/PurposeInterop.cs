using System.Runtime.InteropServices;

public static class PurposeInterop
{
    private const string DLL_NAME = "PurposeClient";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogDelegate([MarshalAs(UnmanagedType.LPStr)] string message);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void RegisterLogCallback(LogDelegate callback);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern bool ConnectToServer();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void ServiceNetwork();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void DisconnectFromServer();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern uint GetAssignedPlayerID();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern uint GetNextDespawnID();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void GetNetworkMetrics(out NetworkMetrics metrics);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void SendMovementInput(uint tick, bool w, bool a, bool s, bool d, bool fire, float yaw);
    
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void SendBecomeSpectatorRequest();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern int GetLatestBitstream([In, Out] byte[] outBuffer, int maxLen);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void SendSpectateTarget(uint targetID);
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct NetworkMetrics
{
    public uint Ping;
    public uint PacketLoss;
    public ulong TotalBytesSent;
    public ulong TotalBytesReceived;
    public float IncomingBandwidth;
    public float OutgoingBandwidth;
}