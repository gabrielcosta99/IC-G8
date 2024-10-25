#include <stdio.h>
#include <iostream>
#include <fstream>    // to read the file
#include <filesystem> // to check if it is a file or if the file exists
#include <vector>
#include <map>
#include <sstream>
#include <boost/locale.hpp>
#include <time.h>
#include "matplotlibcpp.h"

using namespace std;
namespace fs = filesystem;
namespace plt = matplotlibcpp;

bool cmp(pair<wstring, double> &a,
         pair<wstring, double> &b)
{
    return a.second > b.second;
}

// Function to sort the map according to value in each pair
vector<pair<wstring, double>> sortMap(map<wstring, double> &M)
{

    // Declare vector of pairs
    vector<pair<wstring, double>> A;

    // Copy key-value pair from Map to vector of pairs
    for (auto &it : M)
    {
        A.push_back(it);
    }

    // Sort using comparator function
    sort(A.begin(), A.end(), cmp);

    return A;
}


// double calcEntropy(map<wstring, double> charMap){
//     double entropy = 0.0;
//     for(const auto &pair: charMap){
//         double px = pair.second;
//         entropy-= px * log2(px);
//     }
//     return entropy;
// }


int runT4(int argc, char *argv[])
{

    map<wstring, double> wordMap;
    locale::global(locale("en_US.UTF-8")); // Set the global locale
    wcout.imbue(locale("en_US.UTF-8"));    // Set the locale for wcout to be able to print UTF-8 characters
    
    wifstream file(argv[1]);

    // Set the file to read UTF-8
    file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));

    wstring line;
    int nWords = 0;
    clock_t t = clock();

    while (getline(file, line))
    {
        // convert the line to lowercase
        line = boost::locale::to_lower(line, boost::locale::generator().generate(""));

        wstringstream ss(line);
        wstring word;
        bool tag = false;

        while (ss >> word)
        {

            wstring lowerWord = L"";
            for (wchar_t ch : word)
            {
                // if the character is a tag, we mark a flag to ignore the words inside it
                if (ch == L'<')
                {
                    tag = true;
                    break;
                }
                else if (ch == L'>')
                {
                    tag = false;
                    break;
                }
                // if(iswdigit(ch) || ch == L'/'){ // if the word contains digits 
                //     lowerWord = L"";
                //     break;
                // }

                // if the character is inside of a tag, we ignore the whole word
                if(tag){
                    lowerWord = L"";
                    break;
                }
                else if(iswalpha(ch) || (ch == L'-' && lowerWord.length()>0)){ // if the character is a letter, or if it is an "-" like in "trata-se", add the character to the word
                    lowerWord += ch;
                }else{ // if the word contains non-letters, skip the word
                    lowerWord = L"";
                    break;
                }

            }

            if (!lowerWord.empty())
            {
                wordMap[lowerWord] += 1.0;
                nWords += 1;
            }
        }
    }
    t = clock() - t;
    printf("It took %7.3f seconds to calculate the character frequency \n",(double)t / (double)CLOCKS_PER_SEC);
   

    // initiate the sorted map
    vector<pair<wstring, double>> sortedMap = sortMap(wordMap);

    vector<string> words;
    vector<double> frequencies;
    int index = 0;
    vector<string> rawCharacterData; // For histogram
    vector<double> indices; // Numeric indices for the x-axis
    wstring_convert<codecvt_utf8<wchar_t>> converter;

    int count = 0;
    for (const auto &p : sortedMap)
    {
        // "p" is the pair (key,value) of the map "wordMap"

        if(count == 15)
            break;
        words.push_back(converter.to_bytes(p.first));
        indices.push_back(index++); // Generate numeric index for each character
        
        // For histogram, add the character multiple times based on its frequency
        for (int i = 0; i < p.second; i++) {
            rawCharacterData.push_back(converter.to_bytes(p.first)); // Use the numeric value of the character
            // wcout << L"Inserting word: \"" << p.first << L"\" Frequency: " << p.second << endl;
        }
        count++;
    }
    
    if (indices.empty() || words.empty()) {
        cerr << "Error: No valid words to plot." << endl;
        return 1;
    }


    plt::hist(rawCharacterData,15);

    // Add labels and title
    plt::xlabel("Words");
    plt::ylabel("Frequency");
    plt::title("Character Frequency Histogram Chart");

    // Display the plot
    plt::show();

    cout << "Character frequencies" << endl;
    for (auto it = wordMap.begin(); it != wordMap.end(); ++it)
        cout << converter.to_bytes(it->first) << " = " << (double)it->second  << endl;
    cout << "Total number of Words: " << nWords << endl; 


    // double entropy = calcEntropy(wordMap);
    // wcout << L"entropy: " << entropy << endl;

    file.close();
    

    return 0;
}