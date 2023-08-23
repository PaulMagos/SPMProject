//
// Created by Paul Magos on 19/08/23.
//
#include "./fastflow/ff/ff.hpp"
#include "../utils/utimer.cpp"
#include <iostream>
#include <fstream>
#include <utility>
#include <map>
#include <vector>
#include <mutex>
#include <getopt.h>


#define ASCII_MAX 256
// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:t:"

using namespace std;
//using namespace ff;


int main(int argc, char* argv[])
{

    char option;
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/FF/";

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

    for (int i = 0; i < ASCII_MAX; i++) {
        myMap[i] = bitset<8>(i).to_string();
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

void toBits(map<int, string> myMap, string* line){
    string bits;
    for (char j : *line) {
        bits.append(myMap[j]);
    }
    *line = bits;
}

void wWrite(uint8_t Start, uint8_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream& outputFile, mutex& fileMutex){
    string output;
    uint8_t value = 0;
    int i;
    for (i = Start; i < (*bits)[pos].size(); i+=8) {
        for (uint8_t n = 0; n < 8; n++)
            value = ((*bits)[pos][i+n]=='1') | value << 1;
//            value |= ((*bits)[pos][i+n]=='1') << n;
        output.append((char*) (&value), 1);
        value = 0;
    }
    if (pos != (*bits).size()-1) {
        for (uint8_t n = 0; n < 8-End; n++)
            value = ((*bits)[pos][i-8+n]=='1') | value << 1;
//            value |= ((*bits)[pos][i-8+n]=='1') << n;
        for (i = 0; i < End; i++)
            value = ((*bits)[pos+1][i]=='1') | value << 1;
//            value |= ((*bits)[pos+1][i]=='1') << i;
        output.append((char*) (&value), 1);
    }
    {
        unique_lock<mutex> lock(fileMutex);
        outputFile.seekp(writePos/8);
        outputFile.write(output.c_str(), output.size());
    }
}