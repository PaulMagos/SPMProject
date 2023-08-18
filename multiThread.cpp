#include <sstream>
#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <thread>
#include <map>
#include <queue>
#include <functional>
#include "./Node.h"
#include "./utils/utimer.cpp"

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
void writeToFile(const string& bits, const string& encodedFile);
vector<int> readFrequencies(const string& inputFile, int numThreads);
void createMap(Node root, map<int, string> *map, const string &prefix = "");
string createOutput(const string& inputFile, map<int, string> myMap, int numThreads);

int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;
    int numThreads = 4;

    inputFile = "./TestFiles/";
    encodedFile = "./EncodedFiles/";

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
        ascii = readFrequencies(inputFile, numThreads);
        Node root = buildTree(ascii);
        map<int, string> myMap;
        {
            utimer t("createMap");
            createMap(root, &myMap);
        }
        ifstream myFile2 (inputFile);
        string output = createOutput(inputFile, myMap, numThreads);
        writeToFile(output, encodedFile);
    }
    return 0;
}

vector<int> readFrequencies(const string& inputFile, int numThreads){
    // Read file
    ifstream myFile (inputFile);
    vector<vector<int>> ascii(numThreads, vector<int>(ASCII_MAX, 0));
    string str;
    {
        utimer timer("Load file");
        stringstream buffer;
        buffer << myFile.rdbuf(); //read the file
        str = buffer.str(); //str holds the content of the file
    }
    {
        utimer timer("Calculate freq");
        vector<std::thread> threads;
        threads.reserve(numThreads);
        // Read file line by line
        string line;
        size_t len = str.length();

        for (int i = 0; i < numThreads; i++){
            line = str.substr(i * (len / numThreads), len / (numThreads - ((numThreads-1)*(i==numThreads-1))));
            threads.emplace_back([line, &ascii, i]
            {
                 for (char j : line) { ascii[i][j]++;}
            });
        }
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
        for (int i = 1; i < numThreads; i++)
        for (int j = 0; j < ASCII_MAX; j++) {
            ascii[0][j] += ascii[i][j];
        }
    }
    return ascii[0];
}

// Method for building the tree
Node buildTree(vector<int> ascii)
{
    {
        utimer timer("build tree");
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
}

void createMap(Node root, map<int, string> *map, const string &prefix){
    if (root.getChar() != 256) {
        (*map).insert(pair<int, string>(root.getChar(), prefix));
    } else {
        createMap(root.getLeftChild(), &(*map), prefix + "0");
        createMap(root.getRightChild(), &(*map), prefix + "1");
    }
}

string createOutput(const string& inputFile, map<int, string> myMap, int numThreads) {
    string str;
    ifstream myFile (inputFile);
    {
        utimer timer("Load file 2nd");
        stringstream buffer;
        buffer << myFile.rdbuf(); //read the file
        str = buffer.str(); //str holds the content of the file
    }
    {
        utimer timer("create output");
        vector<string> bits(numThreads);
        string line;
        vector<std::thread> threads;
        threads.reserve(numThreads);
        size_t len = str.length();
        for (int i = 0; i < numThreads; i++){
            line = str.substr(i * (len / numThreads), len / (numThreads - ((numThreads-1)*(i==numThreads-1))));
            threads.emplace_back([&bits, line, &myMap, i]
            {
                for (char j : line) {
                    bits[i].append(myMap[j]);
                }
            });
        }
        string output;
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
            output.append(bits[i]);
        }
        return output;
    }
}

void writeToFile(const string& bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    string output;
    {
        utimer timer("write to file");
        uint64_t n = 0;
        uint8_t value = 0;
        for(auto c : bits)
        {
            value |= static_cast<uint8_t>(c == '1') << n;
            if(++n == 8)
            {
                output.append((char*) (&value), 1);
//                outputFile.write((char*) (&value), 1);
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
//            outputFile.write((char*) (&value), 1);
            output.append((char*) (&value), 1);
        }
        outputFile << output;
        outputFile.close();
    }
}