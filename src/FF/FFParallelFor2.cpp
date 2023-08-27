#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include "../utils/utimer.cpp"
#include "../utils/Node.h"
#include "../utils/utils.cpp"
#include <iostream>
#include <thread>
#include <fstream>
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
int NUM_OF_THREADS = thread::hardware_concurrency();
#ifdef PRINT
bool print = true;
#else
bool print = false;
#endif

int main(int argc, char* argv[]) {
    /* -----------------        VARIABLES        ----------------- */
    char option;
    vector<uintmax_t> ascii(ASCII_MAX, 0);
    string inputFile, encFileName, decodedFile, MyDir, csvFile, encFile;
    map<uintmax_t, string> myMap;

    MyDir = "./data/EncodedFiles/FFParFor/";
    inputFile = "./data/TestFiles/";
    csvFile = "./data/CSV/FFParFor";
#if defined(ALL)
    csvFile += (ALL ? "All" : "");
#endif
    csvFile += ".csv";

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
#if not defined(ALL)
    encFile = MyDir + encFileName;
#else
    encFile = MyDir + (ALL ? "All" : "") + encFileName;
#endif
    /* -----------------       MUTEXES      ----------------- */
    mutex readFileMutex;
    mutex writeAsciiMutex;


    /* -----------------        FILE        ----------------- */
    /* Create input stream */
    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();
    vector<string> file(NUM_OF_THREADS);
    vector<long> timers = vector<long>(8, 0);


    {
        utimer total("Total time", timers[6]);
        {
            utimer read("Read file", timers[0]);
            read(fileSize, &in, &file, NUM_OF_THREADS);
        }
        #if defined(ALL)
        {
            utimer count("Count frequencies", timers[1]);
            // TODO count frequencies
        }
        {
            utimer create("Create map", timers[2]);
            Node::createMap(Node::buildTree(ascii), &myMap);
        }
        {
            utimer encode("Encode file", timers[3]);
            // TODO encode

        }
        {
            utimer tobytes("To bytes", timers[4]);
            // TODO to bytes

        }
        #else
            // TODO count frequencies
            Node::createMap(Node::buildTree(ascii), &myMap);
            // TODO encode
            // TODO to bytes
        #endif
        {
            utimer write("Write file", timers[6]);
        }
    }
    return 0;
}