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
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "../utils/Pool.cpp"
#include "../utils/utils.cpp"

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

ThreadPool pool;
int NUM_OF_THREADS = thread::hardware_concurrency();
int Tasks = 0;

int main(int argc, char* argv[])
{

    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/ThreadPool/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/ThreadPool.csv";

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


    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();
    if(Tasks==0) optimal(&Tasks, &NUM_OF_THREADS, fileSize);
    encFile = MyDir + "t_" + to_string(NUM_OF_THREADS) + "_n_" + to_string(Tasks) + "_" + encFileName;

    vector<long> timers(7, 0);
    vector<string> file(Tasks);
    {
        utimer timer("Total", &timers[6]);
        {
            utimer t("Pool Start", &timers[0]);
            pool.Start(NUM_OF_THREADS);
        }
        {
            utimer t("Read Frequencies", &timers[1]);
            readFrequencies(&in, fileSize, &file, &ascii);
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
            utimer t("Write to File", &timers[4]);
            writeToFile(&file, encFile);
        }
        {
            utimer t("Pool Stop", &timers[5]);
            pool.Stop();
        }
    }
    string csv;
    uintmax_t writePos = 0;
    for (int i = 0; i < Tasks; i++) writePos += file[i].size();
    csv.append(encFileName).append(";");
    csv.append(to_string(fileSize)).append(";");
    csv.append(to_string(writePos/8)).append(";");
    csv.append(to_string(NUM_OF_THREADS)).append(";");
    csv.append(to_string(Tasks)).append(";");
    for (long timer : timers) csv.append(to_string(timer)).append(";");
    appendToCsv(csvFile, csv, true);
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
                              capture5 = &(*uAscii)] { calcChar(capture0, capture1, i, nw, len, capture2, capture4, capture5); });
    }
    while (pool.busy());
}

void createOutput(vector<string>* file, const map<uintmax_t,string>& myMap) {
    for (int i = 0; i < Tasks; i++)
        pool.QueueJob([myMap, capture0 = &(*file), i] { toBits(myMap, capture0, i); });
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
            wWrite(Start, End, capture0, i, writePos, capture1, capture2);
        });
        writePos += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
}