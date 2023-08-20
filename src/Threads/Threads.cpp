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
//vector<int> readFrequencies(const string& inputFile, int numThreads);
vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len);
void createMap(Node root, map<int, string> *map, const string &prefix = "");
//string createOutput(const string& inputFile, map<int, string> myMap, int numThreads);
string createOutput(ifstream* myFile, map<int, string> myMap, int numThreads, uintmax_t len);

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

    {
        utimer timer("Total");
        ascii = readFrequencies(&in, numThreads, fileSize);
        map<int, string> myMap;
        {
            utimer t("createMap");
            createMap(buildTree(ascii), &myMap);
        }
//        ifstream myFile2 (inputFile);
        string output = createOutput(&in, myMap, numThreads, fileSize);
        writeToFile(output, encodedFile, numThreads);
    }
    return 0;
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len){
    // Read file
//    uintmax_t len = (uintmax_t) filesize(inputFile.c_str());
//    ifstream myFile (inputFile, ios::binary | ios::ate);
    uintmax_t size = len / numThreads;
    {
        utimer timer("Calculate freq");
        vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
        vector<std::thread> threads;
        // Read file line by line
        for (int i = 0; i < numThreads; i++){
            (*myFile).seekg(i * size);
            if (i == numThreads-1){
                size = (len-(i*size));
            }
            string line(size, ' ');
            (*myFile).read( &line[0], size);
            threads.emplace_back([line, &ascii, i]
                          {
                                  for (char j : (string)line)
                                      ascii[i][j]++;
                          });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 1; i < numThreads; i++) {
            for (int j = 0; j < ASCII_MAX; j++) {
                ascii[0][j] += ascii[i][j];
            }
        }
        return ascii[0];
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

string createOutput(ifstream* myFile, map<int, string> myMap, int numThreads, uintmax_t len) {
    // Read file
//    uintmax_t len = (uintmax_t) filesize(inputFile.c_str());
//    ifstream myFile (inputFile, ios::binary | ios::ate);
    uintmax_t size = len / numThreads;
    {
        utimer timer("create output");
        vector<string> bits(numThreads);
        vector<std::thread> threads;
        // Read file line by line
        for (int i = 0; i < numThreads; i++) {
            (*myFile).seekg(i * size);
            if (i == numThreads - 1) {
                size = (len - (i * size));
            }
            string line(size, '\0');
            (*myFile).read(&line[0], size);
            threads.emplace_back([&bits, line, &myMap, i] {
                for (char j: (string) line) {
                    bits[i].append(myMap[j]);
                }
            });
        }
        string output;
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 0; i < numThreads; i++){
            output.append(bits[i]);
            bits[i] = "";
        }
        return output;
    }
}

string toAscii(const string& bits){
    string output;
    uint8_t n = 0;
    uint8_t value = 0;
    for(int j = 0; j<bits.size(); j++){
        value |= static_cast<uint8_t>(bits[j] == '1') << n;
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
    return output;
}

void writeToFile(const string& bits, const string& encodedFile, int numThreads){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    vector<string> output(numThreads);
    {
        utimer timer("write to file");
        vector<std::thread> threads;
        uintmax_t Start = (((bits.size() - (bits.size() % 8)) / 8) / numThreads + 1) * 8;
        uintmax_t chunckSize = Start;
        for (int i = 0; i < numThreads; i++) {
            if (i == numThreads - 1) {
                chunckSize = bits.size() - (i * Start);
            }
            threads.emplace_back([i, &bits, &output, Start, chunckSize]() {
                output[i] = toAscii(bits.substr(i * Start, chunckSize));
            });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 0; i < numThreads; i++){
            outputFile << output[i];
            output[i] = "";
        }
        outputFile.close();
    }
}