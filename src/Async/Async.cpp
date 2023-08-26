#include <iostream>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <future>
#include <map>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "../utils/utils.cpp"

using namespace std;

// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:t:"
#define ALL false

void writeToFile(vector<string>* bits, const string& encodedFile);
void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii);
void encode(vector<string>* file, const map<uintmax_t, string>& myMap);
void read(vector<string>* file, uintmax_t fileSize, ifstream* in);
void count(vector<string>* file, vector<uintmax_t>* ascii);
void toAscii(vector<string>* file, uintmax_t* writePos, vector<uintmax_t>* writePositions);
void write(const string& encFile, vector<uintmax_t> writePositions, vector<string> file);

int NUM_OF_THREADS = thread::hardware_concurrency();
vector<future<void>> futures;

int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/Async/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/Async";
    csvFile += (ALL? "All":"");
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
    encFile = MyDir + encFileName;

    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();
    vector<string> file(NUM_OF_THREADS);
    vector<uintmax_t> writePositions = vector<uintmax_t>(NUM_OF_THREADS);
    uintmax_t writePos = 0;

    vector<long> timers((ALL?8:1), 0);
    #if (ALL)
        {
        utimer timer("Total", &timers[6]);
        {
            utimer t("Read File", &timers[0]);
            read(&file, fileSize, &in);
        }
        {
            utimer t("Count Frequencies", &timers[1]);
            count(&file, &ascii);
        }
        {
            utimer t("Create Map", &timers[2]);
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer t("Create Output", &timers[3]);
            createOutput(&file, myMap);
        }
        {
            utimer t("Bits To Bytes", &timers[4]);
            toAscii(file, &writePos, &writePositions);
        }
        {
            utimer t("Write To File", &timers[5]);
            ofstream outputFile(encFile, ios::binary | ios::out);
            mutex writeFileMutex;
            vector<std::thread> threads;
            for (int i = 0; i < NUM_OF_THREADS; i++)
                threads.emplace_back(writeFile, writePositions[i], file[i], &outputFile, &writeFileMutex);
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                threads[i].join();
            }

        }
    }
    // Total without read and write
    timers[7] = timers[1] + timers[2] + timers[3] + timers[4];
    #else
        read(&file, fileSize, &in);
        {
            utimer t("Total", &timers[0]);
            count(&file, &ascii);
            Node::createMap(Node::buildTree(ascii), &myMap);
            encode(&file, myMap);
            toAscii(&file, &writePos, &writePositions);
//            readFrequencies(&in, fileSize, &file, &ascii);
//            writeToFile(&file, encFile);
        }
        write(encFile, writePositions, file);
        for (int i =0; i< NUM_OF_THREADS; i++) writePos += file[i].size();
    #endif

    writeResults(encFileName, fileSize, writePos, NUM_OF_THREADS, timers, csvFile, false, ALL);
    return 0;
}

void read(vector<string>* file, uintmax_t fileSize, ifstream* in){
    uintmax_t chunk = fileSize / NUM_OF_THREADS;
    mutex readFileMutex;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        chunk = (i == NUM_OF_THREADS-1)? fileSize - (chunk * (NUM_OF_THREADS-1)) : chunk;
        futures.emplace_back(async(launch::async, readFile, &(*in), &(*file)[i], i*chunk, chunk, &readFileMutex));
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}

void count(vector<string>* file, vector<uintmax_t>* ascii){
    futures = vector<future<void>>();
    mutex asciiMutex;
    for (int i = 0; i < NUM_OF_THREADS; i++)
        futures.emplace_back(async(launch::async, countFrequency, &(*file)[i], &(*ascii), &asciiMutex));
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}


void toAscii(vector<string>* file, uintmax_t* writePos, vector<uintmax_t>* writePositions){
    futures = vector<future<void>>();
    vector<string> output(NUM_OF_THREADS);
    uint8_t Start, End = 0;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        Start = End;
        if (i == NUM_OF_THREADS-1)
            (*file)[i] += (string(8-((*file)[i].size()-Start)%8, '0'));
        End = (8 - ((*file)[i].size()-Start)%8) % 8;
        futures.emplace_back(async(launch::async, toByte, Start, End, (*file), i, &output[i]));
        (*writePositions)[i] = (*writePos);
        (*writePos) += (((*file)[i].size()-Start)+End);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        futures[i].get();
    }
    cout << output[0] << endl;
    (*file) = output;
}

void write(const string& encFile, vector<uintmax_t> writePositions, vector<string> file){
    ofstream outputFile(encFile, ios::binary | ios::out);
    mutex writeFileMutex;
    vector<std::thread> threads;
    for (int i = 0; i < NUM_OF_THREADS; i++)
        threads.emplace_back(writeFile, writePositions[i], (file)[i], &outputFile, &writeFileMutex);
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}

void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii){
    // Read file
    mutex readFileMutex;
    mutex writeAsciiMutex;
    // Read file line by line
    for (int i = 0; i < NUM_OF_THREADS; i++){
        futures.emplace_back(async(launch::async, calcChar, &(*myFile), &(*file), i, NUM_OF_THREADS, len, &readFileMutex, &writeAsciiMutex, &(*uAscii)));
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}

void encode(vector<string>* file, const map<uintmax_t, string>& myMap) {
    // Read file
    futures = vector<future<void>>();
    for (int i = 0; i < NUM_OF_THREADS; i++){
        futures.emplace_back(async(launch::async, toBits, myMap, &(*file), i));
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) futures[i].get();
}

void writeToFile(vector<string>* bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(NUM_OF_THREADS);
    mutex fileMutex;
    vector<std::thread> threads;
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;
    string line;
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        Start = End;
        if (i == NUM_OF_THREADS-1)
            (*bits)[i] += (string(8-((*bits)[i].size()-Start)%8, '0'));
        End = (8 - ((*bits)[i].size()-Start)%8) % 8;
        threads.emplace_back(wWrite, Start, End, &(*bits), i, writePos, &(outputFile), &(fileMutex));
        writePos += (((*bits)[i].size()-Start)+End);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}