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
void write_to_file_Matteo(const string& bits, const string& encodedFile);
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


    map<int, string> myMap;
    ifstream myFile (inputFile);
    string output;
    {
        utimer total("Total");
        {
            utimer t("Calculate freq");
            readFrequencies(&ascii, myFile);
        }
        {
            utimer t("CreateMap");
            createMap(buildTree(ascii), &myMap);
        }
        {
            utimer timer("Create output");
            output = createOutput(inputFile, myMap);
        }
        {
            utimer timer("Write to file");
            writeToFile(output, encodedFile);
        }

        {
            utimer timer("Write to file Matteo");
            write_to_file_Matteo(output,  encodedFile.insert(encodedFile.size()-4, "Matteo_"));
        }
    }
    return 0;
}

void readFrequencies(vector<int>* ascii, ifstream &myFile){
    // Read file
    string str;
    while(myFile){
        char c = myFile.get();
        (*ascii)[c]++;
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

string createOutput(const string& inputFile, map<int, string> myMap) {
    string str;
    ifstream myFile (inputFile);
    string output;
    while(myFile){
        char c = myFile.get();
        output.append(myMap[c]);
    }
    while (output.size() % 8 != 0) {
        output.push_back('0');
    }
    return output;
}

void writeToFile(const string& bits, const string& encodedFile){
    ofstream outputFile(encodedFile, ios::binary | ios::out);
    uint8_t value = 0;
    uint8_t n = 0;
    for(char bit : bits){
        value = (bit == '1') | value << 1;
        if(++n == 8)
        {
            outputFile.write((char*) (&value), 1);
            n = 0;
            value = 0;
        }
    }
}

void write_to_file_Matteo(const string& bits, const string& encodedFile){
    ofstream outputFile1(encodedFile, ios::binary | ios::out);
    string output;
    char byte;
    for (long long unsigned i = 0; i < bits.size(); i += 8) {
        for (int j = 0; j < 8; j++) {
            byte = (bits[i + j] == '1') | byte << 1;
        }
        outputFile1.write((char*) (&byte), 1);
        byte = 0;
    }
}
