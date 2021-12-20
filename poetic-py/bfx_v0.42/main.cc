#include <iostream>
#include <fstream>
#include "compiler/ccparser.h"
using namespace std;

int main(int argc, char **argv) { try
{
    if (argc < 2)
    {
        cout << "Syntax: " << argv[0] << " <BrainFix files (.bfx)> <BrainFuck file>\n";
        return 1;
    }

    vector<ifstream*> inputFiles;
    string outputFileName = "a.bf";
    for (int i = 1; i != argc; ++i)
    {
        string fileName = argv[i];
        size_t pos = fileName.find_last_of('.');
        if (pos == string::npos)
        {
            cout << "BrainFix files should end in '.bfx'.\n";
            return 1;
        }
        
        string ext = fileName.substr(pos);
        if (ext == ".bfx")
            inputFiles.push_back(new ifstream(fileName));
        else if (outputFileName == "a.bf")
            outputFileName = argv[i];
        else
        {
            cout << "Specify at most one output-file.\n";
            return 1;
        }
    }
    
    if (inputFiles.size() < 1)
    {
        cout << "You didn't provide any BrainFix files (*.bfx)!\n";
        return 1;
    }
    
    ofstream outputFile(outputFileName);

    // preprocess all input files
    Preprocessor::Parser prep;
    for (size_t idx = 0; idx != inputFiles.size(); ++idx)
        if (prep.parse(*inputFiles[idx]))
            return 1;

    Compiler::Parser::init(30000);
    Compiler::Parser(prep, "main", outputFile).parse();
    
} catch (std::string const &msg) 
{
    cerr << msg << '\n';
}}