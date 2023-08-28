#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <mutex>
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

void tobytes(vector<string>* bits, vector<uintmax_t>* writePositions, uintmax_t* writePos);
void Frequencies(vector<string>* file, vector<uintmax_t>* uAscii);
void createOutput(vector<string>* myFile, const map<uintmax_t, string>& myMap);

int NUM_OF_THREADS = thread::hardware_concurrency();
#ifdef PRINT
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

    MyDir = "./data/EncodedFiles/Threads/";
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

    vector<long> timers(4, 0);
    vector<string> file(NUM_OF_THREADS);
    vector<uintmax_t> writePositions(NUM_OF_THREADS);
    uintmax_t writePos = 0;

    cout << "Starting Threads Test with " << NUM_OF_THREADS << " threads, on file: "
         << inputFile << " Size: ~" << utils::ConvertSize(fileSize, 'M') << "MB" << endl;
    {
        utimer timer("Total", &timers[2]);
        {
            utimer t("Read File", &timers[0]);
            utils::read(fileSize, &in, &file, NUM_OF_THREADS);
        }
        Frequencies(&file, &ascii);
        Node::createMap(Node::buildTree(ascii), &myMap);
        createOutput(&file, myMap);
        tobytes(&file, &writePositions, &writePos);
        {
            utimer t("Write To File", &timers[1]);
            utils::write(encFile, file, writePositions, NUM_OF_THREADS);
        }
    }

    // Time without read and write
    timers[3] = timers[2] - timers[0] - timers[1];
    timers[0] = 0;
    timers[1] = 0;

    utils::writeResults("Threads", encFileName, fileSize, writePos, 1, timers, false, false, 0, print, csvPath);
    return 0;
}

void Frequencies(vector<string>* file, vector<uintmax_t>* uAscii){
    // Read file
    mutex writeAsciiMutex;
    vector<std::thread> threads;
    for (int i = 0; i < NUM_OF_THREADS; i++){
        threads.emplace_back( utils::countFrequency, &(*file)[i], &(*uAscii), &writeAsciiMutex);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}

void createOutput(vector<string>* file, const map<uintmax_t, string>& myMap) {
    vector<std::thread> threads;
    threads.reserve(NUM_OF_THREADS);
    for (int i = 0; i < NUM_OF_THREADS; i++)
        threads.emplace_back([myMap, capture0 = &(*file), i] { return utils::toBits(myMap, capture0, i); });
    for (int i = 0; i < NUM_OF_THREADS; i++) {
            threads[i].join();
    }
}

void tobytes(vector<string>* bits, vector<uintmax_t>* writePositions, uintmax_t* writePos){
    vector<string> output(NUM_OF_THREADS);
    vector<std::thread> threads;
    uint8_t Start, End = 0;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        Start = End;
        End = (8 - ((*bits)[i].size()-Start)%8)%8;
        if (i == NUM_OF_THREADS-1)
            (*bits)[i] += (string(8-((*bits)[i].size()-Start)%8, '0'));
        (*writePositions)[i] = (*writePos);
        threads.emplace_back(utils::toByte, Start, End, &(*bits), i, &output[i]);
        (*writePos) += (((*bits)[i].size()-Start)+End);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
    *bits = output;
}