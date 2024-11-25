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
    string inputFilename;
    vector<int> originalIntegers;

public:
    Golomb(int m, bool decoder, int mode = 0, string inputFilename="") : inputFilename(inputFilename), m(m), bs("golomb.txt", decoder), mode(mode) {
        if(decoder == false){
            ifstream input(inputFilename);
            if (!input.is_open()) {
                cerr << "Error: Unable to open input file!" << endl;
                return;
            }

            int value;

            while (input >> value) { // Read signed integers directly
                originalIntegers.push_back(value);
            }
            input.close();
        }
    }

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

    void encode()
    {
        for (int value : originalIntegers) {
            if (mode == 0)
            {
                // Sign and magnitude
                bs.writeBit(value < 0); // Write sign bit
                value = abs(value);
            }
            else
            {
                // Zigzag interleaving
                // printf("Zigzag encoding\n");
                value = zigzagEncode(value);
            }
            int q = value / m;
            int r = value % m;
            int numBitsR = ceil(log2(m));

            // Write q as unary
            for (int i = 0; i < q; ++i)
            {
                bs.writeBit(1);
            }
            bs.writeBit(0); // End of unary

            // Write r in binary
            bs.writeBits(r, numBitsR);
        }
    }

    vector<int> decode()
    {
        vector<int> decodedValues;
        while(1){
            int q = 0;
            // int decodedValue = decoder.decode(); // Decode each value
            // decodedValues.push_back(decodedValue);
            if(bs.endOfFile())
                break;
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
                int r = bs.readBits(ceil(log2(m)));

                int magnitude = q * m + r;
                decodedValues.push_back(isNegative ? -magnitude : magnitude);
            }
            else
            {
                // printf("Zigzag decoding\n");
                // Zigzag Interleaving
                // Read unary for q
                while (bs.readBit() == 1)
                {
                    q++;
                }

                // Read binary for r
                int r = bs.readBits(ceil(log2(m)));

                int zigzagValue = q * m + r;
                decodedValues.push_back(zigzagDecode(zigzagValue));
            }
        }
        return decodedValues;
    }
};