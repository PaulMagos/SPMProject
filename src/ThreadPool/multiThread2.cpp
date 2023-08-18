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
#include <queue>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"
#include "./ThreadPool.cpp"

using namespace std;

#define ASCII_MAX 256
// OPT LIST
/*
 * h HELP
 * i input file path
 * p encoded file path
 * o decoded file path
 */
#define OPT_LIST "hi:p:t:"

Node buildTree(vector<int> ascii);
void writeToFile(const vector<string>& bits, const string& encodedFile);
vector<int> readFrequencies(const string& inputFile, int numThreads);
void createMap(Node root, map<int, string> *map, const string &prefix = "");
vector<string> createOutput(const string& inputFile, map<int, string> myMap, int numThreads);

ThreadPool pool;

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/ThreadPool/";

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


    {
        utimer timer("Total");
        pool.Start(numThreads);
        ascii = readFrequencies(inputFile, numThreads);
        map<int, string> myMap;
        {
            utimer t("createMap");
            createMap(buildTree(ascii), &myMap);
        }
        ifstream myFile2 (inputFile);
        vector<string> output = createOutput(inputFile, myMap, numThreads);
        writeToFile(output, encodedFile);
        pool.Stop();
    }
    return 0;
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

vector<int> readFrequencies(const string& inputFile, int numThreads){
    // Read file
    int len = (int) filesize(inputFile.c_str());
    ifstream myFile (inputFile);
    int size = len / numThreads;
    mutex asciiMutex;
    {
        utimer timer("Calculate freq");
        vector<vector<int>> asciiTEMP(numThreads);
        vector<int> ascii(ASCII_MAX, 0);
        // Read file line by line
        for (int i = 0; i < numThreads; i++){
            myFile.seekg(i * (len / numThreads));
            size += (len- ((i+1)*size))*(i==numThreads-1);
            string line(size, ' ');
            myFile.read( &line[0], size);
            pool.QueueJob([line, &ascii, &asciiMutex]
                         {
                             {
                                 lock_guard<mutex> lock(asciiMutex);
                                 for (char j : (string)line)
                                     ascii[j]++;
                             }
                         });
        }
        while (pool.busy());
        return ascii;
    }
}

// Method for building the tree
Node buildTree(vector<int> ascii)
{
        Node *lChild, *rChild, *top;
        // Min Heap
        priority_queue<Node *, vector<Node *>, Node::cmp> minHeap;
        for (int i = 0; i < ASCII_MAX; i++) {
            if (ascii[i] != 0) {
                minHeap.push(new Node(ascii[i], i));
            }
        }
        while (minHeap.size() != 1) {
            lChild = minHeap.top();
            minHeap.pop();

            rChild = minHeap.top();
            minHeap.pop();

            top = new Node(lChild->getValue() + rChild->getValue(), 256);

            top->setLeftChild(lChild);
            top->setRightChild(rChild);

            minHeap.push(top);
        }
        return *minHeap.top();
}

void createMap(Node root, map<int, string> *map, const string &prefix){
    if (root.getChar() != 256) {
        (*map).insert(pair<int, string>(root.getChar(), prefix));
    } else {
        createMap(root.getLeftChild(), &(*map), prefix + "0");
        createMap(root.getRightChild(), &(*map), prefix + "1");
    }
}

vector<string> createOutput(const string& inputFile, map<int, string> myMap, int numThreads) {
    // Read file
    int len = (int) filesize(inputFile.c_str());
    ifstream myFile (inputFile);
    int size = len / numThreads;
    vector<string> bits(numThreads);
    {
        utimer timer("create output");
        vector<std::thread> threads;
        threads.reserve(numThreads);
        // Read file line by line
        for (int i = 0; i < numThreads; i++){
            myFile.seekg(i * (len / numThreads));
            size += (len- ((i+1)*size))*(i==numThreads-1);
            string line(size, '\0');
            myFile.read( &line[0], size);
            threads.emplace_back([&bits, line, &myMap, i]
                     {
                         for (char j : (string)line) {
                             bits[i].append(myMap[j]);
                         }
                     });
        }
        while (pool.busy());
        return bits;
    }
}

void writeToFile(const vector<string>& bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    string bit;
    for (const auto &bit1 : bits) {
        bit.append(bit1);
    }
    string output;
    {
        utimer timer("write to file");
        uint64_t n = 0;
        uint8_t value = 0;
        for(auto c : bit)
        {
            value |= static_cast<uint8_t>(c == '1') << n;
            if(++n == 8)
            {
                output.append((char*) (&value), 1);
                n = 0;
                value = 0;
            }
        }
        if(n != 0)
        {
            while (8-n > 0){
                value |= static_cast<uint8_t>(0) << n;
                n++;
            }
            output.append((char*) (&value), 1);
        }
        outputFile << output;
        outputFile.close();
    }
}