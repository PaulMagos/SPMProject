//
// Created by Paul Magos on 06/08/23.
//

#ifndef SPMPROJECT_NODE_H
#define SPMPROJECT_NODE_H

class Node{
    int value;
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
        void increaseValue(){
            this->value++;
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
        bool operator < (const Node& node) const
            {
                return (this->value > node.getValue());
            }
};


#endif //SPMPROJECT_NODE_H
