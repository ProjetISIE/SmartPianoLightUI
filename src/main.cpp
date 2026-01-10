#include "raylib.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 450, "Smart Piano Trainer UI");
    SetWindowMinSize(400, 300);
    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Welcome to Smart Piano!", screenWidth / 2 - 50, screenHeight / 2,
                 20, DARKGRAY);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
