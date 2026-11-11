#include "raylib.h"
#include "../config.h"

#include "ui/ui.h"

#include "core/node.h"
#include "../main_node.h"
#include <vector>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_NAME);
    SetTargetFPS(TARGET_FPS);

    Font fonts[1];
    fonts[0] = LoadFontEx("assets/Born2bSportyV2.ttf", 128, 0, 400);
    SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_BILINEAR);

    Main main;

    Viewport viewport = (Viewport){
        .rect = (Rectangle) {
            .x = 0,
            .y = 0,
            .width = SCREEN_WIDTH,
            .height = SCREEN_HEIGHT
        },
        .drawTexture = { 0 },
    };

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        viewport.rect.x = 0;
        viewport.rect.y = 0;
        viewport.rect.width = GetScreenWidth();
        viewport.rect.height = GetScreenHeight();

        // Update
        main.Update(&viewport);

        BeginTextureMode(viewport.drawTexture);
            ClearBackground(BLACK);
            main.Draw(&viewport);
        EndTextureMode();

        // Draw
        BeginDrawing();
        DrawTextureRec(viewport.drawTexture.texture,
            (Rectangle){ 0, 0, (float)viewport.drawTexture.texture.width, (float)-viewport.drawTexture.texture.height },
            (Vector2){ 0, 0 }, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(viewport.drawTexture);
    CloseWindow();

    return 0;
}