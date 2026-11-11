#include "main_node.h"
#include <iostream>

Main::Main() : Node() {
    AddChild(&viewport_node);
    AddChild(&controls_node);
}

Main::~Main() {

}

void Main::Update(Viewport *viewport) {
    Rectangle remaining = viewport->rect;
    float half = remaining.width / 2.0f;
    Rectangle left = cut_top(&remaining, half);

    Viewport top_vp = { left, viewport->drawTexture };
    Viewport bottom_vp = { remaining, viewport->drawTexture };

    viewport_node.Update(&top_vp);
    controls_node.Update(&bottom_vp);
}

void Main::Draw(Viewport *viewport) {
    Rectangle remaining = viewport->rect;
    float half = remaining.width / 2.0f;
    Rectangle left = cut_top(&remaining, half);

    Viewport top_vp = { left, viewport->drawTexture };
    Viewport bottom_vp = { remaining, viewport->drawTexture };

    viewport_node.Draw(&top_vp);
    controls_node.Draw(&bottom_vp);
}