#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <bitset>
#include <cassert>

using namespace std;
namespace fs = filesystem;

static char bit_buffer;
static int current_bit=0;

static int readBitPos=0;
static char readBuffer;
static string currentFilename = "";
static ifstream inputFile;

// Writes a single bit to the file.
void writeBit(string filename, bool bit){
    bit_buffer <<= 1;
    bit_buffer |= (bit);
    // printf("bit: %d,  (binary: %s)\n",bit,bitset<8>(bit_buffer).to_string().c_str());
    current_bit++;
    if(current_bit == 8){
        ofstream file(filename,ios::binary | ios::app);
        // printf("writing: %c, (binary: %s)\n",bit_buffer,bitset<8>(bit_buffer).to_string().c_str());
        // file << bit_buffer;
        file.put(bit_buffer);
        bit_buffer=0;
        current_bit = 0;
        file.close();
    }
    
}
void flush_Bits (void)
{
    while (current_bit)
        writeBit ("test.bin",0);
}

// Reads a single bit from the file.
char readBit(string filename){
    if(filename != currentFilename ){
        if(inputFile.is_open()) 
            inputFile.close();
        currentFilename = filename;
        inputFile.open(filename, ios::binary);
        // printf("HERE\n");
    }
    if (readBitPos == 0){
        inputFile.get(readBuffer);
        if(inputFile.eof()){
            //printf("Reached the end of the file\n");
            return -1;
        }
            
        readBitPos = 8;
        // printf("here\n");
    }

    bool bit = (readBuffer & 0x80);
    readBuffer <<= 1;
    readBitPos--;
    
    return bit;
}

//Writes an integer value represented by N bits to the file, where 0 < N < 64 .
void writeBits(string filename, int bits, int N){
    //static_assert(N>0 && N<64, "N must be between 0 and 64");
    //static_assert(bits != NULL, "bits must not be NULL");
    bool bitsArray[N];
    for(int i=0; i<=N; i++){
        bool bit = bits%10;
        bitsArray[i] = bit;
        bits = bits/10;
        // printf("value: %d\n",bit);
    }
    printf("\n\n");
    for(int i=N-1; i>=0; i--){
        writeBit(filename,bitsArray[i]);
        printf("bitsArray[i]: %d\n",bitsArray[i]);
    }

}

// Reads an integer value represented by N bits from the file, where 0 < N < 64 
int readBits(string filename, int N){
    //static_assert(N>0 && N<64, "N must be between 0 and 64");
    int bits = 0;
    for(int i=0; i<N; i++){
        bits *=10;
        bool value = readBit(filename);
        printf("value:%d\n",value);
        bits += value;
        printf("bits now: %d\n",bits);
    }
    printf("bits:%d\n",bits);
    return bits;
}

// // Writes a string of characters to the file as a series of bits.
// void writeString(){

// }

// // Reads a string of characters from the file as a series of bits.
// string readString(){

// }
