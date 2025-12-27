#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm> // For std::max

class BitWriter {
public:
    BitWriter(size_t initialCapacity = 1400) {
        buffer.resize(initialCapacity, 0);
        bitPtr = 0;
    }

    void WriteBits(uint32_t value, int count) {
        for (int i = 0; i < count; ++i) {
            bool bit = (value >> (count - 1 - i)) & 1;
            WriteBit(bit);
        }
    }

    void WriteBit(bool value) {
        size_t byteIndex = bitPtr / 8;

        if (byteIndex >= buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }

        if (value) {
            buffer[byteIndex] |= (1 << (7 - (bitPtr % 8)));
        }
        bitPtr++;
    }

    void WriteInt(int32_t value, int bits) {
        WriteBits(static_cast<uint32_t>(value), bits);
    }

    void WriteFloat(float value) {
        uint32_t temp;
        std::memcpy(&temp, &value, sizeof(float));
        WriteBits(temp, 32);
    }

    void WriteAlign() {
        bitPtr = (bitPtr + 7) & ~7;
    }

    const uint8_t* GetData() const { return buffer.data(); }
    size_t GetByteLength() const { return (bitPtr + 7) / 8; }

private:
    std::vector<uint8_t> buffer;
    size_t bitPtr;
};

class BitReader {
public:
    BitReader(const uint8_t* buffer, size_t size)
        : buffer(buffer), size(size), bitPtr(0) {}

    bool ReadBit() {
        if (bitPtr >= size * 8) return false;
        bool value = (buffer[bitPtr / 8] & (1 << (7 - (bitPtr % 8)))) != 0;
        bitPtr++;
        return value;
    }

    uint32_t ReadBits(int count) {
        uint32_t value = 0;
        for (int i = 0; i < count; i++) {
            if (ReadBit()) {
                value |= (1 << (count - 1 - i));
            }
        }
        return value;
    }

    int32_t ReadInt(int bits) {
        return static_cast<int32_t>(ReadBits(bits));
    }

    float ReadFloat() {
        uint32_t temp = ReadBits(32);
        float val;
        std::memcpy(&val, &temp, sizeof(float));
        return val;
    }

private:
    const uint8_t* buffer;
    size_t size;
    size_t bitPtr;
};