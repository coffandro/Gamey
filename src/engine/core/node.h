#pragma once

#include <vector>
#include "raylib.h"

class Node {
private:
    std::vector<Node*> children;

public:
    Node();
    ~Node();

    void AddChild(Node* node);
    void RemoveChild(int index);
    void ClearChildren();
    std::vector<Node*> GetChildren();
    Node* GetChild(int index);

    virtual void Update(Rectangle viewport);
    virtual void Draw(Rectangle viewport);
};