using System.Runtime.InteropServices;

public static class PurposeInterop
{
    private const string DLL_NAME = "PurposeClient";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogDelegate([MarshalAs(UnmanagedType.LPStr)] string message);

    [DllImport(DLL_NAME)] public static extern void RegisterLogCallback(LogDelegate callback);
    [DllImport(DLL_NAME)] public static extern bool ConnectToServer();
    [DllImport(DLL_NAME)] public static extern void ServiceNetwork();
    [DllImport(DLL_NAME)] public static extern void DisconnectFromServer();
    
    [DllImport(DLL_NAME)] public static extern uint GetAssignedPlayerID();
    [DllImport(DLL_NAME)] public static extern bool GetNextEntityUpdate(out EntityState outState);
    [DllImport(DLL_NAME)] public static extern uint GetNextDespawnID();
    [DllImport(DLL_NAME)] public static extern void SendMovementInput(uint tick, bool w, bool a, bool s, bool d, bool fire, float yaw);
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct EntityState
{
    public ushort type;
    public uint networkID;
    public uint lastProcessedTick;
    public float posX, posY, posZ;
    public float rotationYaw;
}