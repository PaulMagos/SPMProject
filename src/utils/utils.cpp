//
// Created by p.magos on 8/23/23.
//
#include <iostream>
#include <fstream>
#include <vector>
#define ASCII_MAX 256
using namespace std;

void calcChar(ifstream* myFile, string* myString, int i, uintmax_t size, uintmax_t size1, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* uAscii){
    vector<int> ascii(ASCII_MAX, 0);
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * size1);
        (*myFile).read(&(*myString)[0], size);
    }
    for (char j : (*myString)) (ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((ascii)[j] != 0) {
                (*uAscii)[j] += (ascii)[j];
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

void wWrite(uintmax_t Start, uintmax_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream& outputFile, mutex& fileMutex){
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
        for (uint32_t n = 0; n < 8-End; n++)
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