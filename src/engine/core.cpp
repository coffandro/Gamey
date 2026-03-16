#include "raylib.h"
#include "../config.h"

#define CLAY_USE_RAYLIB
#define CLAY_UI_IMPLEMENTATION
#include "ui/ui.h"
#undef CLAY_USE_RAYLIB
#undef CLAY_UI_IMPLEMENTATION

#include "core/node.h"
#include "../main_node.h"
#include <vector>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_NAME);
    SetTargetFPS(TARGET_FPS);

    Font fonts[1];
    fonts[0] = LoadFontEx("assets/Born2bSportyV2.ttf", 128, 0, 400);
    SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_BILINEAR);

    ClayInit(SCREEN_WIDTH, SCREEN_HEIGHT, ClayRaylibMeasureText, 0);

    Main main;
    Rectangle viewport = (Rectangle) {
        .x = 0,
        .y = 0,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT
    };

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        ClaySetPointer(mouse.x, mouse.y, IsMouseButtonDown(MOUSE_LEFT_BUTTON));

        viewport.width = GetScreenWidth();
        viewport.height = GetScreenHeight();

        // Update
        main.Update(viewport);

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);
        ClayBeginFrame(GetScreenWidth(), GetScreenHeight());

        /*BeginClay((ClayOptions) {
            .id = "MainRoot",
            .width = CLAY_GROW(),
            .height = CLAY_GROW(),
            .backgroundColor = CLAY_RGB(100, 100, 100),
        });*/
            main.Draw(viewport);
        EndClay();
        
        ClayRenderCommandArray commands = ClayEndFrame();
        ClayRaylibRender(commands, fonts);
        EndDrawing();
    }

    ClayShutdown();
    CloseWindow();

    return 0;
}