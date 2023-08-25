//
// Created by Paul Magos on 06/08/23.
//
#include <utility>
#include <vector>
#include <map>
#include <queue>

#ifndef SPMPROJECT_NODE_H
#define SPMPROJECT_NODE_H
class Node{
    uintmax_t value;
    int c;
    int code;
    Node *left, *right;
    public:
        Node(int value, int c){
            this->value = value;
            this->c = c;
            this->code = 0;
            this->left = nullptr;
            this->right = nullptr;
        }
        int getValue() const{
            return this->value;
        }
        int getChar() const{
            return this->c;
        }
        Node getLeftChild() {
            return *this->left;
        }
        Node getRightChild() {
            return *this->right;
        }
        void setLeftChild(Node* node){
            this->left = node;
        }
        void setRightChild(Node* node){
            this->right = node;
        }
    public:
    // Method for building the tree
        static Node buildTree(std::vector<uintmax_t> ascii)
        {
            Node *lChild, *rChild, *top;
            // Min Heap
            std::priority_queue<Node *, std::vector<Node *>, cmp> minHeap;
            for (int i = 0; i < 256; i++) {
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
        static void createMap(Node root, std::map<uintmax_t, std::string> *map, const std::string &prefix = ""){
            if (root.getChar() != 256) {
                (*map).insert(std::pair<uintmax_t, std::string>(root.getChar(), prefix));
            } else {
                createMap(root.getLeftChild(), &(*map), prefix + "0");
                createMap(root.getRightChild(), &(*map), prefix + "1");
            }
        }
        struct cmp
        {
            bool operator()(Node* node1, Node* node2) const
            {
                return (node1->getValue() > node2->getValue());
            }
        };
};

#endif //SPMPROJECT_NODE_H
