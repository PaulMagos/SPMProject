//
// Created by p.magos on 8/25/23.
//
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include "../utils/utimer.cpp"
#include "../utils/Node.h"
#include "../utils/utils.cpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <map>
#include <vector>
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

int main(int argc, char* argv[]){
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    map<uintmax_t, string> myMap;

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

    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    vector<string> file(NUM_OF_THREADS);
    uintmax_t fileSize = in.tellg();
    {
        utimer timer("Total");
        /* Count frequencies */
        parallel_for(0, NUM_OF_THREADS, [&](const int i){
            calcChar(&in, &file, i, NUM_OF_THREADS, fileSize, &readFileMutex, &writeAsciiMutex, &ascii);
        }, NUM_OF_THREADS);
        Node::createMap(Node::buildTree(ascii), &myMap);
        parallel_for(0, NUM_OF_THREADS, [&](const int i){
            toBits(myMap, &file[i]);
        }, NUM_OF_THREADS);
    }

    for (int i = 0; i < ASCII_MAX; i++)
        if (ascii[i] != 0)
            cout << (char)i << " : " << myMap[i] << endl;
    return 0;
}