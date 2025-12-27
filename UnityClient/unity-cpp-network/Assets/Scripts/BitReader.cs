using System;

public class BitReader
{
    private readonly byte[] _buffer;
    private int _bitPtr;
    private readonly int _totalBits;

    public BitReader(byte[] buffer, int bitCount = -1)
    {
        _buffer = buffer;
        _bitPtr = 0;
        _totalBits = (bitCount == -1) ? (buffer.Length * 8) : bitCount;
    }

    public bool ReadBit()
    {
        if (_bitPtr >= _totalBits) 
        {
            return false;
        }

        bool value = (_buffer[_bitPtr >> 3] & (1 << (7 - (_bitPtr & 7)))) != 0;
        _bitPtr++;
        return value;
    }

    public uint ReadBits(int count)
    {
        uint value = 0;
        for (int i = 0; i < count; i++)
        {
            if (ReadBit())
                value |= (uint)(1 << (count - 1 - i));
        }
        return value;
    }

    public int ReadInt(int bits) => (int)ReadBits(bits);

    public float ReadFloat()
    {
        uint bits = ReadBits(32);
        byte[] bytes = BitConverter.GetBytes(bits);
        return BitConverter.ToSingle(bytes, 0);
    }
}