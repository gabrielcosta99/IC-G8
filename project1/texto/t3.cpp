#include <stdio.h>
#include <iostream>
#include <codecvt>  // For UTF-8 conversion
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <map>
#include <boost/locale.hpp>
#include <time.h>
#include "matplotlibcpp.h"

using namespace std;
namespace fs = filesystem;
namespace plt = matplotlibcpp;

// double calcEntropy(map<wchar_t, double> charMap){
//     double entropy = 0.0;
//     for(const auto &pair: charMap){
//         double px = pair.second;
//         entropy-= px * log2(px);
//     }
//     return entropy;
// }

int runT3(int argc, char *argv[]){

    map<wchar_t, double> charMap;

    wifstream file(argv[1]);

    // Set the file to read UTF-8
    file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));

    wstring line;
    int nChars = 0;
    clock_t t = clock();
    while(getline(file,line)){
        // convert the line to lowercase
        wstring lowerline = boost::locale::to_lower(line, boost::locale::generator().generate(""));
        
        for(wchar_t ch: lowerline){
            // ignore spaces and punctuation
            if(!iswspace(ch) && !iswpunct(ch)) 
                charMap[ch] += 1;
                nChars+=1; 
            
        }

    }
    t = clock() - t;
    printf("It took %7.3f seconds to calculate the character frequency \n",(double)t / (double)CLOCKS_PER_SEC);
    vector<string> rawCharacterData; // vector that stores each character, a number of times equal to it's frequency

    // Converter for wide characters to UTF-8
    wstring_convert<codecvt_utf8<wchar_t>> converter;

    for (const auto& p : charMap) {
        
        // For histogram, add the character multiple times based on its frequency
        for (int i = 0; i < p.second; ++i) {
            rawCharacterData.push_back(converter.to_bytes(p.first)); // Use the numeric value of the character
        }

    }

    if ( rawCharacterData.empty()) {
        cerr << "Error: No valid characters to plot." << endl;
        return 1;
    }
    t = clock() -t;

    plt::hist(rawCharacterData,50);

    // Add labels and title
    plt::xlabel("Characters");
    plt::ylabel("Frequency");
    plt::title("Character Frequency Histogram");

    // Display the plot
    plt::show();
    cout << "Character frequencies" << endl;
    for (const auto &p: charMap)
        cout << converter.to_bytes(p.first) << " = " << p.second << endl;
    cout << "Total number of Characters: " << nChars << endl; 
    
    // double entropy = calcEntropy(charMap);
    // cout << "entropy: " << entropy << endl;
    
    file.close();
    
    return 0;
}