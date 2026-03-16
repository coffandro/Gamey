#include "main_node.h"
#include <iostream>

Main::Main() : Node() {
    pos = (Vector2) {50, 50};
    vel = (Vector2) {2.5, -2.5};
}

Main::~Main() { }

void Main::Update(Rectangle viewport) {
    Node::Update(viewport);

    if (pos.x > viewport.width ||
        pos.x < 0
    ) {
        vel.x *= -1;
    }

    if (pos.y > viewport.height ||
        pos.y < 0
    ) {
        vel.y *= -1;
    }

    pos.x += vel.x;
    pos.y += vel.y;
}

void Main::Draw(Rectangle viewport) {
    Node::Draw(viewport);

    DrawText("Hello!", 50, 50, 48, RAYWHITE);
    DrawCircle(pos.x, pos.y, 25, WHITE);
}