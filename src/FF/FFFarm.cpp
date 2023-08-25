//
// Created by Paul Magos on 19/08/23.
//
#include <ff/ff.hpp>
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

template<typename Base, typename T>
inline bool instanceof(const T *ptr) {
    return dynamic_cast<const Base*>(ptr) != nullptr;
}

class Task{
public:
    virtual ~Task() = default;
};

namespace Ttask{
    class ffFreq  : public Task{
        public:
            explicit ffFreq(ifstream* myFile, vector<string>* file,
                            int i, int nw, uintmax_t len, mutex* readFileMutex,
                            mutex* writeAsciiMutex, vector<uintmax_t>* uAscii) :
                    myFile(myFile), file(file), i(i), nw(nw), len(len),
                    readFileMutex(readFileMutex), writeAsciiMutex(writeAsciiMutex), uAscii(uAscii) {}
            ifstream* myFile;
            vector<string>* file;
            int i;
            int nw;
            uintmax_t len;
            mutex* readFileMutex;
            mutex* writeAsciiMutex;
            vector<uintmax_t>* uAscii;
        };
    class ffMap : public Task{
        public:
            explicit ffMap(vector<uintmax_t>& ascii, map<uintmax_t, string>* myMap) :
                    ascii(ascii), myMap(myMap) {}
            vector<uintmax_t> ascii;
            map<uintmax_t, string>* myMap;
        };
    class ffBits : public Task{
        public:
            explicit ffBits(map<uintmax_t, string>& myMap, vector<string>* file, int i) :
                    myMap(myMap), file(file), i(i) {}
            map<uintmax_t, string> myMap;
            vector<string>* file;
            int i;
        };
    class ffWrite : public Task {
        public:
            explicit ffWrite(uintmax_t Start, uintmax_t End, vector<string>* bits,
                             int pos, uintmax_t writePos, ofstream* outputFile, mutex* fileMutex) :
            Start(Start), End(End), bits(bits), pos(pos), writePos(writePos),
            outputFile(outputFile), fileMutex(fileMutex) {}
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
        calcChar(((Ttask::ffFreq *) task)->myFile, ((Ttask::ffFreq *) task)->file, ((Ttask::ffFreq *) task)->i,
                 ((Ttask::ffFreq *) task)->nw, ((Ttask::ffFreq *) task)->len, ((Ttask::ffFreq *) task)->readFileMutex,
                 ((Ttask::ffFreq *) task)->writeAsciiMutex, ((Ttask::ffFreq *) task)->uAscii);
    }else if (instanceof<Ttask::ffBits>(task)){
        toBits(((Ttask::ffBits*) task)->myMap, ((Ttask::ffBits*) task)->file, ((Ttask::ffBits*) task)->i);
    }else if (instanceof<Ttask::ffWrite>(task)){
        wWrite(((Ttask::ffWrite*) task)->Start, ((Ttask::ffWrite*) task)->End, ((Ttask::ffWrite*) task)->bits,
               ((Ttask::ffWrite*) task)->pos, ((Ttask::ffWrite*) task)->writePos, ((Ttask::ffWrite*) task)->outputFile,
               ((Ttask::ffWrite*) task)->fileMutex);
    }else if (instanceof<Ttask::ffMap>(task)){
        Node::createMap(Node::buildTree(((Ttask::ffMap*) task)->ascii), ((Ttask::ffMap*) task)->myMap);
    }
    return task;
}


int main(int argc, char* argv[])
{
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    map<uintmax_t, string> myMap;

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
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    uintmax_t writePos = 0;
    uint8_t Start, End = 0;

    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;
    mutex writefileMutex;

    ff_Farm<Task> farm(Wrapper, NUM_OF_THREADS, true);
    ff_Farm<Task> farm1(Wrapper, NUM_OF_THREADS, true);
    ff_Farm<Task> farm2(Wrapper, NUM_OF_THREADS, true);
    ff_node_F<Task> node(Wrapper);
    farm.run();
    farm1.run();
    farm2.run();


    cout << "NUM OF THREADS: " << NUM_OF_THREADS << endl;
    cout << "Started Input FileSize: " << fileSize << endl;
    {
        utimer timer("Total");
        /* -----------------        FREQUENCIES        ----------------- */
        {
            utimer timer2("Read Frequencies");
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                farm.offload(new Ttask::ffFreq(&in, &file, i, NUM_OF_THREADS, fileSize, &readFileMutex, &writeAsciiMutex, &ascii));
            }
            farm.offload(farm.EOS);
            farm.wait();
            farm.stop();
        }
        /* -----------------        Tree        ----------------- */
        {
            utimer timer3("Tree");
            node.svc(new Ttask::ffMap(ascii, &myMap));
        }

        /* -----------------        Encode        ----------------- */
        {
            utimer timer4("Encode");
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                farm1.offload(new Ttask::ffBits(myMap, &file, i));
            }
            farm1.offload(farm.EOS);
            farm1.wait();
            farm1.stop();
        }
        /* -----------------        OUTPUT        ----------------- */
        {
            utimer timer5("Write");
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                Start = End;
                if (i == NUM_OF_THREADS - 1)
                    file[i] += (string(8 - ((file[i].size() - Start) % 8), '0'));
                End = 8 - (file[i].size() - Start) % 8;
                farm2.offload(new Ttask::ffWrite(Start, End, &file, i, writePos, &outputFile, &writefileMutex));
                writePos += ((file[i].size() - Start) + End);
            }
            farm2.offload(farm2.EOS);
            farm2.wait();
            farm2.stop();
        }
    }
    cout << "Finished Encoded FileSize: " << writePos/8 << endl;

    return 0;
}