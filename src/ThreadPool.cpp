//
// Created by p.magos on 8/17/23.
//

#include <sstream>
#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include "utils/Node.h"
#include "utils/utimer.cpp"
#include "utils/Pool.cpp"
#include "utils/utils.cpp"

using namespace std;

// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * t number of threads
 * n number of tasks
 */
#define OPT_LIST "hi:p:t:n:"

void writeToFile(vector<string>* bits, const string& encodedFile);
void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii);
void createOutput(vector<string>* myFile, const map<uintmax_t,string>& myMap);
void tobytes(vector<string>* bits, vector<uintmax_t>* writePositions, uintmax_t* writePos);
void count(vector<uintmax_t>* ascii, vector<string>* file);
ThreadPool pool;
int NUM_OF_THREADS = thread::hardware_concurrency();
int Tasks = 0;

#if defined(MINE)
    bool myImpl = true;
#else
    bool myImpl = false;
#endif
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

    /* -----------------        VARIABLES                     ----------------- */
    char option;
    uintmax_t writePos = 0;
    vector<long> timers(4, 0);
    map<uintmax_t, string> myMap;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, encFile;

    /* -----------------        PATHS                         ----------------- */
    MyDir = "./data/EncodedFiles/ThreadPool/";
    inputFile = "./data/TestFiles/";

    /* -----------------        OPTIONS PARSING               ----------------- */
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
            case 'n':
                Tasks = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }


    /* -----------------         READ FILE                    ----------------- */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();

    /* -----------------         NUMBER OF T and Tasks        ----------------- */
    if(Tasks==0) utils::optimal(&Tasks, &NUM_OF_THREADS, fileSize);

    /* -----------------         ENCODED FILE PATH            ----------------- */
    encFile = MyDir;
    #ifdef NO3
        encFile += "NO3";
    #endif
    #if defined(MINE)
        encFile += "Best_";
    #endif
    encFile += "t_" + to_string(NUM_OF_THREADS) + "_n_" + to_string(Tasks) + "_" + encFileName;


    /* -----------------              VARIABLES               ----------------- */
    vector<string> file(Tasks);
    vector<uintmax_t> writePositions(Tasks);

    /* -----------------              PRINT INFO              ----------------- */
    if(print) cout << "Starting ThreadPool Test with " << NUM_OF_THREADS << " threads, on file: "
    << inputFile << " Size: ~" << utils::ConvertSize(fileSize, 'M') << "MB" << endl;

    /* -----------------                ENCODE FILE           ----------------- */
    {
        utimer timer("Total", &timers[2]);
        // Start thread pool
        pool.Start(NUM_OF_THREADS);
        #if not defined(MINE)
            /* READ AND WRITE ARE NOT CONSIDERED AS A UNIQUE OPERATION SINCE WE
             *
             * CONSIDER THEM AS A PART OF THE PROCESS
             *
             * WE READ ONE STRING AND IMMEDIATELY PASS IT FOR COUNTING
             *
             * WHILE ANOTHER THREAD IS READING A NEW STRING
             *
             * ** SIMILAR FOR THE WRITE OPERATION **
             */
            /* -----------------        READ AND COUNT        ----------------- */
            {
                utimer t("Read File", &timers[0]);
                readFrequencies(&in, fileSize, &file, &ascii);
            }

            /* -----------------        CREATE MAP            ----------------- */
            Node::createMap(Node::buildTree(ascii), &myMap);

            /* -----------------        CREATE OUTPUT         ----------------- */
            createOutput(&file, myMap);

            /* -----------------        TO BYTES              ----------------- */
            {
                utimer t("Write to File", &timers[1]);
                writeToFile(&file, encFile);
            }
            // Encoded file dim
            for (int i = 0; i < Tasks; i++) writePos += file[i].size();
        #else
            /* -----------------        READ FILE             ----------------- */
            {
                utimer t("Read File", &timers[0]);
                utils::read(fileSize, &in, &file, Tasks);
            }

            /* -----------------        COUNT                 ----------------- */
            count(&ascii, &file);

            /* -----------------        CREATE MAP            ----------------- */
            Node::createMap(Node::buildTree(ascii), &myMap);

            /* -----------------        CREATE OUTPUT         ----------------- */
            createOutput(&file, myMap);

            /* -----------------        TO BYTES              ----------------- */
            tobytes(&file, &writePositions, &writePos);

            /* -----------------        WRITE TO FILE         ----------------- */
            {
                utimer t("Write to File", &timers[1]);
                utils::write(encFile, file, writePositions, Tasks);
            }
        #endif
        // Stop thread pool
        pool.Stop();
    }

    /* -----------------      TIME WITHOUT READ AND WRITE     ----------------- */
    timers[3] = timers[2] - timers[0] - timers[1];
    timers[0] = 0;
    timers[1] = 0;

    /* -----------------            WRITE RESULTS             ----------------- */
    string kind= "ThreadPool";
    if(myImpl) kind += "M";
    utils::writeResults(kind, encFileName, fileSize, writePos, NUM_OF_THREADS, timers, true, myImpl, Tasks, print, csvPath);
    return 0;
}

void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii){
    // Read file
    mutex readFileMutex;
    mutex writeAsciiMutex;
    // Read file line by line
    for (int i = 0; i < Tasks; i++){
        pool.QueueJob([capture0 = &(*myFile),
                              capture1 = &(*file),
                              i,
                              nw = Tasks,
                              len,
                              capture2 = &readFileMutex,
                              capture4 = &writeAsciiMutex,
                              capture5 = &(*uAscii)] { utils::calcChar(capture0, capture1, i, nw, len, capture2, capture4, capture5); });
    }
    while (pool.busy());
}

void count(vector<uintmax_t>* ascii, vector<string>* file){
    mutex writeAsciiMutex;
    for (int i = 0; i < Tasks; i++){
        pool.QueueJob([capture0 = &(*file)[i],
                              capture2 = &(*ascii),
                              &writeAsciiMutex] { utils::countFrequency(capture0, capture2, &writeAsciiMutex); });
    }
    while (pool.busy());
}

void createOutput(vector<string>* file, const map<uintmax_t,string>& myMap) {
    for (int i = 0; i < Tasks; i++)
        pool.QueueJob([&myMap, capture0 = &(*file), i] { utils::toBits(myMap, capture0, i); });
    while (pool.busy());
}

void writeToFile(vector<string>* bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(Tasks);
    mutex fileMutex;
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;
    string line;
    for (int i = 0; i < Tasks; i++) {
        Start = End;
        if (i == Tasks-1)
            (*bits)[i] += (string(8-((*bits)[i].size()-Start)%8, '0'));
        End = (8 - ((*bits)[i].size()-Start)%8)%8;
        pool.QueueJob([Start, End, capture0 = &(*bits), i, writePos, capture1 = &outputFile, capture2 = &fileMutex]{
            utils::wWrite(Start, End, capture0, i, writePos, capture1, capture2);
        });
        writePos += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
}

void tobytes(vector<string>* bits, vector<uintmax_t>* writePositions, uintmax_t* writePos){
    vector<string> output(Tasks);
    vector<std::thread> threads;
    uint8_t Start, End = 0;
    for (int i = 0; i < Tasks; i++) {
        Start = End;
        End = (8 - ((*bits)[i].size()-Start)%8)%8;
        if (i == Tasks-1)
            (*bits)[i] += (string(8-((*bits)[i].size()-Start)%8, '0'));
        (*writePositions)[i] = (*writePos);
        pool.QueueJob([Start, End, bits1 = &(*bits), i, capture1 = &output[i]]{
            utils::toByte(Start, End, bits1, i, capture1);
        });
        (*writePos) += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
    *bits = output;
}