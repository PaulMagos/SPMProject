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
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "./ThreadPool2.cpp"
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

void writeToFile(vector<string>* bits, const string& encodedFile);
void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii);
void createOutput(vector<string>* myFile, const map<uintmax_t, string>& myMap, uintmax_t len);

ThreadPool pool;
int NUM_OF_THREADS = 4;

int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/ThreadPool2/";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encodedFile += optarg;
                break;
            case 't':
                NUM_OF_THREADS = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }


    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();

    vector<string> file(NUM_OF_THREADS);
    map<uintmax_t, string> myMap;
    {
        utimer timer("Total");
        {
            utimer t("Pool Start");
            pool.Start(NUM_OF_THREADS);
        }
        {
            utimer t("Read Frequencies");
            readFrequencies(&in, fileSize, &file, &ascii);
        }
        {
            utimer t("Create Map");
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer t("Create Output");
            createOutput(&file, myMap, fileSize);
        }
        {
            utimer t("Write to File");
            writeToFile(&file, encodedFile);
        }
        {
            utimer t("Pool Stop");
            pool.Stop();
        }
    }
    return 0;
}

void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii){
    // Read file
    mutex readFileMutex;
    mutex writeAsciiMutex;
    // Read file line by line
    for (int i = 0; i < NUM_OF_THREADS; i++){
        pool.QueueJob([capture0 = &(*myFile),
                              capture1 = &(*file),
                              i,
                              nw=NUM_OF_THREADS,
                              len,
                              capture2 = &readFileMutex,
                              capture4 = &writeAsciiMutex,
                              capture5 = &(*uAscii)] {
            return calcChar(capture0, capture1, i, nw, len, capture2, capture4, capture5); });
    }
    while (pool.busy());
}

void createOutput(vector<string>* file, const map<uintmax_t, string>& myMap, uintmax_t len) {
    for (int i = 0; i < NUM_OF_THREADS; i++)
        pool.QueueJob([myMap, capture0 = &(*file)[i]] { return toBits(myMap, capture0); });
    while (pool.busy());
}

void writeToFile(vector<string>* bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(NUM_OF_THREADS);
    mutex fileMutex;
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;
    string line;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        Start = End;
        if (i == NUM_OF_THREADS-1)
            (*bits)[i] += (string(8-((*bits)[i].size()-Start)%8, '0'));
        End = 8 - ((*bits)[i].size()-Start)%8;
        pool.QueueJob([Start, End, capture0 = &(*bits), i, writePos, capture1 = &outputFile, capture2 = &fileMutex]{
            wWrite(Start, End, capture0, i, writePos, capture1, capture2);
        });
        writePos += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
}