#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <future>
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
vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file);
void createMap(Node root, vector<string> *map, const string &prefix = "");
string createOutput(vector<string> file, const vector<string>& myMap, int numThreads, uintmax_t len);

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Async/";

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
        vector<string> mMap(ASCII_MAX);
        {
            utimer t("createMap");
            createMap(buildTree(ascii), &mMap);
        }
        string output = createOutput(file, mMap, numThreads, fileSize);
        writeToFile(output, encodedFile, numThreads);
    }
    return 0;
}

ifstream::pos_type filesize(const char* filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg();
}

vector<int> calcChar(const string& line){
    vector<int> ascii(ASCII_MAX, 0);
    for (char j : line)
        ascii[j]++;
    return ascii;
}

vector<int> readFrequencies(ifstream* myFile, int numThreads, uintmax_t len, vector<string>* file){
    // Read file
    uintmax_t size = len / numThreads;
    {
        utimer timer("Calculate freq");
        vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
        vector<future<vector<int>>> futures;
        // Read file line by line
        for (int i = 0; i < numThreads; i++){
            (*myFile).seekg(i * size);
            if (i == numThreads-1){
                size = (len-(i*size));
            }
            (*file)[i] = string(size, '\0');
            (*myFile).read( &(*file)[i][0], size);
            futures.emplace_back(async(launch::async, calcChar, (*file)[i]));
        }
        ascii[0] = futures[0].get();
        for (int i = 1; i < numThreads; i++) {
            ascii[i] = futures[i].get();
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

void createMap(Node root, vector<string> *map, const string &prefix){
    if (root.getChar() != 256) {
        (*map)[root.getChar()] = prefix;
    } else {
        createMap(root.getLeftChild(), &(*map), prefix + "0");
        createMap(root.getRightChild(), &(*map), prefix + "1");
    }
}

string toBits(vector<string> myMap, const string& line){
    string bits;
    for (char j : line) {
        bits.append(myMap[j]);
    }
    return bits;
}

string createOutput(vector<string> file, const vector<string>& myMap, int numThreads, uintmax_t len) {
    // Read file
    {
        utimer timer("create output");
        vector<future<string>> futures;
        // Read file line by line
        for (int i = 0; i < numThreads; i++){
            futures.emplace_back(async(launch::async, toBits, myMap, file[i]));
        }
        string output;
        for (int i = 0; i < numThreads; i++) {
            output.append(futures[i].get());
        }
        return output;
    }
}

string toAscii(const string& bits){
    string output;
    uint8_t n = 0;
    uint8_t value = 0;
    for(char bit : bits){
        value |= static_cast<uint8_t>(bit == '1') << n;
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
        vector<thread> threads;
        uintmax_t Start = (((bits.size() - (bits.size()%8))/8)/numThreads + 1)*8;
        uintmax_t chunckSize = Start;
        for(int i = 0; i<numThreads; i++){
            if (i == numThreads-1){
                chunckSize = bits.size() - (i*Start);
            }
            threads.emplace_back([i, &bits, &output, Start, chunckSize](){
                output[i] = toAscii(bits.substr(i*Start, chunckSize));
            });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 0; i < numThreads; i++)
            outputFile << output[i];
        outputFile.close();
    }
}