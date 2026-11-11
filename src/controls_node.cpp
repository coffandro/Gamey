#include "controls_node.h"
#include <iostream>

ControlsNode::ControlsNode() : Node() {
}

ControlsNode::~ControlsNode() { }

void ControlsNode::Update(Viewport *viewport) {
    Node::Update(viewport);
}

void ControlsNode::Draw(Viewport *viewport) {
    Node::Draw(viewport);
    DrawRectangle(viewport->rect.x, viewport->rect.y, viewport->rect.width, viewport->rect.height, BLUE);
}