#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <map>
#include <utility>
#include <vector>
#include <getopt.h>
#include "utils/utimer.cpp"
#include "utils/utils.cpp"
#include "utils/Node.h"


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


struct ff_task_t {
    ff_task_t(vector<string>* file, vector<uintmax_t>* ascii, map<uintmax_t, string>* myMap, uintmax_t* writePos,
              vector<uintmax_t>* writePositions, vector<uintmax_t>* Starts, vector<uintmax_t>* Ends, int i) :
            file(file), ascii(ascii), myMap(myMap), writePos(writePos),
    writePositions(writePositions), Starts(Starts), Ends(Ends), index(i) {
        line = &file->operator[](i);
        localAscii = vector<uintmax_t>(ASCII_MAX, 0);
    }

    int index;
    string* line;
    vector<string>* file;
    vector<uintmax_t>* ascii;
    vector<uintmax_t> localAscii;
    map<uintmax_t, string>* myMap;
    uintmax_t* writePos;
    vector<uintmax_t>* writePositions;
    vector<uintmax_t>* Starts;
    vector<uintmax_t>* Ends;
};

namespace QUEUE {
    queue<ff_task_t*> tasks;
    mutex tasksMutex;
    condition_variable tasksCondVar;

    void enque(ff_task_t* task) {
        {
            unique_lock<mutex> lock(tasksMutex);
            tasks.push(task);
        }
        tasksCondVar.notify_one();
    }

    ff_task_t* deque() {
        ff_task_t* task;
        {
            unique_lock<mutex> lock(tasksMutex);
            tasksCondVar.wait(lock, []{ return !tasks.empty(); });
            task = tasks.front();
            tasks.pop();
        }
        return task;
    }

    bool empty(){
        return tasks.empty();
    }
}

struct Worker : ff_Map<ff_task_t> {
    ff_task_t *svc(ff_task_t *) override {
        ff_task_t* inA = QUEUE::deque();
        // this is the parallel_for provided by the ff_Map class
        for (int i = 0; i < (*inA->line).size(); i++)
            inA->localAscii[(*inA->line)[i]]++;
        ff_send_out(inA);
        return EOS;
    }
};

struct collectCounts: ff_Map<ff_task_t> {
    ff_task_t *svc(ff_task_t *inA) override {
        ff_task_t &A = *inA;
        {
            for (int i = 0; i < ASCII_MAX; i++)
                A.ascii->operator[](i) += A.localAscii[i];
        }
        ff_send_out(inA);
        return GO_ON;
    }
};

struct mapWorker : ff_Map<ff_task_t> {
    int numoftasks = 0;
    vector<ff_task_t*> tasks = vector<ff_task_t*>(NUM_OF_THREADS);
    ff_task_t *svc(ff_task_t *inA) override {
        // If farm finished then create map
        ff_task_t &A = *inA;
        numoftasks++;
        tasks[inA->index] = inA;
        if (numoftasks == NUM_OF_THREADS) {
            Node::createMap(Node::buildTree(*A.ascii), inA->myMap);
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                ff_send_out(tasks[i]);
            }
        }
        return GO_ON;
    }
};

struct mapApply : ff_Map<ff_task_t> {
    ff_task_t *svc(ff_task_t *inA) override {
        ff_task_t *A = inA;
        string tmp;
        for (int i = 0; i < (A->line)->size(); i++) {
            tmp += A->myMap->operator[]((*A->line)[i]);
        }
        *(A->line) = tmp;
        ff_send_out(A);
        return EOS;
    }
};

struct calcIndices : ff_Map<ff_task_t>{
    int numOfTasks = 0;
    vector<ff_task_t*> tasks = vector<ff_task_t*>(NUM_OF_THREADS);
    ff_task_t *svc(ff_task_t *inA) override {
        numOfTasks++;
        tasks[inA->index] = inA;
        if (numOfTasks == NUM_OF_THREADS) {
            vector<string> *file = tasks[0]->file;
            vector<uintmax_t> *writePositions = tasks[0]->writePositions;
            vector<uintmax_t> *Starts = tasks[0]->Starts;
            vector<uintmax_t> *Ends = tasks[0]->Ends;
            uintmax_t *writePos = tasks[0]->writePos;
            int Start, End = 0;
            for (int i = 0; i<NUM_OF_THREADS; i++){
                Start = End;
                (*Starts)[i] = Start;
                if(i==NUM_OF_THREADS-1)
                    (*file)[i].append(string(8-(((*file)[i].size() - Start)%8), '0'));
                End = (8 - ((*file)[i].size()-Start)%8) % 8;
                (*Ends)[i] = End;
                (*writePositions)[i] = *writePos;
                *writePos += ((*file)[i].size()-Start)+End;
            }
            for (int i = 0; i < NUM_OF_THREADS; i++) {
                ff_send_out(tasks[i]);
            }
        }
        return GO_ON;
    }
};

struct applyBit : ff_Map<ff_task_t> {
    int numoftasks = 0;
    vector<ff_task_t*> tasks = vector<ff_task_t*>(NUM_OF_THREADS);
    ff_task_t *svc(ff_task_t *inA) override {
        numoftasks++;
        tasks[inA->index] = inA;
        if (numoftasks == NUM_OF_THREADS) {
            vector<string> out = vector<string>(NUM_OF_THREADS);
            vector<string> *file = tasks[0]->file;
            vector<uintmax_t> *writePositions = tasks[0]->writePositions;
            vector<uintmax_t> *Starts = tasks[0]->Starts;
            vector<uintmax_t> *Ends = tasks[0]->Ends;
            FF_PARFOR_BEGIN(apply, i, 0, NUM_OF_THREADS, 1, 1, NUM_OF_THREADS){
                uintmax_t j = 0;
                string tmp;
                uint8_t byte = 0;
                for (j = (*Starts)[i]; j < (*file)[i].size(); j+=8) {
                    for (uintmax_t k = 0; k < 8; k++) {
                        byte = ((*file)[i][j+k]=='1') | byte << 1 ;
                    }
                    tmp.append((char*) (&byte), 1);
                    byte = 0;
                }
                if(i!=NUM_OF_THREADS-1){
                    for (int k = 0; k < 8 - (*Ends)[i]; k++) byte = ((*file)[i][j-8+k]=='1') | byte << 1;
                    for (int k = 0; k < (*Ends)[i]; k++) byte = ((*file)[i+1][k]=='1') | byte << 1;
                    tmp.append((char*) (&byte), 1);
                }
                out[i] = tmp;
            }FF_PARFOR_END(apply);
            (*file) = out;
            return EOS;
        }
        return GO_ON;
    }
};

int main(int argc, char* argv[]) {
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/FastFlow/";
    inputFile = "./data/TestFiles/";

    while ((option = (char) getopt(argc, argv, OPT_LIST)) != -1) {
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encFileName += optarg;
                break;
            case 't':
                NUM_OF_THREADS = (int) atoi(optarg);
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
    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();
    uintmax_t writePos = 0;
    vector<string> file(NUM_OF_THREADS);
    vector<uintmax_t> writePositions(NUM_OF_THREADS, 0);
    vector<uintmax_t> Starts(NUM_OF_THREADS, 0);
    vector<uintmax_t> Ends(NUM_OF_THREADS, 0);
    vector<long> timers = vector<long>(4, 0);


    {
        utimer total("Total time", &timers[2]);
        {
            utimer readTime("Read file", &timers[0]);
            utils::read(fileSize, &in, &file, NUM_OF_THREADS);
        }
        for (size_t i = 0; i < NUM_OF_THREADS; i++) {
            QUEUE::enque(new ff_task_t(&file, &ascii, &myMap, &writePos, &writePositions, &Starts, &Ends, i));
        }
        ff_Farm<ff_task_t, ff_task_t> counts( []() {
            std::vector<std::unique_ptr<ff_node> > W;
            for(size_t i=0;i<NUM_OF_THREADS;++i) {
                W.push_back(make_unique<Worker>());
            }
            return W;
        }() );
        collectCounts collectCounts;
        mapWorker createMap;
        ff_Farm<ff_task_t, ff_task_t> mapApp( []() {
            std::vector<std::unique_ptr<ff_node> > W;
            for(size_t i=0;i<NUM_OF_THREADS;++i) {
                W.push_back(make_unique<mapApply>());
            }
            return W;
        }() );
        calcIndices calcIndices;
        applyBit appBit;
        ff_Pipe<> pipe(counts, collectCounts, createMap, mapApp, calcIndices, appBit);
        pipe.run_and_wait_end();
        {
            utimer writeTime("Write file", &timers[1]);
            utils::write(encFile, file, writePositions, NUM_OF_THREADS);
        }
    }

    // Time without read and write
    timers[3] = timers[2] - timers[0] - timers[1];
    timers[0] = 0;
    timers[1] = 0;
    utils::writeResults("FastFlow", encFileName, fileSize, writePos, NUM_OF_THREADS, timers, false, false, 0, print, csvPath);
    return 0;
}