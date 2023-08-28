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
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>

#define ASCII_MAX 256
using namespace std;
using namespace ff;

struct ff_task_t{
    ff_task_t(string* file, int i, int tasks) : file(file), index(i), Tasks(tasks) {
        localAscii = vector<uintmax_t>(ASCII_MAX, 0);
    };
    string *file;
    int index;
    int Tasks;
    vector<uintmax_t> localAscii;
};

struct ff_map_t{
    ff_map_t(vector<uintmax_t>* ascii, int tasks, vector<string>* file) :
            ascii(ascii), Tasks(tasks), file(file){
    }
    vector<uintmax_t> * ascii;
    vector<string>* file;
    int Tasks;
};

struct ff_apply_map_t{
    ff_apply_map_t(map<uintmax_t, string>* myMap, int tasks, string* line) :
            myMap(myMap), Tasks(tasks), line(line){
    }
    map<uintmax_t, string>* myMap;
    string* line;
    int Tasks;
};

struct ff_calc_idx_t{
    ff_calc_idx_t(vector<string>* file, int Tasks) : file(file), Tasks(Tasks) {};
    vector<string>* file;
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


struct Emitter: ff_monode_t<ff_task_t> {
    Emitter(int nw, vector<ff_task_t*>* tasks):nw(nw), tasks(tasks) {};
    ff_task_t *svc(ff_task_t*) {
        for (int i = 0; i < tasks->size(); i++) {
            ff_send_out_to(tasks->operator[](i), i % nw);
        }
        return EOS;
    }
    int nw;
    vector<ff_task_t*>* tasks;
};

struct Worker : ff_node_t<ff_task_t> {
    ff_task_t *svc(ff_task_t *inA) override {
//        ff_task_t* inA = QUEUE::deque();
        // this is the parallel_for provided by the ff_Map class
        for (int i = 0; i < (*inA->file).size(); i++)
            inA->localAscii[(*inA->file)[i]]++;
        ff_send_out(inA);
        return GO_ON;
    }
};

struct collectCounts: ff_node_t<ff_task_t> {
    collectCounts(vector<uintmax_t> *ascii, vector<string>* file): ascii(ascii), file(file) {};
    ff_task_t *svc(ff_task_t *inA) override {
        ff_task_t &A = *inA;
        nt++;
        {
            for (int i = 0; i < ASCII_MAX; i++)
                this->ascii->operator[](i) += A.localAscii[i];
        }
        string* line = A.file;
        if(nt == A.Tasks) ff_send_out(new ff_map_t(ascii, A.Tasks, file));
        return GO_ON;
    }
    vector<uintmax_t> *ascii;
    vector<string>* file;
    int nt = 0;
};

struct mapWorker : ff_node_t<ff_map_t> {
    mapWorker(map<uintmax_t, string> *myMap, int nw, vector<string>* file): myMap(myMap), nw(nw), file(file) {};
    ff_map_t *svc(ff_map_t *inA) override {
        Node::createMap(Node::buildTree(*inA->ascii), myMap);
        for (int i = 0; i < inA->Tasks; i++) {
            auto* task = new ff_apply_map_t(this->myMap, inA->Tasks, &(*file)[i]);
            ff_send_out(task, i%nw);
        }
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
        for (int i = 0; i < (*inA->line).size(); i++) {
            tmp += (*inA->myMap)[(*inA->line)[i]];
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
                for (int i = 0; i < ntasks; i++) {
                    ff_send_out(new ff_bit_byte_t(file, ntasks, writePositions, Starts, Ends, writePos));
                }
            }
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
    applyBit(int nw): nw(nw) {};
    ff_bit_byte_t *svc(ff_bit_byte_t *inA) override {
        vector<string> out = vector<string>(inA->Tasks);
        vector<string> *file = inA->file;
        vector<uintmax_t> *writePositions = inA->writePositions;
        vector<uintmax_t> *Starts = inA->Starts;
        vector<uintmax_t> *Ends = inA->Ends;
        uintmax_t chunk = inA->Tasks/nw;
        FF_PARFOR_BEGIN(apply, i, 0, inA->Tasks, 1, chunk, nw){
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
            if(i!=inA->Tasks-1){
                for (int k = 0; k < 8 - (*Ends)[i]; k++) byte = ((*file)[i][j-8+k]=='1') | byte << 1;
                for (int k = 0; k < (*Ends)[i]; k++) byte = ((*file)[i+1][k]=='1') | byte << 1;
                tmp.append((char*) (&byte), 1);
            }
            out[i] = tmp;
        }FF_PARFOR_END(apply);
        (*file) = out;
        return EOS;
    }
    int nw;
};

#endif //SPMPROJECT_FFSTRUCTURES_H
