#include "node.h"
#include <iostream>

Node::Node() {

}

Node::~Node() {

}

void Node::AddChild(Node* node) {
    children.push_back(node);
}

void Node::RemoveChild(int index) {
    children.erase(children.begin() + index);
}

void Node::ClearChildren() {
    children.clear();
}

std::vector<Node*> Node::GetChildren() {
    return children;
}

Node* Node::GetChild(int index) {
    children.at(index);
}

void Node::Update(Rectangle viewport) {
    for (Node* child : children) {
        child->Update(viewport);
    }
}

void Node::Draw(Rectangle viewport) {
    for (Node* child : children) {
        child->Draw(viewport);
    }
}