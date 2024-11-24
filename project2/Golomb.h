#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <bitset>
#include <cassert>
#include <vector>
#include <cmath> // For log2()

#include "BitStream.h"

using namespace std;

class Golomb
{
private:
    int m;
    BitStream bs;
    int mode; // 0 for sign/magnitude, 1 for zigzag interleaving

public:
    Golomb(int m, bool decoder, int mode = 0) : m(m), bs("golomb.txt", decoder), mode(mode) {}

    void end()
    {
        bs.end();
    }
     
    int zigzagEncode(int value)
    {
        return value >= 0 ? 2 * value : -2 * value - 1;
    }

    int zigzagDecode(int value)
    {
        return (value % 2 == 0) ? (value / 2) : -(value / 2) - 1;
    }

    void encode(int value)
    {
        if (mode == 0)
        {
            // Sign and magnitude
            bs.writeBit(value < 0); // Write sign bit
            value = abs(value);
        }
        else
        {
            // Zigzag interleaving
            printf("Zigzag encoding\n");
            value = zigzagEncode(value);
        }

        int q = value / m;
        int r = value % m;
        int numBitsR = log2(m);

        // Write q as unary
        for (int i = 0; i < q; ++i)
        {
            bs.writeBit(1);
        }
        bs.writeBit(0); // End of unary

        // Write r in binary
        bs.writeBits(r, numBitsR);
    }

    int decode()
    {
        int q = 0;

        if (mode == 0)
        {
            // Sign and Magnitude
            bool isNegative = bs.readBit(); // Read sign bit

            // Read unary for q
            while (bs.readBit() == 1)
            {
                q++;
            }

            // Read binary for r
            int r = bs.readBits(log2(m));

            int magnitude = q * m + r;
            return isNegative ? -magnitude : magnitude;
        }
        else
        {
            printf("Zigzag decoding\n");
            // Zigzag Interleaving
            // Read unary for q
            while (bs.readBit() == 1)
            {
                q++;
            }

            // Read binary for r
            int r = bs.readBits(log2(m));

            int zigzagValue = q * m + r;
            return zigzagDecode(zigzagValue);
        }
    }
};