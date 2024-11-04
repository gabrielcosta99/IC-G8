#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <bitset>

using namespace std;
namespace fs = filesystem;

// Writes a single bit to the file.
void writeBit(string filename, bitset<1> bit){
    ofstream file(filename);
    file << bit;
    file.close();
}

// Reads a single bit from the file.
bitset<1> readBit(string filename,int bitPos){
    ifstream file(filename);
    
    file.close();
}

//Writes an integer value represented by N bits to the file, where 0 < N < 64 .
void writeBits(){

}

// Reads an integer value represented by N bits from the file, where 0 < N < 64 
int readBits(){

}

// Writes a string of characters to the file as a series of bits.
void writeString(){

}

// Reads a string of characters from the file as a series of bits.
string readString(){

}
