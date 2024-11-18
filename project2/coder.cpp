#include <iostream>
#include <fstream>
#include <string>
#include "BitStream.h" // Assuming the above code is in BitStream.h

using namespace std;

void decodeFile(const string& inputFilename, const string& outputFilename) {
    BitStream bs("encoded.bin", true);
    ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        cerr << "Error: Unable to open output file!" << endl;
        return;
    }

    while (true) {
        int bit = bs.readBit();
        if (bit == -1) { // End of file
            break;
        }
        outputFile << (bit ? '1' : '0'); // Convert bit back to character
    }

    outputFile.close();
    cout << "Decoding complete. Text file written to: " << outputFilename << endl;
}


void encodeFile(const string& inputFilename, const string& outputFilename) {
    BitStream bs(outputFilename, false);
    ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open input file!" << endl;
        return;
    }

    char bitChar;
    int bitCount = 0;
    uint8_t byteBuffer = 0;

    while (inputFile >> bitChar) {
        if (bitChar != '0' && bitChar != '1') {
            cerr << "Error: Input file contains invalid character!" << endl;
            return;
        }

        // Pack the bit into the byte buffer
        byteBuffer <<= 1;
        byteBuffer |= (bitChar - '0'); // Convert '0'/'1' to 0/1
        bitCount++;

        // If we've collected 8 bits, write them out
        if (bitCount == 8) {
            bs.writeBits(byteBuffer, 8);
            byteBuffer = 0; // Reset buffer
            bitCount = 0;
        }
    }

    // Flush remaining bits if the file does not end on a byte boundary
    if (bitCount > 0) {
        byteBuffer <<= (8 - bitCount); // Pad with zeros
        bs.writeBits(byteBuffer, 8);
    }

    inputFile.close();
    cout << "Encoding complete. Output written to " << outputFilename << endl;
}

int main() {
    string inputFilename = "input.txt";
    string encodedFilename = "encoded.bin";
    string decodedFilename = "decoded.txt";

    encodeFile(inputFilename, encodedFilename);
    decodeFile(encodedFilename, decodedFilename);

    return 0;
}