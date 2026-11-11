#pragma once
#include "engine/core/node.h"
#include "viewport_node.h"
#include "controls_node.h"

class Main : public Node {
private:
    ControlsNode controls_node;
    ViewportNode viewport_node;

public:
    Main();

    ~Main();
    
    void Update(Viewport *viewport) override;

    void Draw(Viewport *viewport) override;
};