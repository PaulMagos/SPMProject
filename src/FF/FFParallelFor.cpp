//
// Created by p.magos on 8/25/23.
//
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include "../utils/utimer.cpp"
#include "../utils/Node.h"
#include "../utils/utils.cpp"
#include <iostream>
#include <thread>
#include <fstream>
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
int NUM_OF_THREADS = thread::hardware_concurrency();
#ifdef PRINT
    bool print = true;
#else
    bool print = false;
#endif

int main(int argc, char* argv[]){
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/FFParFor/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/FFParFor";
    #if defined(ALL)
        csvFile += (ALL? "All":"");
    #endif
    csvFile += ".csv";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encFileName += optarg;
                break;
            case 't':
                NUM_OF_THREADS = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }
    #if not defined(ALL)
        encFile = MyDir + encFileName;
    #else
        encFile = MyDir + (ALL? "All":"") + encFileName;
    #endif
    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    vector<string> file(NUM_OF_THREADS);
    uintmax_t fileSize = in.tellg();
    uintmax_t writePos = 0;
    vector<uintmax_t> writePositions(NUM_OF_THREADS);
    vector<long> timers = vector<long>(8, 0);
    cout << "Starting FastFlow ParallelFor Test with " << NUM_OF_THREADS << " threads on file: "
         << inputFile << " Size: ~" << ConvertSize(fileSize, 'M') << "MB" << endl;
    {
        utimer timer("Total", &timers[6]);
        /* Count frequencies */
        {
            utimer timer1("Read File", &timers[0]);
            read(fileSize, &in, &file, NUM_OF_THREADS);
        }
    #if defined(ALL)
        {
            utimer timer2("Count Frequencies", &timers[1]);
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                countFrequency(&file[i], &ascii, &writeAsciiMutex);
            }, NUM_OF_THREADS);
        }
        {
            utimer timer3("Create Map", &timers[2]);
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer timer4("Encode", &timers[3]);
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                toBits(myMap, &file, i);
            }, NUM_OF_THREADS);
        }
        {
            utimer timer5("To Bytes", &timers[4]);
            uint8_t Start, End = 0;
            vector<uint8_t> Starts(NUM_OF_THREADS), Ends(NUM_OF_THREADS);
            vector<string> output(NUM_OF_THREADS);
            for (int i = 0; i < NUM_OF_THREADS; ++i) {
                Start = End;
                Starts[i] = Start;
                if(i==NUM_OF_THREADS-1)
                    file[i].append(string(8-((file[i].size() - Start)%8), '0'));
                End = (8 - (file[i].size()-Start)%8) % 8;
                Ends[i] = End;
                writePositions[i] = writePos;
                writePos += (file[i].size()-Start)+End;
            }
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                toByte(Starts[i], Ends[i], &file, i, &output[i]);
            }, NUM_OF_THREADS);
            file = output;
        }
        #else
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                countFrequency(&file[i], &ascii, &writeAsciiMutex);
            }, NUM_OF_THREADS);
            Node::createMap(Node::buildTree(ascii), &myMap);
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                toBits(myMap, &file, i);
            }, NUM_OF_THREADS);
            uint8_t Start, End = 0;
            vector<uint8_t> Starts(NUM_OF_THREADS), Ends(NUM_OF_THREADS);
            vector<string> output(NUM_OF_THREADS);
            for (int i = 0; i < NUM_OF_THREADS; ++i) {
                Start = End;
                Starts[i] = Start;
                if(i==NUM_OF_THREADS-1)
                    file[i].append(string(8-((file[i].size() - Start)%8), '0'));
                End = (8 - (file[i].size()-Start)%8) % 8;
                Ends[i] = End;
                writePositions[i] = writePos;
                writePos += (file[i].size()-Start)+End;
            }
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                toByte(Starts[i], Ends[i], &file, i, &output[i]);
            }, NUM_OF_THREADS);
            file = output;
        #endif
        {
            utimer timer6("Write to file", &timers[5]);
            ofstream outputFile(encFile, ios::binary | ios::out);
            write(encFile, file, writePositions, NUM_OF_THREADS);
        }
    }

    timers[7] = timers[6] - timers[0] - timers[5];
    #if not defined(ALL)
        timers[0] = 0;
        timers[5] = 0;
    #endif

    /* -----------------        CSV        ----------------- */
    writeResults(encFileName, fileSize, writePos, NUM_OF_THREADS, timers, csvFile, false,
    #ifdef ALL
         true
    #else
    false
    #endif
    , false,0, print);
    return 0;
}