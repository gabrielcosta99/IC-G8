#include <stdio.h>
#include "BitStream.h"
#include "Golomb.h"
#include <cassert>

using namespace std;

int main(int argc, char* argv[]){
    // char filename[] = "out.bin";
    // FILE* file = fopen(filename, "wb"); // Open in binary mode for initial clearing
    // fclose(file);  // Close immediately to reset the file
    // 72 = H
    BitStream test1("out.bin",false);
    printf("Testing writeBit\n");
    test1.writeBit(1);
    test1.writeBit(0);
    test1.writeBit(0);
    test1.writeBit(1);
    test1.writeBit(0);
    test1.writeBit(0);
    test1.writeBit(0);
    test1.writeBit(0);

    test1.writeBit(0);
    test1.writeBit(0);
    test1.writeBit(0);
    test1.writeBit(1);
    test1.writeBit(0);
    test1.writeBit(0);
    printf("Passed writeBit\n");
    test1.end();

    BitStream test11("out.bin",true);
    printf("Testing readBit\n");
    assert(test11.readBit() == 1);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 1);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);

    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 1);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);
    assert(test11.readBit() == 0);

    assert(test11.readBit() == -1);
    printf("Passed readBit\n");


    // char filename2[] = "out2.bin";
    // file = fopen(filename2, "wb"); // Open in binary mode for initial clearing
    // fclose(file); // Close immediately to reset the file

    BitStream test2("out2.bin",false);

    printf("\nTesting writeBits\n");
    test2.writeBits(0b10101010, 8); 
    test2.writeBits(0b01010101, 8);
    test2.writeBits(0b11110000, 8);
    test2.writeBits(0b00001111, 8);
    test2.writeBits(0b11111011, 8);
    test2.writeBits(0b000110, 3);
    printf("Passed writeBits\n");
    test2.end();

    BitStream test21("out2.bin",true);

    printf("Testing readBits\n");
    assert(test21.readBits(8) == 0b10101010);
    assert(test21.readBits(8) == 0b01010101);
    assert(test21.readBits(8) == 0b11110000);
    assert(test21.readBits(8) == 0b00001111);
    assert(test21.readBits(8) == 0b11111011);
    assert(test21.readBits(3) == 0b110);
    printf("Passed readBits\n");

    // char filename3[] = "out3.bin";
    // file = fopen(filename3, "wb"); // Open in binary mode for initial clearing
    // fclose(file); // Close immediately to reset the file


    BitStream test3("out3.bin",false);
    printf("\nTesting writeString\n");
    test3.writeString("Hello World\n");
    printf("Passed writeString\n");

    test3.end();
    BitStream test31("out3.bin",true);
    printf("Testing readString\n");
    assert(test31.readString() == "Hello World\n");
    printf("Passed readString\n");




    printf("\nTesting Golomb\n");
    Golomb g1(8,false);
    g1.encode(46);
    g1.end();
    Golomb g11(8,true);
    // printf("reading: %d\n",bs.readBits(8) == 0b11111011);
    // printf("final test: %d\n",g.decode());
    assert(g11.decode() == 46);
    g11.end();

    Golomb g2(16,false);
    g2.encode(35);
    g2.end();

    Golomb g21(16,true);
    assert(g21.decode() == 35);
    g21.end();
    printf("Passed Golomb\n");
    printf("\nPassed all tests\n");

    // assert(g.decode() == 45);    

    //printf("Testing flush_Bits\n");
}

