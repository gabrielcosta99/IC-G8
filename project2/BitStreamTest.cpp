#include <stdio.h>
#include "BitStream.h"
#include <cassert>

using namespace std;

int main(int argc, char* argv[]){
    char filename[] = "out.bin";
    FILE* file = fopen(filename, "wb"); // Open in binary mode for initial clearing
    fclose(file);  // Close immediately to reset the file
    // 72 = H
    printf("Testing writeBit\n");
    writeBit(filename,1);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,1);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,0);

    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,1);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,0);
    writeBit(filename,0);
    printf("Passed writeBit\n");

    printf("Testing readBit\n");
    assert(readBit(filename) == 1);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 1);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);

    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 1);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);
    assert(readBit(filename) == 0);

    assert(readBit(filename) == -1);
    printf("Passed readBit\n");


    char filename2[] = "out2.bin";
    file = fopen(filename2, "wb"); // Open in binary mode for initial clearing
    fclose(file); // Close immediately to reset the file

    printf("Testing writeBits\n");
    writeBits(filename2, 0b10101010, 8); 
    writeBits(filename2, 0b01010101, 8);
    writeBits(filename2, 0b11110000, 8);
    writeBits(filename2, 0b00001111, 8);
    printf("Passed writeBits\n");

    printf("Testing readBits\n");
    assert(readBits(filename2, 8) == 0b10101010);
    assert(readBits(filename2, 8) == 0b01010101);
    assert(readBits(filename2, 8) == 0b11110000);
    assert(readBits(filename2, 8) == 0b00001111);
    printf("Passed readBits\n");

    char filename3[] = "out3.bin";
    file = fopen(filename3, "wb"); // Open in binary mode for initial clearing
    fclose(file); // Close immediately to reset the file

    printf("Testing writeString\n");
    writeString(filename3, "Hello World\n");
    printf("Passed writeString\n");

    printf("Testing readString\n");
    assert(readString(filename3) == "Hello World\n");
    printf("Passed readString\n");



    printf("\nPassed all tests\n");

    //printf("Testing flush_Bits\n");
}

