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
        

        Golomb(int m):m(m) ,bs("golomb.txt",false){};

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

        void encode(int value){
            int q = value / m;
            int r = value % m;
            int q_bits = q;
            if (q_bits == 0){
                bs.writeBit(0);
            } else {
                for(int i=0; i < q; i++){
                    bs.writeBit(1);
                }   
                bs.writeBit(0);
                // bs.writeBit(1);
                // bs.writeBits(q_bits, q);
            }
            uint64_t rBin = decToBinary(r,log2(m));
            
            // bs.writeBits(r, m);
            // printf("q: %d, r: %d, rBits:%ld\n",q,r,rBin);
            bs.writeBits(rBin, m);
            bs.end();
        };

        int decode(){
            //SHOULD WE CHANGE THIS?
            BitStream bsRead("golomb.txt",true);
            int q = 0;
            while (bsRead.readBit() == 1){
                q++;
            }
            if (q == 0){
                return bsRead.readBits(m);
            } else {
                // int q_bits = bsRead.readBits(q);
                int r = bsRead.readBits(m);
                // printf("q: %d,  r: %d\n",q,r);
                return q * m + r;
            }
        };
        void print();

};