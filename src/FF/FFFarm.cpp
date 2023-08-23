//
// Created by Paul Magos on 19/08/23.
//
#include "./fastflow/ff/ff.hpp"
#include "../utils/utimer.cpp"
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <mutex>
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
//using namespace ff;


void Func(ifstream* myFile, string* myString, int i, uintmax_t size, uintmax_t size1, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* uAscii){
    vector<int> ascii(ASCII_MAX, 0);
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * size1);
        (*myFile).read(&(*myString)[0], size);
    }
    for (char j : (*myString)) (ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((ascii)[j] != 0) {
                (*uAscii)[j] += (ascii)[j];
            }
        }
    }
}

struct fftask_t {
    fftask_t(ifstream* myFile, string* myString, int i, uintmax_t size, uintmax_t size1, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* uAscii):
    myFile(myFile), myString(myString), i(i), size(size), size1(size1), readFileMutex(readFileMutex), writeAsciiMutex(writeAsciiMutex), uAscii(uAscii) {}
    ifstream* myFile;
    string* myString;
    int i;
    uintmax_t size;
    uintmax_t size1;
    mutex* readFileMutex;
    mutex* writeAsciiMutex;
    vector<int>* uAscii;
};
static inline fftask_t* Wrapper(fftask_t *t, ff::ff_node *const) {
    Func(&(*t->myFile), &(*t->myString), t->i, t->size, t->size1,&(*t->readFileMutex), &(*t->writeAsciiMutex), &(*t->uAscii));
    return t;
}



int main(int argc, char* argv[])
{

    char option;
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

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
                numThreads = (int)atoi(optarg);
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }

    ifstream in(inputFile, std::ifstream::ate | std::ifstream::binary);
    uintmax_t fileSize = in.tellg();
    cout <<  fileSize << endl;
    ff::ff_Farm<fftask_t> farm(Wrapper, numThreads, true);
    farm.run();
    vector<string> file= vector<string>(numThreads);
    uintmax_t size1 = fileSize / numThreads;
    vector<int> ascii(ASCII_MAX, 0);
    // Read file line by line
    mutex asciim;
    mutex Filem;
    fftask_t *task = nullptr;
    uintmax_t size = size1;
    {
        utimer timer("Total");
        {
        utimer timr("Calculate freq");

            for (uint i = 0; i < numThreads; i++){
                size = (i==numThreads-1)? (fileSize-(i*size1)):size1;
                farm.offload(new fftask_t(&in, &file[i], i, size, size1, &Filem, &asciim, &ascii));
            }
            farm.run_and_wait_end();
        }
    }
    return 0;
}

