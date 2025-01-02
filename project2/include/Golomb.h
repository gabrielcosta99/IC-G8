#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath> // For log2()
#include "BitStream.h"


using namespace std;

class Golomb {
private:
    int m;                     // Golomb parameter
    BitStream bs;              // BitStream for binary file operations
    int mode;                  // 0 for Sign/Magnitude, 1 for Zigzag Interleaving
    string inputFilename;      // Optional input file for integers
    vector<int> originalIntegers; // Buffer for integers (used for encoding)
    int numBitsR;              // Precomputed number of bits for remainder

public:
    // Constructor
    Golomb(int m, bool decoder, string file = "golomb.txt", int mode = 0, string inputFilename = "")
        : m(m), mode(mode), bs(file, decoder), inputFilename(inputFilename), numBitsR(ceil(log2(m))) {
        if (m <= 0) {
            throw invalid_argument("Golomb parameter 'm' must be > 0.");
        }
    }

    // End of encoding
    void end() {
        bs.end();
    }

    // Zigzag encoding
    int zigzagEncode(int value) {
        return value >= 0 ? 2 * value : -2 * value - 1;
    }

    // Zigzag decoding
    int zigzagDecode(int value) {
        return (value % 2 == 0) ? (value / 2) : -(value / 2) - 1;
    }

    // Encode a single value
    void encode(int value) {
        if (mode == 0) {
            bs.writeBit(value < 0); // Sign bit
            value = abs(value);
        } else {
            value = zigzagEncode(value);
        }

        int q = value / m;
        int r = value % m;

        // Write q as unary
        for (int i = 0; i < q; ++i) {
            bs.writeBit(1);
        }
        bs.writeBit(0); // End of unary

        // Write r as binary
        bs.writeBits(r, numBitsR);
    }

    // Decode a single value
    int decode_val() {
        int q = 0;

        if (mode == 0) {
            // Sign and magnitude
            bool isNegative = bs.readBit();
            while (bs.readBit() == 1) {
                q++;
            }
            int r = bs.readBits(numBitsR);
            int magnitude = q * m + r;
            return isNegative ? -magnitude : magnitude;
        } else {
            // Zigzag interleaving
            while (bs.readBit() == 1) {
                q++;
            }
            int r = bs.readBits(numBitsR);
            int zigzagValue = q * m + r;
            return zigzagDecode(zigzagValue);
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