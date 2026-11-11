#include "node.h"
#include <iostream>
#include <raymath.h>
#include <math.h>

#include <algorithm> // for std::min / std::max

Rectangle cut_left(Rectangle* rect, float a) {
    float cut = std::min(a, rect->width);

    Rectangle result = {
        rect->x,
        rect->y,
        cut,
        rect->height
    };

    rect->x += cut;
    rect->width -= cut;

    return result;
}

Rectangle cut_right(Rectangle* rect, float a) {
    float cut = std::min(a, rect->width);

    Rectangle result = {
        rect->x + rect->width - cut,
        rect->y,
        cut,
        rect->height
    };

    rect->width -= cut;

    return result;
}

Rectangle cut_top(Rectangle* rect, float a) {
    float cut = std::min(a, rect->height);

    Rectangle result = {
        rect->x,
        rect->y,
        rect->width,
        cut
    };

    rect->y += cut;
    rect->height -= cut;

    return result;
}

Rectangle cut_bottom(Rectangle* rect, float a) {
    float cut = std::min(a, rect->height);

    Rectangle result = {
        rect->x,
        rect->y + rect->height - cut,
        rect->width,
        cut
    };

    rect->height -= cut;

    return result;
}


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

void Node::Update(Viewport *viewport) {
    for (Node* child : children) {
        child->Update(viewport);
    }
}

void Node::Draw(Viewport *viewport) {
    for (Node* child : children) {
        child->Draw(viewport);
    }
}