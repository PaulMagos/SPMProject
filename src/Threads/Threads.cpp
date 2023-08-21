#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <queue>
#include <mutex>
#include "../utils/Node.h"
#include "../utils/utimer.cpp"

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
void writeToFile(const string& bits, const string& encodedFile, int numThreads);
vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file);
void createMap(Node root, map<int, string> *map, const string &prefix = "");
string createOutput(vector<string>* myFile, map<int, string> myMap, int numThreads, uintmax_t len);

int main(int argc, char* argv[])
{
    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Threads/";

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

    ifstream in(inputFile, ifstream::ate | ifstream::binary);
    uintmax_t fileSize = in.tellg();

    vector<string> file(numThreads);
    {
        utimer timer("Total");
        ascii = readFrequencies(&in, numThreads, fileSize, &file);
        map<int, string> myMap;
        {
            utimer t("createMap");
            createMap(buildTree(ascii), &myMap);
        }
        string output = createOutput(&file, myMap, numThreads, fileSize);
        writeToFile(output, encodedFile, numThreads);
    }
    return 0;
}

void calcChar(ifstream* myFile, string* myString, int i, uintmax_t size, mutex* readFileMutex, mutex* writeAsciiMutex, vector<int>* ascii, vector<int>* uAscii){
    {
        unique_lock<mutex> lock(*readFileMutex);
        (*myFile).seekg(i * size);
        (*myFile).read(&(*myString)[0], size);
    }
    for (char j : (*myString)) (*ascii)[j]++;
    {
        unique_lock<mutex> lock(*writeAsciiMutex);
        for (int j = 0; j < ASCII_MAX; j++) {
            if ((*ascii)[j] != 0) {
                (*uAscii)[j] += (*ascii)[j];
            }
        }
    }
}

vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file){
    // Read file
    uintmax_t size = len / numThreads;
    mutex readFileMutex;
    mutex writeAsciiMutex;
    {
        utimer timer("Calculate freq");
        vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
        vector<int> uAscii(ASCII_MAX, 0);
        vector<std::thread> threads;
        for (int i = 0; i < numThreads; i++){
            size = (i==numThreads-1) ? len - (i*size) : size;
            (*file)[i] = string(size, ' ');
            threads.emplace_back([capture0 = &(*myFile),
                                         capture1 = &(*file)[i],
                                         i,
                                         size,
                                         capture2 = &readFileMutex,
                                         capture3= &ascii[i],
                                         capture4 = &writeAsciiMutex,
                                        capture5 = &uAscii] {
                return calcChar(capture0, capture1, i, size, capture2, capture4, capture3, capture5); });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        return uAscii;
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

void toBits(map<int, string> myMap, string* line){
    string bits;
    for (char j : *line) {
        bits.append(myMap[j]);
    }
    *line = bits;
}

string createOutput(vector<string>* file, map<int, string> myMap, int numThreads, uintmax_t len) {
    // Read file
    {
        utimer timer("create output");
        vector<std::thread> threads;
        // Read file line by line
        for (int i = 0; i < numThreads; i++)
            threads.emplace_back([myMap, capture0 = &(*file)[i]] { return toBits(myMap, capture0); });
        string output;
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 0; i < numThreads; i++){
            output.append((*file)[i]);
        }
        while (output.size()%8 != 0){
            output.append("0");
        }
        return output;
    }
}

void writeToFile(const string& bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    mutex fileMutex;
    {
        utimer timer("write to file");
        vector<std::thread> threads;
        uintmax_t Start = (((bits.size() - (bits.size() % 8)) / 8) / numThreads + 1) * 8;
        uintmax_t chunkSize = Start;
        for (int i = 0; i < numThreads; i++) {
            chunkSize += (i==numThreads-1) ? bits.size() - ((i+1)*Start) : 0;
            threads.emplace_back([&bits, start=i*Start, chunkSize, &outputFile, &fileMutex]() {
                string output;
                uint8_t n = 0;
                uint8_t value = 0;
                for(int j = 0; j<chunkSize; j++){
                    value = (bits[start+j] == '1') | value << 1;
                    if(++n == 8)
                    {
                        output.append((char*) (&value), 1);
                        n = 0;
                        value = 0;
                    }
                }
                {
                    unique_lock<mutex> lock(fileMutex);
                    outputFile.seekp(start/8);
                    outputFile.write(output.c_str(), output.size());
                }
            });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        outputFile.close();
    }
}