#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <bitset>
#include <cassert>


#include "BitStream.h"

using namespace std;

class Golomb
{
    private:
        int m;

    public:
        BitStream bitStream;

        Golomb(int m){
            this->m = m;
        };
        void end(){
            bitStream.end();
        };
        void encode(int value){
            int q = value / m;
            int r = value % m;
            int q_bits = q;
            if (q_bits == 0){
                bitStream.writeBit(0);
            } else {
                bitStream.writeBit(1);
                bitStream.writeBits(q_bits, q);
            }
            bitStream.writeBits(r, m);
        };
        int decode(){
            int q = 0;
            while (bitStream.readBit() == 1){
                q++;
            }
            if (q == 0){
                return bitStream.readBits(m);
            } else {
                int q_bits = bitStream.readBits(q);
                int r = bitStream.readBits(m);
                return q_bits * m + r;
            }
        };
        void print();

};