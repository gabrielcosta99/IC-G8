#include <stdio.h>
#include <iostream>
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <cctype>   //to use tolower();
#include <codecvt>
#include <locale>

using namespace std;
namespace fs = filesystem;

std::string toLower(const char ch) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideStr = converter.from_bytes(str);

    // Convert to lowercase using the appropriate locale
    for (size_t i = 0; i < wideStr.size(); ++i) {
        wideStr[i] = std::towlower(wideStr[i]);
    }

    return converter.to_bytes(wideStr);
}

int main(int argc, char *argv[]){
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        vector<string> lines;
        string line;
        while(getline(file,line)){
            string lowerline="";
            for(char ch: line){
                if(ispunct(ch))
                    continue;
                lowerline+=tolower(ch);
            }
            lines.push_back(lowerline);
            lines.push_back("\n");
            // cout << line;
            // cout << "\n";
        }
        for(const string &l: lines){
            cout << l;
        }
        file.close();
    }
    else{
        printf("Error opening the file %s\n",argv[1]);
    }
    
    return 0;
}