#include <stdio.h>
#include <iostream>
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <cctype>   //to use tolower();
#include <codecvt>
#include <locale>
#include <map>

using namespace std;
namespace fs = filesystem;

int main(int argc, char *argv[]){
    map<char, int> m;
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        vector<string> lines;
        string line;
        while(getline(file,line)){
            string lowerline="";
            for(char ch: line){
                if(ispunct(ch))
                    continue;
                ch = tolower(ch);
                lowerline+=ch;
                if(ch == ' ') continue;
                if(m.find(ch) == m.end()){
                    m.insert(pair<char,int>(ch,1));
                }
                else{
                    m.at(ch)+=1;                    
                }
            }
            lines.push_back(lowerline);
            lines.push_back("\n");
            // cout << line;
            // cout << "\n";
        }
        for(const string &l: lines){
            cout << l;
        }

        cout << "Character frequencies" << endl;
        for (auto it = m.begin(); it != m.end(); ++it)
            cout << it->first << " = " << it->second << endl;
        file.close();
    }
    else{
        printf("Error opening the file %s\n",argv[1]);
    }
    
    return 0;
}