#include <stdio.h>
#include <iostream>
#include <codecvt>  // For UTF-8 conversion
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <map>
#include <boost/locale.hpp>

#include "matplotlibcpp.h"

using namespace std;
namespace fs = filesystem;
namespace plt = matplotlibcpp;

int main(int argc, char *argv[]){
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    if(!fs::is_regular_file(argv[1])){
        printf("Error opening the file %s\n",argv[1]);
        return 1;
    }
    map<wchar_t, int> charMap;

    wifstream file(argv[1]);

    // Set the file to read UTF-8
    file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));

    wstring line;
    int nChars = 0;
    while(getline(file,line)){
        // convert the line to lowercase
        line = boost::locale::to_lower(line, boost::locale::generator().generate(""));
        
        string lowerline = "";
        for(wchar_t ch: line){
         
            lowerline+=ch;
            // ignore spaces and punctuation
            if(iswspace(ch) || iswpunct(ch)) 
                continue;

            charMap[ch] += 1;
            nChars+=1;
            
        }
        // cout << line;
        // cout << "\n";

    }
    vector<string> characters;
    vector<string> rawCharacterData; // For histogram


    //values needed for a bar graph
    /*
    vector<double> frequencies;
    int index = 0;
    vector<double> indices;  // Numeric indices for the x-axis
    */
    
    

    // Converter for wide characters to UTF-8
    wstring_convert<codecvt_utf8<wchar_t>> converter;

    for (const auto& p : charMap) {
        // "p" is the pair (key,value) of the map "charMap"
        
        characters.push_back(converter.to_bytes(p.first));

        // values needed for a bar graph
        /*
        frequencies.push_back((double) p.second);
        indices.push_back(index++);  // Generate numeric index for each character
        */
     

        // For histogram, add the character multiple times based on its frequency
        for (int i = 0; i < p.second; ++i) {
            rawCharacterData.push_back(converter.to_bytes(p.first)); // Use the numeric value of the character
        }
    }

    if (indices.empty() || characters.empty()) {
        cerr << "Error: No valid characters to plot." << endl;
        return 1;
    }

    // for(string c: characters)
    //     cout << c << endl;


    plt::hist(rawCharacterData,30);


    // Set the x-axis labels to characters
    // plt::xticks(indices, characters);

    // Add labels and title
    plt::xlabel("Characters");
    plt::ylabel("Frequency");
    plt::title("Character Frequency Bar Chart");

    // Display the plot
    plt::show();
    
    // cout << "Character frequencies" << endl;
    // for (const auto &p: charMap)
    //     cout << converter.to_bytes(p.first) << " = " << (double) p.second/nChars << endl;
    
    
    file.close();
    
    return 0;
}