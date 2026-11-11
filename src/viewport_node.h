#pragma once
#include "engine/core/node.h"

class ViewportNode : public Node {
private:
    Vector2 pos, vel;

public:
    ViewportNode();

    ~ViewportNode();
    
    void Update(Viewport *viewport) override;

    void Draw(Viewport *viewport) override;
};