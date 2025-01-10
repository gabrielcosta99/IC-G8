#include<stdio.h>
#include <fstream>
#include <iostream>
#include "./t2.cpp"
using namespace std;

int main(int argc, char* argv[]){
    if (argc < 2) {
        cout << "Usage: ./program <audio_file>" << endl;
        return 1;
    }
    const char* filename = argv[1];
    t2(filename);
    return 0;
}