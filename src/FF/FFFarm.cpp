//
// Created by Paul Magos on 19/08/23.
//
#include "./fastflow/ff/ff.hpp"
#include <ff/parallel_for.hpp>
#include "../utils/utimer.cpp"
#include "../utils/Node.h"
#include "../utils/utils.cpp"
#include <iostream>
#include <fstream>
#include <utility>
#include <thread>
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

template<typename Base, typename T>
inline bool instanceof(const T *ptr) {
    return dynamic_cast<const Base*>(ptr) != nullptr;
}

class Task{
public:
    virtual ~Task() = default;
};

namespace Ttask{
    class ffFreq;
        class Ttask::ffFreq : public Task{
        public:
            explicit ffFreq(ifstream* myFile, vector<string>* file, int i, int nw, uintmax_t len, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* uAscii) :
                    myFile(myFile), file(file), i(i), nw(nw), len(len), readFileMutex(readFileMutex), writeAsciiMutex(writeAsciiMutex), uAscii(uAscii) {}
            ifstream* myFile;
            vector<string>* file;
            int i;
            int nw;
            uintmax_t len;
            mutex* readFileMutex;
            mutex* writeAsciiMutex;
            vector<int>* uAscii;
        };
    class ffBits;
        class Ttask::ffBits : public Task{
        public:
            explicit ffBits(map<int, string>& myMap, string* file) :
                    myMap(myMap), file(file) {}
            map<int, string> myMap;
            string* file;
        };
    class ffWrite;
        class Ttask::ffWrite : public Task {
        public:
            explicit ffWrite(uintmax_t Start, uintmax_t End, vector<string>* bits, int pos, uintmax_t writePos, ofstream* outputFile, mutex* fileMutex) :
            Start(Start), End(End), bits(bits), pos(pos), writePos(writePos), outputFile(outputFile), fileMutex(fileMutex) {}
            uintmax_t Start;
            uintmax_t End;
            vector<string>* bits;
            int pos;
            uintmax_t writePos;
            ofstream* outputFile;
            mutex* fileMutex;
        };
};


static inline Task* Wrapper(Task* task, ff_node *const){
    if (instanceof<Ttask::ffFreq>(task)) {
        auto *task1 = (Ttask::ffFreq *) task;
        calcChar(task1->myFile, task1->file, task1->i, task1->nw, task1->len, task1->readFileMutex,
                 task1->writeAsciiMutex, task1->uAscii);
    }
    else if (instanceof<Ttask::ffBits>(task)){
        auto *task1 = (Ttask::ffBits*) task;
        toBits(task1->myMap, task1->file);
    }
    return task;
}


int main(int argc, char* argv[])
{
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    map<int, string> myMap;

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



    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    vector<string> file(NUM_OF_THREADS);
    uintmax_t fileSize = in.tellg();

    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;

    ff_Farm<Task> farm(Wrapper, NUM_OF_THREADS, true);
    ff_Farm<Task> farm1(Wrapper, NUM_OF_THREADS, true);
    farm.run();
    farm1.run();

    {
        utimer timer("Total");

        /* -----------------        FREQ        ----------------- */
        {
            utimer timr("Freq");
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                farm.offload(new Ttask::ffFreq(&in, &file, i, NUM_OF_THREADS, fileSize, &readFileMutex, &writeAsciiMutex, &ascii));
            }
            farm.offload(farm.EOS);
            farm.wait();
            farm.stop();
        }

        /* -----------------        HUFFMAN        ----------------- */
        {
            utimer timr("Huffman");
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        /* -----------------        MAP        ----------------- */
        {
            utimer timr("Map");
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                farm1.offload(new Ttask::ffBits(myMap, &file[i]));
            }
            farm1.offload(farm.EOS);
            farm1.wait();
            farm1.stop();
        }

        /* -----------------        OUTPUT        ----------------- */
        {
            utimer timr("Output");
            uintmax_t writePos = 0;
            uint8_t Start, End = 0;
            mutex writefileMutex;
            ofstream outputFile(encodedFile, ios::binary | ios::out);
            vector<uintmax_t> writePositions(NUM_OF_THREADS);
            vector<uint8_t> Starts(NUM_OF_THREADS);
            vector<uint8_t> Ends(NUM_OF_THREADS);
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                Start = End;
                Starts[i] = Ends[i-1] * (i != 0);
                if (i == NUM_OF_THREADS - 1)
                    file[i] += (string(8 - ((file[i].size() - Starts[i]) % 8), '0'));
                Ends[i] = 8 - ((file[i].size() - Starts[i]) % 8);
                End = 8 - (file[i].size() - Start) % 8;
                writePositions[i] = writePos;
                writePos += ((file[i].size() - Start) + End);
            }
            FF_PARFOR_BEGIN(test3, i, 0, NUM_OF_THREADS, 1, 1, NUM_OF_THREADS) {
                wWrite(Starts[i], Ends[i], &file, i, writePositions[i], &outputFile, &writefileMutex);
            }FF_PARFOR_END(test3);
        }
    }
    return 0;
}