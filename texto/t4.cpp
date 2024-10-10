#include <stdio.h>
#include <iostream>
#include <fstream>    // to read the file
#include <filesystem> // to check if it is a file or if the file exists
#include <vector>
#include <map>
#include <sstream>
#include <boost/locale.hpp>

#include "matplotlibcpp.h"

using namespace std;
namespace fs = filesystem;
namespace plt = matplotlibcpp;

bool cmp(pair<wstring, int> &a,
         pair<wstring, int> &b)
{
    return a.second > b.second;
}

// Function to sort the map according
// to value in a (key-value) pairs
vector<pair<wstring, int>> sortMap(map<wstring, int> &M)
{

    // Declare vector of pairs
    vector<pair<wstring, int>> A;

    // Copy key-value pair from Map
    // to vector of pairs
    for (auto &it : M)
    {
        A.push_back(it);
    }

    // Sort using comparator function
    sort(A.begin(), A.end(), cmp);

    // // Print the sorted value
    // for (auto& it : A) {

    //     cout << it.first << ' '
    //         << it.second << endl;
    // }

    return A;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    map<wstring, int> m;
    locale::global(locale("en_US.UTF-8")); // Set the global locale
    wcout.imbue(locale("en_US.UTF-8"));    // Set the locale for wcout
    if (fs::is_regular_file(argv[1]))
    {
        wifstream file(argv[1]);

        // Set the file to read UTF-8
        file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));

        wstring line;
        int nWords = 0;
        while (getline(file, line))
        {
            line = boost::locale::to_lower(line, boost::locale::generator().generate(""));

            // Create a C locale object (locale_t)
            locale_t c_locale = newlocale(LC_ALL_MASK, "en_US.UTF-8", nullptr);

            // Convert wstring to string
            // wstring_convert<codecvt_utf8<wchar_t>> converter;
            // string utf8_line = converter.to_bytes(line);

            wstringstream ss(line);
            wstring word;
            bool tag = false;
            // wcout << line << endl;

            while (ss >> word)
            {

                wstring lowerWord = L"";
                for (wchar_t ch : word)
                {
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

                    if(tag){
                        break;
                    }
                    else if(iswalpha_l(ch,c_locale) || (ch == L'-' && lowerWord.length()>0)){ // if the character is a letter, or if it is an "-" like in "trata-se", add the character to the word
                        lowerWord += ch;
                    }else{ // if the word contains non-letters, skip the word
                        break;
                    }

                }
                if (!lowerWord.empty() && lowerWord != L"-")
                {
                    // wcout << L"Inserting word: " << lowerWord << endl;
                    m[lowerWord] += 1;

                    nWords += 1;
                }
            }
            freelocale(c_locale);
        }
        // initiate the sorted map
        vector<pair<wstring, int>> sortedMap = sortMap(m);

        vector<string> characters;
        vector<double> frequencies;
        int index = 0;
        vector<string> rawCharacterData; // For histogram
        vector<double> indices; // Numeric indices for the x-axis
        wstring_convert<codecvt_utf8<wchar_t>> converter;

        int count = 0;
        for (const auto &p : sortedMap)
        {
            // "p" is the pair (key,value) of the map "m"

            if(count == 15)
                break;
            characters.push_back(converter.to_bytes(p.first));
            frequencies.push_back((double)p.second / nWords);
            indices.push_back(index++); // Generate numeric index for each character
             // For histogram, add the character multiple times based on its frequency
            for (int i = 0; i < p.second; i++) {
                rawCharacterData.push_back(converter.to_bytes(p.first)); // Use the numeric value of the character
                wcout << L"Inserting word: \"" << p.first << L"\" Frequency: " << p.second << endl;
            }
            count++;
        }
        
        plt::hist(rawCharacterData,10);
        // plt::bar(indices,frequencies);

        // Set the x-axis labels to characters
        plt::xticks(indices, characters);

        // Add labels and title
        plt::xlabel("Characters");
        plt::ylabel("Frequency");
        plt::title("Character Frequency Bar Chart");

        // Display the plot
        plt::show();

        wcout << "Character frequencies" << endl;
        for (auto it = m.begin(); it != m.end(); ++it)
            wcout << it->first << " = " << (double)it->second  << endl;
        
        file.close();
    }
    else
    {
        printf("Error opening the file %s\n", argv[1]);
    }

    return 0;
}