#include <stdio.h>
#include "BitStream.h"
#include <cassert>

using namespace std;

int main(int argc, char* argv[]){
    char filename[] = "out.bin";
    FILE* file = fopen(filename, "wb"); // Open in binary mode for initial clearing
    fclose(file);  // Close immediately to reset the file
    // 72 = H
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

}

