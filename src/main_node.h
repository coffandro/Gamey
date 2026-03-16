#pragma once
#include "engine/core/node.h"
#include "raylib.h"

class Main : public Node {
private:
    Vector2 pos, vel;

public:
    Main();

    ~Main();
    
    void Update(Rectangle viewport) override;

    void Draw(Rectangle viewport) override;
};