#include <iostream>
#include <utility>
#include <vector>
#include <getopt.h>
#include <fstream>
#include <map>
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
#define OPT_LIST "hi:p:o:"

class Node{
    int value;
    int c;
    string code;
    vector<Node> children;
public:
    Node(int value, int c){
        this->value = value;
        this->c = c;
        this->code = "";
        this->children = vector<Node>();
    }
    int getValue() const{
        return this->value;
    }
    int getChar() const{
        return this->c;
    }
    void increaseValue(){
        this->value++;
    }
    void setChild(const Node& node1, const Node& node2) {
        this->children.push_back(node1);
        this->children.push_back(node2);
    }
    vector<Node> getChild() const{
        return this->children;
    }
    bool operator < (const Node& node) const
    {
        return (this->value > node.getValue());
    }
    bool operator == (const Node& node) const
    {
        return (this->c == node.getChar());
    }
};

void encode(const string& outputName, const string& inputFile);
void createMap(const Node& tree, const string& prefix = "", map<char, string>* map = nullptr);



int main(int argc, char* argv[])
{

    char option;
    vector<int> ascii(ASCII_MAX, 0);
    string inputFile, encodedFile, decodedFile;

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
            default:
                cout << "Invalid option" << endl;
                return 1;
        }
    }


//    encode(encodedFile, inputFile);

    ifstream myFile ("test111.txt");
    ifstream myFile2 ("test1112.txt");
    string f1;
    cout << "start 1" << endl;
    while (myFile) {
        f1.append(1, (char)myFile.get());
    }
    cout << "end 1" << endl;
    cout << "start 2" << endl;
    string f2;
    while (myFile2) {
        f2.append(1, (char)myFile2.get());
    }
    cout << "end 2" << endl;
    for (int i = 0; i < f1.size(); ++i) {
        if(i>=f2.size()){
            cout << "ERROR LENGHT";
        }
        if(f1[i] != f2[i]){
            cout << "ERROR" << f1[1] << " != " << f2[i] <<  " at " << i << endl;
        }
    }
    for (int i = 0; i < f2.size(); ++i) {
        if(i>=f1.size()){
            cout << "ERROR LENGHT";
        }
        if(f1[i] != f2[i]){
            cout << "ERROR" << f1[1] << " != " << f2[i] <<  " at " << i << endl;
        }
    }
    return 0;
}


// Method for building the tree
void buildTree(const string& inputFile, map<char, string> *map)
{
    vector<Node> tree;
    {
        utimer t("buildTree");

        ifstream myFile(inputFile);
        bool contained;
        // Read file
        if ((myFile).is_open()) {
            // Read file char by char
            int myChar;
            while ((myFile)) {
                // Get char
                myChar = (myFile).get();
                // If not EOF
                if (myChar != EOF) {
                    contained = false;
                    for (auto &i: tree) {
                        if (i.getChar() == myChar) {
                            i.increaseValue();
                            contained = true;
                        }
                    }
                    if (!contained) {
                        tree.push_back(Node(1, myChar));
                    }
                }
            }
        }

        sort(tree.begin(), tree.end());

        // Create Huffman tree
        while (tree.size() > 1) {
            Node right = tree[tree.size() - 1];
            Node left = tree[tree.size() - 2];
            Node newNode = Node(left.getValue() + right.getValue(), 256);
            tree.erase(tree.end() - 2, tree.end());
            newNode.setChild(left, right);
            tree.push_back(newNode);
            sort(tree.begin(), tree.end());
        }
    }

    {
        utimer t("createMap");
        createMap(tree[0], "", &(*map));
    }
}

void createMap(const Node& tree, const string &prefix, map<char, string> *map){
    if (tree.getChar() != 256) {
        (*map).insert(pair<char, string>((char)tree.getChar(), prefix));
    } else {
        createMap(tree.getChild()[0], prefix + "0", &(*map));
        createMap(tree.getChild()[1], prefix + "1", &(*map));
    }
}

// Method for encoding
/*
 * @param outputName -> name of the output file
 * @param ascii -> vector of frequencies
 */
void encode(const string& outputName, const string& inputFile){
    // Create output file
    {
        utimer t("encode");
        map<char, string> myMap;
        buildTree(inputFile, &myMap);
        ofstream encodedFile(outputName);
        ifstream originalFile(inputFile);
        string encodedString;
        // Map original file to encoded file
        if ((originalFile).is_open()) {
            // Read file char by char
            int myChar;
            while ((originalFile)) {
                // Get char
                myChar = (originalFile).get();
                // If not EOF
                if (myChar != EOF) {
                    // Increment frequency
                    encodedString += myMap[(char) myChar];
                }
            }
        }
        uint8_t n = 0;
        uint8_t value = 0;
        for (auto c: encodedString) {
            value |= static_cast<uint8_t>(c == '1') << n;
            if (++n == 8) {
                encodedFile.write((char *) (&value), 1);
                n = 0;
                value = 0;
            }
        }
        if (n != 0) {
            while (8 - n > 0) {
                value |= static_cast<uint8_t>(0) << n;
                n++;
            }
            encodedFile.write((char *) (&value), 1);
        }
        encodedFile.close();
    }
}


