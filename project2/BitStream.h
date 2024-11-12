#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <bitset>

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
            printf("Reached the end of the file\n");
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

// //Writes an integer value represented by N bits to the file, where 0 < N < 64 .
// void writeBits(){

// }

// // Reads an integer value represented by N bits from the file, where 0 < N < 64 
// int readBits(){

// }

// // Writes a string of characters to the file as a series of bits.
// void writeString(){

// }

// // Reads a string of characters from the file as a series of bits.
// string readString(){

// }
