#include <stdio.h>
#include <iostream>
#include <fstream>      // to read the file
#include <filesystem>   // to check if it is a file or if the file exists
#include <vector>
#include <map>
#include <sstream>

using namespace std;
namespace fs = filesystem;

int main(int argc, char *argv[]){
    map<string, int> m;
    if(fs::is_regular_file(argv[1])){
        ifstream file(argv[1]);
        string line;
        int nWords = 0;
        while(getline(file,line)){
            stringstream ss(line);
            string word;
            bool tag = false;
            while(ss >> word){
                string lowerWord = "";
                for(char ch: word){
                    if(ch == '<'){
                        tag = true;
                        continue;
                    }
                    else if(ch == '>'){
                        tag=false;
                        continue;
                    }
                    if(ispunct(ch) || tag)
                        continue;
                    lowerWord+= tolower(ch);
                    
                }
                if(lowerWord != ""){
                    if(m.find(lowerWord) == m.end()){
                        m.insert(pair<string,int>(lowerWord,1));
                    }
                    else{
                        m.at(lowerWord) += 1;
                    }
            
                    nWords+=1;
                }
            }
          
        }

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