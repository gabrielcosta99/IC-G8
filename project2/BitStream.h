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
    bit_buffer |= (bit); // set the last bit to the value of bit
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

    bool bit = (readBuffer & 0x80); // get the most significant bit
    readBuffer <<= 1;
    readBitPos--;
    
    return bit;
}

//Writes an integer value represented by N bits to the file, where 0 < N < 64 .
void writeBits(string filename, uint64_t bits, int N){
    //static_assert(N>0 && N<64, "N must be between 0 and 64");
    //static_assert(bits != NULL, "bits must not be NULL");
    
    for (int i = N - 1; i >= 0; --i) {
        bool bit = (bits >> i) & 1; // Extract the i-th bit
        writeBit(filename, bit);
        //printf("bit: %d\n",bit);
    }
}

// Reads an integer value represented by N bits from the file, where 0 < N < 64 
// Reads an integer value represented by N bits from the file, where 0 < N <= 64.
uint64_t readBits(string filename, int N) {
    assert(N > 0 && N <= 64); // Ensure valid range for N

    uint64_t result = 0; // Initialize result to zero

    for (int i = 0; i < N; ++i) {
        int bit = readBit(filename);
        //printf("bit: %d\n",bit);
        if (bit == -1) {
            return -1;// End of file
        }
        result = (result << 1) | bit; // Shift left and add the current bit
    }
    //printf("result: %s\n", bitset<8>(result).to_string().c_str());
    return result;
}


// Writes a string of characters to the file as a series of bits.
void writeString(string filename, string str){
    for (char c : str) {
        // convert the character to an 8bit unsigned integer
        writeBits(filename, c, 8);
    }

}

// Reads a string of characters from the file as a series of bits.
string readString(string filename){
    string result;
    while (true)
    {
        char c = readBits(filename, 8);
        //printf("C: %c\n",c);
        if (c == -1) {
            break;
        }
        result += c;
    }
    return result;
}
