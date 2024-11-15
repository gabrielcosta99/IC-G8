#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include "t1.cpp"
#include "t2.cpp"
#include "t3.cpp"
#include "t4.cpp"
#include <boost/locale.hpp>

#include "matplotlibcpp.h"

using namespace std;
namespace fs = filesystem;

int main(int argc, char *argv[])
{
    if (argc < 3 && strcmp(argv[1],"--help") != 0) {
        cerr << "Usage: " << argv[0] << " <-t1 | -t2 | -t3 | -t4> <filename>" << endl;
        return 1;
    }
   
    
    string flag = argv[1];
    argv[1] = argv[2];   
    if (flag == "--help"){
        cout << "Usage: " << argv[0] << " <-t1 | -t2 | -t3 | -t4> <filename>\n" << endl;
        cout << "  -t1  ->  prints the contents of a file" << endl;
        cout << "  -t2  ->  converts the contents of a file to lowercase and remove punctuation" << endl;
        cout << "  -t3  ->  counts the frequency of characters in a given file" << endl;
        cout << "  -t4  ->  counts the frequency of words in a given file" << endl;
        cout << "\nExample:\n  " << argv[0]<< " -t1 text_files/ep-01-01-15.txt     prints the contents of the file 'ep-01-01-15.txt'"<<endl;
        return 0;
    }

    if (!fs::is_regular_file(argv[2]))
    {
        printf("Error opening the file %s\n", argv[1]);
        return 1;
    }

    if (flag=="-t1")
        runT1(argc, argv);
    else if(flag == "-t2")
        runT2(argc,argv);
    else if(flag == "-t3")
        runT3(argc,argv);
    else if (flag == "-t4")
        runT4(argc,argv);
    else{
        cerr << "Unknown flag: " << flag << endl;
        return 1;
    }
}