#include <iostream>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <future>
#include <map>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
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
void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<int>* uAscii);
void createOutput(vector<string>* file, const map<int, string>& myMap);

int NUM_OF_THREADS = 4;
vector<future<void>> futures;

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Async/";

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
    map<int, string> myMap;
    {
        utimer timer("Total");
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
            createOutput(&file, myMap);
        }
        {
            utimer t("Write To File");
            writeToFile(&file, encodedFile);
        }
    }
    return 0;
}

void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<int>* uAscii){
    // Read file
    mutex readFileMutex;
    mutex writeAsciiMutex;
    // Read file line by line
    for (int i = 0; i < NUM_OF_THREADS; i++){
        futures.emplace_back(async(launch::async, calcChar, &(*myFile), &(*file)[i], i, NUM_OF_THREADS, len, &readFileMutex, &writeAsciiMutex, &(*uAscii)));
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}

void createOutput(vector<string>* file, const map<int, string>& myMap) {
    // Read file
    futures = vector<future<void>>();
    for (int i = 0; i < NUM_OF_THREADS; i++){
        futures.emplace_back(async(launch::async, toBits, myMap, &(*file)[i]));
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}

void writeToFile(vector<string>* bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(NUM_OF_THREADS);
    mutex fileMutex;
    vector<std::thread> threads;
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;
    string line;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        Start = End;
        End = 8 - ((*bits)[i].size()-Start)%8;
        if (i == NUM_OF_THREADS-1)
            (*bits)[i] += (string(((*bits)[i].size()-Start)%8, '0'));
        threads.emplace_back(wWrite, Start, End, &(*bits), i, writePos, &(outputFile), &(fileMutex));
        writePos += (((*bits)[i].size()-Start)+End);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}