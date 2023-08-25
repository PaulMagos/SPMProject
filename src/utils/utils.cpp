//
// Created by p.magos on 8/23/23.
//
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>
#include <map>
#define ASCII_MAX 256
using namespace std;


/*
 * Read the file and count the frequencies of each character
 * @param myFile: the file to read
 * @param len: the length of the file
 * @param i: the number of the thread
 * @param nw: the number of threads
 * @param len: the length of the file
 * @param file: the vector to store the file
 * @param uAscii: the vector to store the frequencies
 * @param readFileMutex: the mutex to lock the file
 * @param writeAsciiMutex: the mutex to lock the frequencies
 * @return void
 */
void calcChar(ifstream* myFile, vector<string>* file, int i, int nw, uintmax_t len, mutex* readFileMutex, mutex* writeAsciiMutex, vector<uintmax_t>* uAscii){
    uintmax_t chunk = len / nw;
    uintmax_t size = (i == nw-1) ? len - (nw-1) * chunk : chunk;
    (*file)[i] = string(size, ' ');
    vector<int> ascii(ASCII_MAX, 0);
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * chunk);
        (*myFile).read(&(*file)[i][0], size);
    }
    for (char j : (*file)[i]) (ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((ascii)[j] != 0) {
                (*uAscii)[j] += (ascii)[j];
            }
        }
    }
}

void toBits(map<uintmax_t, string> myMap, string* line){
    string bits;
    for (char j : *line) {
        bits.append(myMap[j]);
    }
    *line = bits;
}

void toBits(map<uintmax_t, string> myMap, vector<string>* line, int i){
    string bits;
    for (char j : (*line)[i]) {
        bits.append(myMap[j]);
    }
    (*line)[i] = bits;
}

void wWrite(uint8_t Start, uint8_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream* outputFile, mutex* fileMutex){
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
        for (uint32_t n = 0; n < 8-End; n++)
            value = ((*bits)[pos][i-8+n]=='1') | value << 1;
        for (i = 0; i < End; i++)
            value = ((*bits)[pos+1][i]=='1') | value << 1;
        output.append((char*) (&value), 1);
    }
    {
        unique_lock<mutex> lock((*fileMutex));
        (*outputFile).seekp(writePos/8);
        (*outputFile).write(output.c_str(), output.size());
    }
}