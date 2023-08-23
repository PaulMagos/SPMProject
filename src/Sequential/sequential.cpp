#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <map>
#include <queue>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"

using namespace std;

#define ASCII_MAX 256
// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:"

void readFrequencies(vector<int>* ascii, ifstream &myFile);
void writeToFile(const string& bits, const string& encodedFile);
void createOutput(const string& inputFile, map<int, string> myMap, string* encodedFile);

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Sequential/";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encodedFile += optarg;
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }


    map<int, string> myMap;
    ifstream myFile (inputFile);
    string output;
    {
        utimer total("Total");
        {
            utimer t("Calculate freq");
            readFrequencies(&ascii, myFile);
        }
        {
            utimer t("CreateMap");
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer timer("Create output");
            createOutput(inputFile, myMap, &output);
        }
        {
            utimer timer("Write to file");
            writeToFile(output, encodedFile);
        }
    }
    return 0;
}

void readFrequencies(vector<int>* ascii, ifstream &myFile){
    // Read file
    while(myFile){
        char c = myFile.get();
        (*ascii)[c]++;
    }
}

void createOutput(const string& inputFile, map<int, string> myMap, string* encodedFile) {
    ifstream myFile (inputFile);
    while(myFile){
        char c = myFile.get();
        (*encodedFile).append(myMap[c]);
    }
    (*encodedFile).append(string(8 - (*encodedFile).size() % 8, '0'));
}

void writeToFile(const string& bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    uint8_t value = 0;
    for (long long unsigned i = 0; i < bits.size(); i += 8) {
        for (int j = 0; j < 8; j++) {
            value = (bits[i + j]) | value << 1;
        }
        outputFile.write((char*) (&value), 1);
        value = 0;
    }
}