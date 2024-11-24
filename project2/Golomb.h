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

    public:
        

        Golomb(int m,bool decoder):m(m) ,bs("golomb.txt",decoder){};

        void end(){
            bs.end();
        };


        uint64_t decToBinary(int n, int size) {
            vector<uint64_t> binaryDigits;
            uint64_t binaryNum = 0;
            uint64_t i = 0;
            // Convert to binary and store each digit in a vector
            while (n > 0) {
                uint64_t bit = n % 2;
                binaryDigits.push_back(bit);
                n /= 2;
                i++;
                size--;
            }
            for(; size>0;size--){
                binaryDigits.push_back(0);
            }

            // Print binary digits in reverse order
            for (int j = binaryDigits.size() - 1; j >= 0; j--) {
                // printf("%ld", binaryDigits[j]);
                binaryNum = (binaryNum << 1) | binaryDigits[j]; // Construct binary number
            }
            // printf("\n");

            return binaryNum;
        }

        void encode(int value) {
            int q = value / m;
            int r = value % m;
            int numBitsR = log2(m);

            cout << "Encoding value: " << value << "\n";
            //cout << "Quotient (q): " << q << ", Remainder (r): " << r << ", Bits for r: " << numBitsR << "\n";

            // Write q as unary
            for (int i = 0; i < q; ++i) {
                bs.writeBit(1);
            }
            bs.writeBit(0); // End of unary

            // Write r in binary
            bs.writeBits(r, numBitsR);

            //cout << "Encoded bits written for r: " << std::bitset<8>(r).to_string() << "\n";
        }


        int decode() {
            int q = 0;
        
            // Read unary for q
            while (bs.readBit() == 1) {
                q++;
            }
        
            // Read binary for r
            int r = bs.readBits(log2(m));
        
            //cout << "Decoded q: " << q << ", Decoded r: " << r << "\n";
        
            int value = q * m + r;
            cout << "Decoded value: " << value << endl;
        
            return value;
        }

        void print();

};