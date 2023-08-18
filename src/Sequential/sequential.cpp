#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
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
#define OPT_LIST "hi:p:"

Node buildTree(vector<int> ascii);
void readFrequencies(vector<int>* ascii, ifstream &myFile);
void writeToFile(const string& bits, const string& encodedFile);
void createMap(Node root, map<int, string> *map, const string &prefix = "");
string createOutput(const string& inputFile, map<int, string> myMap);



int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;

    inputFile = "./data/TestFiles/";
    encodedFile = "./data/EncodedFiles/Sequential/";

    while((option = (char)getopt(argc, argv, OPT_LIST)) != -1){
        switch (option) {
            case 'i':
                inputFile += optarg;
                break;
            case 'p':
                encodedFile += optarg;
                break;
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }

    {
        utimer timer("Total");
        ifstream myFile (inputFile);
        readFrequencies(&ascii, myFile);
        Node root = buildTree(ascii);
        map<int, string> myMap;
        {
            utimer t("createMap");
            createMap(root, &myMap);
        }
        ifstream myFile2 (inputFile);
        string output = createOutput(inputFile, myMap);
        writeToFile(output, encodedFile);
    }
    return 0;
}

void readFrequencies(vector<int>* ascii, ifstream &myFile){
    // Read file
    string str;
    {
        utimer timer("Calculate freq");
        while(myFile){
            char c = myFile.get();
            (*ascii)[c]++;
        }
    }
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

string createOutput(const string& inputFile, map<int, string> myMap) {
    string str;
    ifstream myFile (inputFile);
    {
        utimer timer("Create output");
        string output;
        while(myFile){
            char c = myFile.get();
            output.append(myMap[c]);
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