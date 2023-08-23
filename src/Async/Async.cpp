#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <future>
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
#define OPT_LIST "hi:p:t:"

void writeToFile(const string& bits, const string& encodedFile, int numThreads);
vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file);
string createOutput(vector<string>* file, const map<int, string>& myMap, int numThreads);
vector<future<void>> futures;

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

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
    string output;
    {
        utimer timer("Total");
        {
            utimer t("Read Frequencies");
            ascii = readFrequencies(&in, numThreads, fileSize, &file);
        }
        {
            utimer t("Create Map");
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer t("Create Output");
            output = createOutput(&file, myMap, numThreads);
        }
        {
            utimer t("Write To File");
            writeToFile(output, encodedFile, numThreads);
        }
    }
    return 0;
}

void calcChar(ifstream* myFile, string* myString, int i, uintmax_t size, uintmax_t size1, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* ascii, vector<int>* uAscii){
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * size1);
        (*myFile).read(&(*myString)[0], size);
    }
    for (char j : (*myString)) (*ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((*ascii)[j] != 0) {
                (*uAscii)[j] += (*ascii)[j];
            }
        }
    }
}

vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file){
    // Read file
    uintmax_t size1 = len / numThreads;
    mutex readFileMutex;
    mutex writeAsciiMutex;
    vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
    vector<int> uAscii(ASCII_MAX, 0);
    // Read file line by line
    for (int i = 0; i < numThreads; i++){
        uintmax_t size = (i==numThreads-1) ? len - (i*size1) : size1;
        (*file)[i] = string(size, ' ');
        futures.emplace_back(async(launch::async, calcChar, &(*myFile), &(*file)[i], i, size, size1, &readFileMutex, &writeAsciiMutex, &ascii[i], &uAscii));
    }
    for (int i = 0; i < numThreads; i++) {
        futures[i].get();
    }
    return uAscii;
}

void toBits(map<int, string> myMap, string* line){
    string bits;
    for (char j : *line) {
        bits.append(myMap[j]);
    }
    *line = bits;
}

string createOutput(vector<string>* file, const map<int, string>& myMap, int numThreads) {
    // Read file
    futures = vector<future<void>>();
    for (int i = 0; i < numThreads; i++){
        futures.emplace_back(async(launch::async, toBits, myMap, &(*file)[i]));
    }
    string output;
    for (int i = 0; i < numThreads; i++) {
        futures[i].get();
        output.append((*file)[i]);
    }
    while (output.size()%8 != 0)
        output.push_back('0');
    return output;
}

void writeToFile(const string& bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    mutex fileMutex;
    vector<std::thread> threads;
    uintmax_t Start = (((bits.size() - (bits.size() % 8)) / 8) / numThreads + 1) * 8;
    uintmax_t chunkSize = Start;
    for (int i = 0; i < numThreads; i++) {
        chunkSize += (i==numThreads-1) ? bits.size() - ((i+1)*Start) : 0;
        threads.emplace_back([&bits, start=i*Start, chunkSize, &outputFile, &fileMutex]() {
            string output;
            uint8_t value = 0;
            for(int j = 0; j<chunkSize; j+=8){
                for (uint8_t n = 0; n < 8; n++)
                    value = (bits[start+j+n] == '1') | value << 1;
                output.append((char*) (&value), 1);
                value = 0;
            }
            {
                unique_lock<mutex> lock(fileMutex);
                outputFile.seekp(start/8);
                outputFile.write(output.c_str(), output.size());
            }
        });
    }
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}