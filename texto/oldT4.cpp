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
    map<string, int> m;
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        vector<string> lines;
        string line;
        int nWords = 0;
        string word = "";
        string lowerText = "";
        for(istreambuf_iterator<char> it(file), end; it != end; ++it){
            char ch = *it;
            if(ispunct(ch))
                continue;
            ch = tolower(ch);
            lowerText+=ch;
            if(ch == ' ' || ch == '\n'){
                if(m.find(word) == m.end()){
                    m.insert(pair<string,int>(word,1));
                }
                else{
                    m.at(word)+=1;                    
                }
                word = "";
            }
            else{
                word+=ch;
            }
            
            nWords+=1;
        }
        cout << lowerText << endl;
        cout << "Character frequencies" << endl;
        for (auto it = m.begin(); it != m.end(); ++it)
            cout << it->first << " = " << (double) it->second/nWords << endl;
        file.close();
    }
    else{
        printf("Error opening the file %s\n",argv[1]);
    }
    
    return 0;
}