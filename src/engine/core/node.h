#pragma once

#include <vector>
#include "raylib.h"

typedef struct {
    Rectangle rect;
    RenderTexture2D drawTexture;
} Viewport;

Rectangle cut_left(Rectangle* rect, float a);
Rectangle cut_right(Rectangle* rect, float a);
Rectangle cut_top(Rectangle* rect, float a);
Rectangle cut_bottom(Rectangle* rect, float a);

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

    virtual void Update(Viewport *viewport);
    virtual void Draw(Viewport *viewport);
};