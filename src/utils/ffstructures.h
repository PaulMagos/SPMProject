//
// Created by Paul Magos on 28/08/23.
//

#ifndef SPMPROJECT_FFSTRUCTURES_H
#define SPMPROJECT_FFSTRUCTURES_H
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "./Node.h"
// #include "./utimer.cpp"
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>

#define ASCII_MAX 256
using namespace std;
using namespace ff;

struct ff_task_t{
    ff_task_t(int tasks, vector<string>* file, vector<uintmax_t>* ascii) : Tasks(tasks), file(file), ascii(ascii) {
    };
    int Tasks;
    vector<string>* file;
    vector<uintmax_t>* ascii;
};

struct ff_apply_map_t{
    ff_apply_map_t(map<uintmax_t, string>* myMap, int tasks, string* line) :
            myMap(myMap), Tasks(tasks), line(line){
    }
    map<uintmax_t, string>* myMap;
    string* line;
    int Tasks;
};

struct ff_bit_byte_t{
    ff_bit_byte_t(vector<string>* file, int Tasks, vector<uintmax_t>* writePositions, vector<uintmax_t>* Starts,
                  vector<uintmax_t>* Ends, uintmax_t* writePos) : file(file), Tasks(Tasks), writePositions(writePositions),
                                                                  Starts(Starts), Ends(Ends), writePos(writePos) {};
    vector<uintmax_t>* writePositions;
    vector<uintmax_t>* Starts;
    vector<uintmax_t>* Ends;
    uintmax_t* writePos;
    vector<string>* file;
    int Tasks;
};


struct Emitter: ff_node_t<ff_task_t> {
    Emitter(int nw, ff_task_t *task):nw(nw), task(task) {};
    ff_task_t *svc(ff_task_t*) override {
        ff::ffTime(START_TIME);
        vector<string>* file = task->file;
        vector<uintmax_t>* ascii = task->ascii;
        vector<uintmax_t> loc(ASCII_MAX, 0);
        auto count = [&](const long int i, vector<uintmax_t>& loc) {
            for (auto j: (*file)[i]){
                loc[j]++;}
        };
        auto reduce = [](vector<uintmax_t>& global, const vector<uintmax_t> elem){
            for(int j=0; j<ASCII_MAX; j++){
                (global)[j] += elem[j];
            }
        };

        ParallelForReduce<vector<uintmax_t>> prf(nw, true);
        long chunk = task->Tasks/nw;

        prf.parallel_reduce(*ascii, loc, 0, task->Tasks, 1, chunk, count, reduce);
        ff_send_out(task);
        ff::ffTime(STOP_TIME);
        printf("ParForReduce %d Time = %g\n", nw, ff::ffTime(GET_TIME));
        return EOS;
    }
    int nw;
    ff_task_t* task;
};

struct mapWorker : ff_node_t<ff_task_t> {
    mapWorker(map<uintmax_t, string> *myMap, int nw, vector<string>* file): myMap(myMap), nw(nw), file(file) {};
    ff_task_t *svc(ff_task_t *inA) override {
        ff::ffTime(START_TIME);
        Node::createMap(Node::buildTree(*inA->ascii), myMap);
        for (int i = 0; i < inA->Tasks; i++) {
            auto* task = new ff_apply_map_t(this->myMap, inA->Tasks, &(*file)[i]);
            ff_send_out(task, i%nw);
        }
        ff::ffTime(STOP_TIME);
        printf("Map Create and Emit for 2nd Farm %d Time = %g\n", nw, ff::ffTime(GET_TIME));
        return GO_ON;
    }
    map<uintmax_t, string>* myMap;
    vector<string>* file;
    int nw;
};

struct mapApply : ff_node_t<ff_apply_map_t> {
    mapApply(vector<string>* file): file(file){};
    ff_apply_map_t *svc(ff_apply_map_t *inA) override {
        string tmp;
        string line = *inA->line;
        for (int i = 0; i < line.size(); i++) {
            tmp.append((*inA->myMap)[(line)[i]]);
        }
        (*inA->line) = tmp;
        ff_send_out(inA);
        return GO_ON;
    }
    vector<string>* file;
};

struct calcIndices : ff_node_t<ff_apply_map_t>{
    calcIndices(vector<uintmax_t>* writePositions, vector<uintmax_t>* Starts, vector<uintmax_t>* Ends, uintmax_t* writePos, vector<string>* file) :
            writePositions(writePositions), Starts(Starts), Ends(Ends), writePos(writePos), file(file) {};
    ff_apply_map_t *svc(ff_apply_map_t *inA) override {
        if(ntasks==0) ff::ffTime(START_TIME);
        ntasks++;
        if (ntasks == inA->Tasks) {
            int Start, End = 0;
            for (int i = 0; i< ntasks; i++){
                Start = End;
                (*Starts)[i] = Start;
                if(i==ntasks-1)
                    (*file)[i].append(string(8-(((*file)[i].size() - Start)%8), '0'));
                End = (8 - ((*file)[i].size()-Start)%8) % 8;
                (*Ends)[i] = End;
                (*writePositions)[i] = *writePos;
                *writePos += ((*file)[i].size()-Start)+End;
            }
            ff_send_out(new ff_bit_byte_t(file, ntasks, writePositions, Starts, Ends, writePos));
            ff::ffTime(STOP_TIME);
            printf("Collect 2nd Farm Time = %g\n", ff::ffTime(GET_TIME));
        }
        return GO_ON;
    }
    vector<uintmax_t>* writePositions;
    vector<uintmax_t>* Starts;
    vector<uintmax_t>* Ends;
    uintmax_t* writePos;
    vector<string>* file;
    int ntasks = 0;
};

struct applyBit : ff_node_t<ff_bit_byte_t> {
    applyBit(int nw, string encFile): nw(nw), encFile(encFile) {};
    ff_bit_byte_t *svc(ff_bit_byte_t *inA) override {
        vector<string> out = vector<string>(inA->Tasks);
        vector<string> *file = inA->file;
        vector<uintmax_t> *writePositions = inA->writePositions;
        vector<uintmax_t> *Starts = inA->Starts;
        vector<uintmax_t> *Ends = inA->Ends;
        uintmax_t chunk = inA->Tasks/nw;
        mutex writeFile;
        ofstream outputFile(encFile,  ios::binary|ios::out);
        FF_PARFOR_BEGIN(apply, i, 0, inA->Tasks, 1, chunk, nw){
            uintmax_t j;
            uint8_t byte = 0;
            for (j = (*Starts)[i]; j < (*file)[i].size(); j+=8) {
                if (j+8>((*file)[i].size())) break;
                for (uintmax_t k = 0; k < 8; k++) {
                    byte = ((*file)[i][j+k]=='1') | byte << 1 ;
                }
                out[i].append((char*) (&byte), 1);
                byte = 0;
            }
            if(i!=inA->Tasks-1){
                for (int k = j-8; k < (*file)[i].size(); k++) byte = ((*file)[i][k]=='1') | byte << 1;
                for (int k = 0; k < (*Ends)[i]; k++) byte = ((*file)[i+1][k]=='1') | byte << 1;
                out[i].append((char*) (&byte), 1);
            }
        #ifndef TIME
            {
            unique_lock<mutex> lock(writeFile);
            (outputFile).seekp((*writePositions)[i] / 8);
            (outputFile).write(out[i].c_str(), out[i].size());
            }
        #endif
        }FF_PARFOR_END(apply);
        #ifdef TIME
            (*file) = out;
        #endif
        return EOS;
    }
    int nw;
    string encFile;
};

#endif //SPMPROJECT_FFSTRUCTURES_H
