#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <queue>
#include <mutex>
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
void writeToFile2(vector<string>* bits, const string& encodedFile, int numThreads);
vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file);
void createOutput(vector<string>* myFile, const map<int, string>& myMap, int numThreads);
int main(int argc, char* argv[])
{
    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Threads/";

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
            utimer t("Create output");
            createOutput(&file, myMap, numThreads);
        }
//        {
//            utimer t("Concatenate");
//            for (auto & i : file) output.append(i);
//            output.append(string(8-(output.size()%8), '0'));
//        }
        {
            utimer t("Write to file");
//            writeToFile(output, encodedFile, numThreads);
            writeToFile2(&file, encodedFile, numThreads);
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
    uintmax_t size = len / numThreads;
    mutex readFileMutex;
    mutex writeAsciiMutex;
    vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
    vector<int> uAscii(ASCII_MAX, 0);
    vector<std::thread> threads;
    uintmax_t size1 = size;
    for (int i = 0; i < numThreads; i++){
        size = (i == numThreads - 1) ? len - (i * size1) : size1;
        (*file)[i] = string(size, ' ');
        threads.emplace_back([capture0 = &(*myFile),
                                     capture1 = &(*file)[i],
                                     i,
                                     size,
                                     size1,
                                     capture2 = &readFileMutex,
                                     capture3= &ascii[i],
                                     capture4 = &writeAsciiMutex,
                                    capture5 = &uAscii] {
            return calcChar(capture0, capture1, i, size, size1, capture2, capture4, capture3, capture5); });
    }
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
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

void createOutput(vector<string>* file, const map<int, string>& myMap, int numThreads) {
    vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back([myMap, capture0 = &(*file)[i]] { return toBits(myMap, capture0); });
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}

void writeToFile(const string& bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    mutex fileMutex;
    vector<std::thread> threads;
    uintmax_t Start = (((bits.size() - (bits.size() % 8))) / numThreads + 1);
    uintmax_t chunkSize = Start;
    for (int i = 0; i < numThreads; i++) {
        chunkSize += (i==numThreads-1) ? bits.size() - ((i+1)*Start) : 0;
        threads.emplace_back([&bits, start=i*Start, chunkSize, &outputFile, &fileMutex]() {
            string output;
            uint8_t value = 0;
            for(int j = 0; j<chunkSize; j+=8){
                for (uint8_t n = 0; n < 8; n++)
                    value = (bits[start+j+n]) | value << 1;
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

void writeToFile2(vector<string>* bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    mutex fileMutex;
    vector<std::thread> threads;
    uintmax_t Start, writePos = 0;
    uintmax_t End = 0;
    string line;
    for (int i = 0; i < numThreads; i++) {
        Start = End;
        End = 8 - ((*bits)[i].size()-Start)%8;
        if (i == numThreads-1)
            (*bits)[i] += (string(((*bits)[i].size()-Start)%8, '0'));
        threads.emplace_back([&bits, Start, End, pos=i, writePos, &outputFile, &fileMutex]() {
            string output;
            uint8_t value = 0;
            int i;
            for (i = Start; i < (*bits)[pos].size(); i+=8) {
                for (uint8_t n = 0; n < 8; n++)
                    value = ((*bits)[pos][i+n]=='1') | value << 1;
                output.append((char*) (&value), 1);
                value = 0;
            }
            if (pos != (*bits).size()-1) {
                for (uint8_t n = 0; n < 8-End; n++)
                    value = ((*bits)[pos][i-8+n]=='1') | value << 1;
                for (i = 0; i < End; i++){
                    value = ((*bits)[pos+1][i]=='1') | value << 1;
                }
                output.append((char*) (&value), 1);
            }
            {
                unique_lock<mutex> lock(fileMutex);
                outputFile.seekp(writePos/8);
                outputFile.write(output.c_str(), output.size());
            }
        });
        writePos += (((*bits)[i].size()-Start)+End);
    }
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}