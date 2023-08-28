#include <iostream>
#include <sstream>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <map>
#include "utils/Node.h"
#include "utils/utimer.cpp"
#include "utils/utils.cpp"

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

void readFile(ifstream &myFile, string* file, uintmax_t size);
void count(vector<uintmax_t>* ascii, const string& file);
void createOutput(string* file, map<uintmax_t, string> myMap);
void toBytes(string* bits);
void write(const string& bits, ofstream* outputFile, uintmax_t writePos=0);

#ifdef PRINT
    bool print = true;
#else
    bool print = false;
#endif
#if defined(NO3)
    string csvPath = "./data/ResultsNO3.csv";
    #else
    string csvPath = "./data/Results.csv";
#endif


int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, encFile;
    map<uintmax_t, string> myMap;

    inputFile = "./data/TestFiles/";
    MyDir = "./data/EncodedFiles/Sequential/";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encFileName += optarg;
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }

    encFile = MyDir;
    #ifdef NO3
        encFile += "NO3";
    #endif
    encFile += encFileName;

    ifstream myFile (inputFile, ifstream::binary | ifstream::ate);
    ofstream outputFile(encFile, ios::binary | ios::out);
    uintmax_t fileSize = myFile.tellg();
    vector<long> timers(4,0);
    string file;
    cout << "Starting Sequential Test on file: " << inputFile << " Size: ~"
    << utils::ConvertSize(fileSize, 'M') << "MB" << endl;
    {
        utimer total("Total", &timers[2]);
        {
            utimer t("Read File", &timers[0]);
            readFile(myFile, &file, fileSize);
        }
        count(&ascii, file);
        Node::createMap(Node::buildTree(ascii), &myMap);
        createOutput(&file, myMap);
        toBytes(&file);
        {
            utimer timer("Write to file", &timers[1]);
            write(file, &outputFile);
        }
    }
    // Time without read and write
    timers[3] = timers[2] - timers[0] - timers[1];
    timers[0] = 0;
    timers[1] = 0;
    uintmax_t writePos = file.size()*8;
    utils::writeResults("Sequential", encFileName, fileSize, writePos, 1, timers, false, false, 0, print, csvPath);
    return 0;
}

void readFile(ifstream &myFile, string* file, uintmax_t size){
    *file = string(size, ' ');
    myFile.seekg(0);
    myFile.read(&(*file)[0], size);
}

void count(vector<uintmax_t>* ascii, const string& file){
    for (char i : file) (*ascii)[i]++;
}

void createOutput(string* inputFile, map<uintmax_t, string> myMap) {
    string tmp;
    for (auto &i : *inputFile) {
        (tmp).append(myMap[i]);
    }
    (tmp).append(string(8 - ((tmp).size() % 8), '0'));
    swap(tmp, *inputFile);
}

void toBytes(string* bits){
    string output;
    uint8_t value = 0;
    for (long long unsigned i = 0; i < (*bits).size(); i += 8) {
        for (int j = 0; j < 8; j++) {
            value = ((*bits)[i + j] == '1') | value << 1;
        }
        output.append((char *) (&value), 1);
        value = 0;
    }
    *bits = output;
}

void write(const string& bits, ofstream* outputFile, uintmax_t writePos){
    (*outputFile).seekp(writePos/8);
    (*outputFile).write((bits).c_str(), (bits).size());
}