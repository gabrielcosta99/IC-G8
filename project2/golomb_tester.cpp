#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Golomb.h"
#include <iomanip> // For std::setw
#include <bitset>
using namespace std;

void runGolombTester(const string &inputFile, int m, int mode = 0) {
    // Step 1: Read integers from the input file
    ifstream input(inputFile);
    if (!input.is_open()) {
        cerr << "Error: Unable to open input file!" << endl;
        return;
    }

    vector<int> originalIntegers;
    int value;

    while (input >> value) { // Read signed integers directly
        originalIntegers.push_back(value);
    }
    input.close();

    // Debugging: Print the read values
    cout << "Read integers from file: ";
    for (const int &val : originalIntegers) {
        cout << val << " ";
    }
    cout << endl;

    // Step 2: Encode integers (in-memory encoding)
    cout << "Using Golomb parameter m = " << m << " and mode = " << (mode == 0 ? "Sign/Magnitude" : "Zigzag Interleaving") << endl;
    vector<int> encodedData; // Simulating encoded data for demonstration
    {
        Golomb encoder(m,false,mode); // Initialize Golomb encoder with parameter m

        for (const int &val : originalIntegers) {
            encoder.encode(val); // Encode each value
        }
        encoder.end(); // Finalize encoding
    }
    cout << "Encoding completed.\n";

    // Step 3: Decode integers
    vector<int> decodedValues;
    {
        Golomb decoder(m,true,mode); // Initialize Golomb decoder with parameter m

        for (size_t i = 0; i < originalIntegers.size(); ++i) {
            int decodedValue = decoder.decode(); // Decode each value
            decodedValues.push_back(decodedValue);
        }
    }
    cout << "Decoding completed.\n";

    cout << "Decoded values: ";
    for (const int &val : decodedValues) {
        cout << val << " ";
    }
    cout << endl;

    // Step 4: Verify correctness
    cout << "Original Value   | Decoded Value    | Match\n";
    cout << "------------------------------------------\n";

    bool allMatch = true;
    for (size_t i = 0; i < originalIntegers.size(); ++i) {
        bool match = (originalIntegers[i] == decodedValues[i]);
        cout << std::setw(15) << originalIntegers[i] << " | "
             << std::setw(15) << decodedValues[i] << " | "
             << (match ? "Yes" : "No") << "\n";

        if (!match) {
            allMatch = false;
        }
    }

    if (allMatch) {
        cout << "\nAll values matched successfully!\n";
    } else {
        cout << "\nThere were mismatches in encoding/decoding.\n";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input_file> <m> <mode>\n";
        return 1;
    }

    string inputFile = argv[1];
    int m = stoi(argv[2]);
    int mode = stoi(argv[3]);

    runGolombTester(inputFile, m, mode);

    return 0;
}
