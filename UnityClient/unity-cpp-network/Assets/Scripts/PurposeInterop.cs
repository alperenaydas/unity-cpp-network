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
    public static extern bool GetNextEntityUpdate(out EntityData outData);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern uint GetNextDespawnID();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void GetNetworkMetrics(out NetworkMetrics metrics);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] 
    public static extern void SendMovementInput(uint tick, bool w, bool a, bool s, bool d, bool fire, float yaw);
}


[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct EntityData
{
    public uint networkID;
    public uint lastProcessedTick;
    public float posX, posY, posZ;
    public float rotationYaw;
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct NetworkMetrics
{
    public uint ping;
    public uint packetLoss;
    public ulong totalBytesSent;
    public ulong totalBytesReceived;
    public float incomingBandwidth;
    public float outgoingBandwidth;
}