//
// Created by Paul Magos on 19/08/23.
//
#include "./fastflow/ff/ff.hpp"
#include <ff/parallel_for.hpp>
#include "../utils/utimer.cpp"
#include "../utils/utils.cpp"
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
using namespace ff;

int NUM_OF_THREADS = 4;

int main(int argc, char* argv[])
{
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    map<int, string> myMap;

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
                NUM_OF_THREADS = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }



    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    vector<string> file(NUM_OF_THREADS);
    uintmax_t fileSize = in.tellg();

    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    ffTime(START_TIME);

    /* -----------------        FREQ        ----------------- */
    FF_PARFOR_BEGIN(test1, i, 0, NUM_OF_THREADS, 1, 2, NUM_OF_THREADS) {
        calcChar(&in, &file[i], i, NUM_OF_THREADS, fileSize, &readFileMutex, &writeAsciiMutex, &ascii);
    } FF_PARFOR_END(test1);
    ffTime(STOP_TIME);
    printf("Time =%g\n", ffTime(GET_TIME));

    /* -----------------        MAP        ----------------- */
    ffTime(START_TIME);
    FF_PARFOR_BEGIN(test2, i, 0, NUM_OF_THREADS, 1, 2, NUM_OF_THREADS) {
        toBits(myMap, &file[i]);
    } FF_PARFOR_END(test2);
    ffTime(STOP_TIME);
    printf("Time =%g\n", ffTime(GET_TIME));

    /* -----------------        OUTPUT        ----------------- */
    ffTime(START_TIME);
    uintmax_t writePos = 0;
    uintmax_t Start, End = 0;
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    mutex fileMutex;
    FF_PARFOR_BEGIN(test3, i, 0, NUM_OF_THREADS, 1, 2, NUM_OF_THREADS) {
        Start = End;
        End = 8 - ((file)[i].size()-Start)%8;
        wWrite(Start, End, &file, i, writePos, &outputFile, &fileMutex);
        writePos += ((file[i].size()-Start)+End);
    } FF_PARFOR_END(test3);
    ffTime(STOP_TIME);
    printf("Time =%g\n", ffTime(GET_TIME));

    return 0;
}