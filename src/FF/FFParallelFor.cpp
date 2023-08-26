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
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/FFParFor/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/FFParFor.csv";

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
    encFile = MyDir + encFileName;

    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    vector<string> file(NUM_OF_THREADS);
    uintmax_t fileSize = in.tellg();
    uintmax_t writePos = 0;
    vector<long> timers = vector<long>(5);
    {
        utimer timer("Total", &timers[4]);
        /* Count frequencies */
        {
            utimer timer2("Read Frequencies", &timers[0]);
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                calcChar(&in, &file, i, NUM_OF_THREADS, fileSize, &readFileMutex, &writeAsciiMutex, &ascii);
            }, NUM_OF_THREADS);
        }
        {
            utimer timer3("Create Map", &timers[1]);
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer timer4("Encode", &timers[2]);
            parallel_for(0, NUM_OF_THREADS, [&](const int i){
                toBits(myMap, &file, i);
            }, NUM_OF_THREADS);
        }
        {
            utimer timer5("Write File", &timers[3]);
            uint8_t Start, End = 0;
            vector<uint8_t> Starts(NUM_OF_THREADS), Ends(NUM_OF_THREADS);
            vector<uintmax_t> writePositions(NUM_OF_THREADS);
            mutex writefileMutex;
            ofstream outputFile(encFile, ios::binary | ios::out);
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
                wWrite(Starts[i], Ends[i], &file, i, writePositions[i], &outputFile, &writefileMutex);
            }, NUM_OF_THREADS);
        }
    }

    string csv;
    csv.append(encFileName).append(";");
    csv.append(to_string(fileSize)).append(";");
    csv.append(to_string(writePos/8)).append(";");
    csv.append(to_string(NUM_OF_THREADS)).append(";");
    for (long timer : timers) csv.append(to_string(timer)).append(";");
    appendToCsv(csvFile, csv);


    return 0;
}