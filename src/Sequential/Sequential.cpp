#include <iostream>
#include <sstream>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <map>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "../utils/utils.cpp"

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

int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    inputFile = "./data/TestFiles/";
    MyDir = "./data/EncodedFiles/Sequential/";
    csvFile = "./data/CSV/Sequential";
    #if not defined(ALL)
        csvFile += "All";
    #elif defined(ALL)
        csvFile += (ALL? "All":"");
    #endif
    csvFile += ".csv";

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

    encFile = MyDir + encFileName;

    ifstream myFile (inputFile, ifstream::binary | ifstream::ate);
    ofstream outputFile(encFile, ios::binary | ios::out);
    uintmax_t fileSize = myFile.tellg();
    vector<long> timers(8,0);
    string file;
    {
        utimer total("Total", &timers[6]);
        {
            utimer t("Read File", &timers[0]);
            readFile(myFile, &file, fileSize);
        }
        #if not defined(ALL) or ALL
            {
                utimer t("Count", &timers[1]);
                count(&ascii, file);
            }
            {
                utimer t("CreateMap", &timers[2]);
                Node::createMap(Node::buildTree(ascii), &myMap);
            }
            {
                utimer timer("Create output", &timers[3]);
                createOutput(&file, myMap);
            }
            {
                utimer timer("To bytes", &timers[4]);
                toBytes(&file);
            }
        #elif defined(ALL) and not ALL
            count(&ascii, file);
            Node::createMap(Node::buildTree(ascii), &myMap);
            createOutput(&file, myMap);
            toBytes(&file);
        #endif
        {
            utimer timer("Write to file", &timers[5]);
            write(file, &outputFile);
        }
    }
    // Time without read and write
    timers[7] = timers[6] - timers[0] - timers[5];
    #if defined(ALL) and not ALL
            timers[0] = 0;
            timers[5] = 0;
    #endif
    uintmax_t writePos = file.size()*8;
    writeResults(encFileName, fileSize, writePos, 1, timers, csvFile, false,
    #if defined(ALL)
        ALL
    #else
        true
    #endif
    );
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