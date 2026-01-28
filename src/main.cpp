#include "Logger.hpp"
#include "UdsTransport.hpp"
#include <csignal>
#include <raylib.h>

static UdsTransport* g_transport = nullptr;

/**
 * @brief Gestionnaire de signaux pour arrêts propres
 * @param signum Numéro du signal reçu
 */
void signalHandler(int signum) {
    Logger::log("[MAIN] Signal reçu: {}", signum);
    if (g_transport) g_transport->stop();
}

int main() {
    std::println("[MAIN] Hello Smart Piano");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 450, "Smart Piano Trainer UI");
    SetWindowMinSize(400, 300);
    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Welcome to Smart Piano!", screenWidth / 2 - 50,
                 screenHeight / 2, 20, DARKGRAY);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    g_transport = nullptr;
    return 0;
}
