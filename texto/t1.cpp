#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

using namespace std;
namespace fs = filesystem;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    if (!fs::is_regular_file(argv[1]))
    {
        printf("Error opening the file %s\n", argv[1]);
        return 1;
    }

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