public static class PurposeProtocol
{
    public const float TICK_RATE = 50.0f;
    public const float TICK_DELTA = 1.0f / TICK_RATE;
    public const float QUANT_RES = 100.0f;
    
    public enum PacketType : ushort
    {
        Welcome = 1,
        WorldState = 2,
        EntityDespawn = 3,
        ClientInput = 4,
        ClientAck = 5,
        DebugHit = 6,
        Spectator = 7
    }
}