#pragma once
#include "engine/core/node.h"

class ControlsNode : public Node {
private:

public:
    ControlsNode();

    ~ControlsNode();
    
    void Update(Viewport *viewport) override;

    void Draw(Viewport *viewport) override;
};