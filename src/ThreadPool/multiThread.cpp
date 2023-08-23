//
// Created by p.magos on 8/17/23.
//

#include <sstream>
#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <queue>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "./ThreadPool.cpp"
#include "../utils/utils.cpp"

using namespace std;

// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:t:"

void writeToFile(vector<string>* bits, const string& encodedFile, int numThreads);
void readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file, vector<int>* uAscii);
void createOutput(vector<string>* myFile, const map<int, string>& myMap, int numThreads, uintmax_t len);

ThreadPool pool;

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/ThreadPool/";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encodedFile += optarg;
                break;
            case 't':
                numThreads = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }

    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();

    vector<string> file(numThreads);
    map<int, string> myMap;
    {
        utimer timer("Total");
        {
            utimer t("Pool Start");
            pool.Start(numThreads);
        }
        {
            utimer t("Read Frequencies");
            readFrequencies(&in, numThreads, fileSize, &file, &ascii);
        }
        {
            utimer t("Create Map");
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer t("Create Output");
            createOutput(&file, myMap, numThreads, fileSize);
        }
        {
            utimer t("Write to File");
            writeToFile(&file, encodedFile, numThreads);
        }
        {
            utimer t("Pool Stop");
            pool.Stop();
        }
    }
    return 0;
}

void readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file, vector<int>* uAscii){
    // Read file
    uintmax_t size1 = len / numThreads;
    uintmax_t size = size1;
    mutex readFileMutex;
    mutex writeAsciiMutex;
    // Read file line by line
    for (int i = 0; i < numThreads; i++){
        size = (i==numThreads-1) ? len - (i*size1) : size1;
        (*file)[i] = string(size, ' ');
        pool.QueueJob([capture0 = &(*myFile),
                              capture1 = &(*file)[i],
                              i,
                              size,
                              size1,
                              capture2 = &readFileMutex,
                              capture4 = &writeAsciiMutex,
                              capture5 = &(*uAscii)] { calcChar(capture0, capture1, i, size, size1, capture2, capture4, capture5); });
    }
    while (pool.busy());
}

void createOutput(vector<string>* file, const map<int, string>& myMap, int numThreads, uintmax_t len) {
    for (int i = 0; i < numThreads; i++)
        pool.QueueJob([myMap, capture0 = &(*file)[i]] { toBits(myMap, capture0); });
    while (pool.busy());
}

void writeToFile(vector<string>* bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    mutex fileMutex;
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;
    string line;
    for (int i = 0; i < numThreads; i++) {
        Start = End;
        End = 8 - ((*bits)[i].size()-Start)%8;
        if (i == numThreads-1)
            (*bits)[i] += (string(((*bits)[i].size()-Start)%8, '0'));
        pool.QueueJob([Start, End, capture0 = &(*bits), i, writePos, capture1 = &outputFile, capture2 = &fileMutex]{
            wWrite(Start, End, capture0, i, writePos, ref(*capture1), ref(*capture2));
        });
        writePos += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
}