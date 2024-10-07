#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

using namespace std;
namespace fs = filesystem;

int main(int argc, char *argv[]){
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        for(istreambuf_iterator<char> it(file), end; it != end; ++it){
            cout << *it;
        }
        // vector<string> lines;
        // string line;
        // while(getline(file,line)){
        //     lines.push_back(line);
        //     lines.push_back("\n");
        //     // cout << line;
        //     // cout << "\n";
        // }
        // for(const string &l: lines){
        //     cout << l;
        // }
        file.close();
    }
    else{
        printf("Error opening the file %s\n",argv[1]);
    }
    
    return 0;
}