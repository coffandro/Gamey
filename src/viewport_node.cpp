#include "viewport_node.h"
#include <iostream>

ViewportNode::ViewportNode() : Node() {
    pos = (Vector2) {50, 50};
    vel = (Vector2) {2.5, -2.5};
}

ViewportNode::~ViewportNode() { }

void ViewportNode::Update(Viewport *viewport) {
    Node::Update(viewport);

    if (pos.x > viewport->rect.width ||
        pos.x < 0
    ) {
        vel.x *= -1;
    }

    if (pos.y > viewport->rect.height ||
        pos.y < 0
    ) {
        vel.y *= -1;
    }

    pos.x += vel.x;
    pos.y += vel.y;
}

void ViewportNode::Draw(Viewport *viewport) {
    Node::Draw(viewport);

    float ox = viewport->rect.x;
    float oy = viewport->rect.y;

    DrawText("Hello!", ox + 50, oy + 50, 48, RAYWHITE);
    DrawCircle(ox + pos.x, oy + pos.y, 25, WHITE);
}