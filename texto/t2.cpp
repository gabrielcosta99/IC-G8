#include <stdio.h>
#include <iostream>
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <boost/locale.hpp>

using namespace std;
namespace fs = filesystem;


int main(int argc, char *argv[]){
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        vector<string> lines;
        string line;
        while(getline(file,line)){
            line = boost::locale::to_lower(line, boost::locale::generator().generate(""));
            string lowerline = "";
            for(char ch: line){
                if(ispunct(ch))
                    continue;
                lowerline+=ch;

            }
            lines.push_back(lowerline);
            lines.push_back("\n");

        }
        for(string l: lines){
            cout << l;
        }
        file.close();
    }
    else{
        printf("Error opening the file %s\n",argv[1]);
    }
    
    return 0;
}