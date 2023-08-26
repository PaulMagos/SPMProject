#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <mutex>
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

void writeToFile(vector<string>* bits, const string& encodedFile);
void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii);
void createOutput(vector<string>* myFile, const map<uintmax_t, string>& myMap);

int NUM_OF_THREADS = 4;

int main(int argc, char* argv[])
{
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/Threads/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/Threads.csv";

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

    vector<long> timers(5, 0);
    vector<string> file(NUM_OF_THREADS);
    {
        utimer timer("Total", &timers[4]);
        {
            utimer t("Read Frequencies", &timers[0]);
            readFrequencies(&in, fileSize, &file, &ascii);
        }
        {
            utimer t("Create Map", &timers[1]);
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer t("Create output", &timers[2]);
            createOutput(&file, myMap);
        }
        {
            utimer t("Write to file", &timers[3]);
            writeToFile(&file, encFile);
        }
    }
    string csv;
    uintmax_t writePos = 0;
    for (int i = 0; i < NUM_OF_THREADS; i++) writePos += file[i].size();
    csv.append(encFileName).append(";");
    csv.append(to_string(fileSize)).append(";");
    csv.append(to_string(writePos/8)).append(";");
    csv.append(to_string(NUM_OF_THREADS)).append(";");
    for (long timer : timers) csv.append(to_string(timer)).append(";");
    appendToCsv(csvFile, csv);
    return 0;
}

void readFrequencies(ifstream* myFile, uintmax_t len, vector<string>* file, vector<uintmax_t>* uAscii){
    // Read file
    mutex readFileMutex;
    mutex writeAsciiMutex;
    vector<std::thread> threads;
    for (int i = 0; i < NUM_OF_THREADS; i++){
        threads.emplace_back([capture0 = &(*myFile),
                                     capture1 = &(*file),
                                     i,
                                     nw=NUM_OF_THREADS,
                                     len,
                                     capture2 = &readFileMutex,
                                     capture4 = &writeAsciiMutex,
                                    capture5 = &(*uAscii)] {
            return calcChar(capture0, capture1, i, nw, len, capture2, capture4, capture5); });
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}

void createOutput(vector<string>* file, const map<uintmax_t, string>& myMap) {
    vector<std::thread> threads;
    threads.reserve(NUM_OF_THREADS);
    for (int i = 0; i < NUM_OF_THREADS; i++)
        threads.emplace_back([myMap, capture0 = &(*file), i] { return toBits(myMap, capture0, i); });
    for (int i = 0; i < NUM_OF_THREADS; i++) {
            threads[i].join();
    }
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
        End = (8 - ((*bits)[i].size()-Start)%8)%8;
        if (i == NUM_OF_THREADS-1)
            (*bits)[i] += (string(((*bits)[i].size()-Start)%8, '0'));
        threads.emplace_back(wWrite, Start, End, &(*bits), i, writePos, &outputFile, &fileMutex);
        writePos += (((*bits)[i].size()-Start)+End);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        threads[i].join();
    }
}