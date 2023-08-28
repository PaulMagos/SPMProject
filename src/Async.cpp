#include <iostream>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <future>
#include <map>
#include "utils/Node.h"
#include "utils/utimer.cpp"
#include "utils/utils.cpp"

using namespace std;

// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:t:"

int NUM_OF_THREADS = thread::hardware_concurrency();
vector<future<void>> futures;
#if defined(PRINT)
    bool print = true;
#else
    bool print = false;
#endif
#if defined(NO3)
string csvPath = "./data/ResultsNO3.csv";
#else
string csvPath = "./data/Results.csv";
#endif

int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/Async/";
    inputFile = "./data/TestFiles/";

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
    encFile = MyDir;
    #ifdef NO3
        encFile += "NO3";
    #endif
    encFile += encFileName;

    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();
    vector<string> file(NUM_OF_THREADS);
    vector<uintmax_t> writePositions = vector<uintmax_t>(NUM_OF_THREADS);
    uintmax_t writePos = 0;


    vector<long> timers(4, 0);
    cout << "Starting Async Test with " << NUM_OF_THREADS << " futures, on file: "
   << inputFile << " Size: ~" << utils::ConvertSize(fileSize, 'M') << "MB" << endl;
    {
        utimer timer("Total", &timers[2]);
        {
            utimer t("Read File", &timers[0]);
            utils::read(fileSize, &in, &file, NUM_OF_THREADS);
        }
        mutex asciiMutex;
        for (int i = 0; i < NUM_OF_THREADS; i++)
            futures.emplace_back(async(launch::async, utils::countFrequency, &(file[i]), &ascii, &asciiMutex));
        for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
        Node::createMap(Node::buildTree(ascii), &myMap);
        futures = vector<future<void>>();
        for (int i = 0; i < NUM_OF_THREADS; i++){
            futures.emplace_back(async(launch::async, utils::toBits, myMap, &file, i));
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
        futures = vector<future<void>>();
        vector<string> output(NUM_OF_THREADS);
        uint8_t Start, End = 0;
        for (int i = 0; i < NUM_OF_THREADS; i++) {
            Start = End;
            if (i == NUM_OF_THREADS-1)
                (file)[i] += (string(8-((file)[i].size()-Start)%8, '0'));
            End = (8 - ((file)[i].size()-Start)%8) % 8;
            (writePositions)[i] = (writePos);
            futures.emplace_back(async(launch::async, utils::toByte, Start, End, &file, i, &output[i]));
            (writePos) += file[i].size() - Start + End;
        }
        for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
        file = output;
        {
            utimer t("Write To File", &timers[1]);
            utils::write(encFile, file, writePositions, NUM_OF_THREADS);
        }
    }



    // Time without read and write
    timers[3] = timers[2] - timers[0] - timers[1];
    timers[0] = 0;
    timers[1] = 0;
    utils::writeResults("Async", encFileName, fileSize, writePos, NUM_OF_THREADS, timers, false, false, 0, print, csvPath);
    return 0;
}

