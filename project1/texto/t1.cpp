#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

using namespace std;
namespace fs = filesystem;

int runT1(int argc, char *argv[])
{

    ifstream file(argv[1]);
    vector<string> lines;
    string line;
    while (getline(file, line))
    {
        lines.push_back(line);
        lines.push_back("\n");
    }
    for (const string &l : lines)
    {
        cout << l;
    }
    file.close();

    return 0;
}