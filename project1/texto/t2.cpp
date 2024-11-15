#include <stdio.h>
#include <iostream>
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <boost/locale.hpp>

using namespace std;
namespace fs = filesystem;


int runT2(int argc, char *argv[]){

    ifstream file(argv[1]);
    vector<string> lines;
    string line;
    while(getline(file,line)){
        string lowerline = boost::locale::to_lower(line, boost::locale::generator().generate(""));
        string parsedLine = "";
        for(char ch: lowerline){
            if(!ispunct(ch))
                parsedLine+=ch;
        }
        lines.push_back(parsedLine+"\n");

    }
    
    for(string l: lines){
        cout << l;
    }
    file.close();

    return 0;
}