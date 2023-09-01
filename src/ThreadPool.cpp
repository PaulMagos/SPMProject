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
void wWrite(uint8_t Start, uint8_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream* outputFile, mutex* fileMutex);
void calcChar(ifstream* myFile, vector<string>* file, int i, int nw, uintmax_t len, mutex* readFileMutex, mutex* writeAsciiMutex, vector<uintmax_t>* uAscii);

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
        #if not defined(TIME)
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
                              capture5 = &(*uAscii)] { calcChar(capture0, capture1, i, nw, len, capture2, capture4, capture5); });
    }
    while (pool.busy());
}

void count(vector<uintmax_t>* ascii, vector<string>* file){
    mutex writeAsciiMutex;
    for (int i = 0; i < Tasks; i++){
        pool.QueueJob([capture0 = &(*file)[i],
                              capture2 = &(*ascii),
                              capture4= &writeAsciiMutex] { utils::countFrequency(capture0, capture2, capture4); });
    }
    while (pool.busy());
}

void createOutput(vector<string>* file, const map<uintmax_t,string>& myMap) {
    for (int i = 0; i < Tasks; i++)
        pool.QueueJob([myMap, capture0 = &(*file), i] { utils::toBits(myMap, capture0, i); });
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

void tobytes(vector<string>* bits, vector<uintmax_t>* writePositions, uintmax_t* writePos){
    vector<string> output(Tasks);
    vector<std::thread> threads;
    uint8_t Start, End = 0;
    for (int i = 0; i < Tasks; i++) {
        Start = End;
        End = (8 - ((*bits)[i].size()-Start)%8)%8;
        if (i == Tasks-1)
            (*bits)[i].append(string(8-((*bits)[i].size()-Start)%8, '0'));
        (*writePositions)[i] = (*writePos);
        pool.QueueJob([Start, End, bits1 = &(*bits), i, capture1 = &output[i]]{
            utils::toByte(Start, End, bits1, i, capture1);
        });
        (*writePos) += (((*bits)[i].size()-Start)+End);
    }
    while (pool.busy());
    *bits = output;
}

void wWrite(uint8_t Start, uint8_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream* outputFile, mutex* fileMutex){
    string output;
    uint8_t value = 0;
    int i;
    for (i = Start; i < (*bits)[pos].size(); i+=8) {
        for (uint8_t n = 0; n < 8; n++)
            value = ((*bits)[pos][i+n]=='1') | value << 1;
        output.append((char*) (&value), 1);
        value = 0;
    }
    if (pos != (*bits).size()-1) {
        for (uint32_t n = 0; n < 8-End; n++)
            value = ((*bits)[pos][i-8+n]=='1') | value << 1;
        for (i = 0; i < End; i++)
            value = ((*bits)[pos+1][i]=='1') | value << 1;
        output.append((char*) (&value), 1);
    }
    {
        unique_lock<mutex> lock((*fileMutex));
        (*outputFile).seekp(writePos/8);
        (*outputFile).write(output.c_str(), output.size());
    }
}

/*
    * Read the file and count the frequencies of each character
    * @param myFile: the file to read
    * @param len: the length of the file
    * @param i: the number of the thread
    * @param nw: the number of threads
    * @param len: the length of the file
    * @param file: the vector to store the file
    * @param uAscii: the vector to store the frequencies
    * @param readFileMutex: the mutex to lock the file
    * @param writeAsciiMutex: the mutex to lock the frequencies
    * @return void
    */
void calcChar(ifstream* myFile, vector<string>* file, int i, int nw, uintmax_t len, mutex* readFileMutex, mutex* writeAsciiMutex, vector<uintmax_t>* uAscii){
    uintmax_t chunk = len / nw;
    uintmax_t size = (i == nw-1) ? len - (nw-1) * chunk : chunk;
    (*file)[i] = string(size, ' ');
    vector<int> ascii(ASCII_MAX, 0);
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * chunk);
        (*myFile).read(&(*file)[i][0], size);
    }
    for (char j : (*file)[i]) (ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((ascii)[j] != 0) {
                (*uAscii)[j] += (ascii)[j];
            }
        }
    }
}