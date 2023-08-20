//
// Created by Paul Magos on 19/08/23.
//
#include <ff/ff.hpp>
#include "../utils/utimer.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>
#include <mutex>
#include <map>
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


static inline vector<int> Func(const string& line) {
    vector<int> ascii(ASCII_MAX, 0);
    for (char c : line) {
        ascii[c]++;
    }
    return ascii;
}

#if !defined(SEQUENTIAL)
struct ffFREQ_T {
    explicit ffFREQ_T(string line):line(std::move(line)) {}
    string line;
    vector<int> ascii = vector<int>(ASCII_MAX, 0);
};
static inline ffFREQ_T* WrapperFreq(ffFREQ_T *t, ff::ff_node *const) {
    t->ascii = Func(t->line);
    return t;
}
#endif

#if defined(SEQUENTIAL)
vector<int> readFrequencies(ifstream &myFile){
    vector<int> ascii(ASCII_MAX, 0);
    // Read file
    string str;
    {
        utimer timer("Calculate freq");
        while(myFile){
            int c = myFile.get();
            (*ascii)[c]++;
        }
    }
    return ascii;
}
#else
vector<int> readFrequencies(ifstream* myFile, uint fileSize, int numThreads, ff::ff_farm* farm){
    // Read file
    uintmax_t size = fileSize / numThreads;
    {
        utimer timer("Calculate freq");
        vector<int> ascii(ASCII_MAX, 0);
        // Read file line by line
        mutex m;
        void **task = nullptr;
        for (uint i = 0; i < numThreads; i++){
            (*myFile).seekg(i * size);
            if (i == numThreads-1){
                size = (fileSize- (i*size));
            }
            string line(size, ' ');
            (*myFile).read( &line[0], size);
            farm->offload(new ffFREQ_T(line));
            if (farm->load_result_nb(task)){
                for (int j = 0; j < ASCII_MAX; j++){
                    {
                        lock_guard<mutex> lock(m);
                        ascii[j] += ((ffFREQ_T*)task)->ascii[j];
                    }
                }
                delete task;
            }
        }
        return ascii;
    }
}
#endif





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


    cout << "File size: " << fileSize << endl;
    ffFREQ_T* r = NULL;
    ff::ff_Farm<ffFREQ_T> farm(WrapperFreq, numThreads, true);
    farm.run();


    {
        utimer timer("Total");
        vector<int> ascii = readFrequencies(&in, fileSize, numThreads, &farm);
        for (int i = 0; i < ASCII_MAX; ++i) {
            cout << ascii[i] << endl;
        }
//        map<int, string> myMap;
//        {
//            utimer t("createMap");
//            createMap(buildTree(ascii), &myMap);
//        }
//        ifstream myFile2 (inputFile);
//        string output = createOutput(inputFile, myMap);
//        writeToFile(output, encodedFile);
    }
    return 0;
}

