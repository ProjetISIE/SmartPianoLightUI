#include "raylib.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 450, "Raylib Project");
    SetWindowMinSize(400, 300);
    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello from Raylib!", screenWidth / 2 - 50, screenHeight / 2,
                 20, DARKGRAY);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
