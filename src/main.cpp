#include "Communication.hpp"
#include "Logger.hpp"
#include <csignal>
#include <memory>
#include <raylib.h>
#include <string>

// Game state management
enum class GameState {
    DISCONNECTED,
    CONNECTED,
    WAITING_FOR_ACK,
    CONFIGURED,
    WAITING_FOR_CHALLENGE,
    CHALLENGE_ACTIVE,
    SHOWING_RESULT,
    GAME_OVER
};

std::string challenge_text;
std::string result_text;
std::string game_over_text;

/**
 * @brief Gestionnaire de signaux pour arrêts propres
 * @param signum Numéro du signal reçu
 */
void signalHandler(int signum) {
    Logger::log("[MAIN] Signal reçu: {}", signum);
}

int main() {
    Logger::log("[MAIN] Hello Smart Piano");
    GameState current_state = GameState::DISCONNECTED;
    auto comm = std::make_unique<Communication>();
    if (comm->connect()) current_state = GameState::CONNECTED;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Smart Piano Trainer UI");
    SetWindowMinSize(600, 400);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (comm->isConnected()) {
            if (auto msg_opt = comm->popMessage()) {
                Message msg = *msg_opt;
                Logger::log("[MAIN] Processing message: {}", msg.getType());
                if (msg.getType() == "ack") {
                    if (msg.getField("status") == "ok") {
                        current_state = GameState::CONFIGURED;
                    } else {
                        current_state = GameState::CONNECTED;
                        challenge_text =
                            "Config error: " + msg.getField("message");
                    }
                } else if (msg.getType() == "note") {
                    challenge_text = "Play note: " + msg.getField("note");
                    current_state = GameState::CHALLENGE_ACTIVE;
                } else if (msg.getType() == "chord") {
                    challenge_text = "Play chord: " + msg.getField("name") +
                                     " (" + msg.getField("notes") + ")";
                    current_state = GameState::CHALLENGE_ACTIVE;
                } else if (msg.getType() == "result") {
                    result_text = "Correct: " + msg.getField("correct") +
                                  " | Incorrect: " + msg.getField("incorrect");
                    current_state = GameState::SHOWING_RESULT;
                } else if (msg.getType() == "over") {
                    game_over_text =
                        "Game Over! Total: " + msg.getField("total") +
                        ", Perfect: " + msg.getField("perfect");
                    current_state = GameState::GAME_OVER;
                } else if (msg.getType() == "error") {
                    challenge_text = "Engine Error: " + msg.getField("message");
                }
            }
        } else current_state = GameState::DISCONNECTED;
        if (IsKeyPressed(KEY_S)) { // Start Game
            if (current_state == GameState::CONNECTED ||
                current_state == GameState::CONFIGURED ||
                current_state == GameState::GAME_OVER) {
                Message config_msg("config", {{"game", "note"}});
                comm->send(config_msg);
                current_state = GameState::WAITING_FOR_ACK;
            }
        }
        if (IsKeyPressed(KEY_R)) { // Ready for next challenge
            if (current_state == GameState::CONFIGURED ||
                current_state == GameState::SHOWING_RESULT) {
                Message ready_msg("ready");
                comm->send(ready_msg);
                challenge_text = "Attente d’un challenge...";
                current_state = GameState::WAITING_FOR_CHALLENGE;
            }
        }
        if (IsKeyPressed(KEY_Q)) { // Quit game
            if (current_state != GameState::CONNECTED &&
                current_state != GameState::DISCONNECTED) {
                Message quit_msg("quit");
                comm->send(quit_msg);
                current_state = GameState::CONNECTED;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawFPS(10, 10);

        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        int y_pos = 50;

        switch (current_state) {
        case GameState::DISCONNECTED:
            DrawText("Connection au moteur (le lancer avant l’ui)...",
                     screenWidth / 2 - 150, screenHeight / 2 - 10, 30,
                     LIGHTGRAY);
            break;
        case GameState::CONNECTED:
            DrawText("Connecté !", screenWidth / 2 - 80, screenHeight / 2 - 20,
                     30, LIME);
            DrawText("Appuyer 'S' pour lancer un jeu de note",
                     screenWidth / 2 - 150, screenHeight / 2 + 20, 20,
                     DARKGRAY);
            break;
        case GameState::WAITING_FOR_ACK:
            DrawText("Configuration Serveur (partie)...", screenWidth / 2 - 120,
                     screenHeight / 2 - 10, 20, DARKGRAY);
            break;
        case GameState::CONFIGURED:
            DrawText("Serveur configuré !", screenWidth / 2 - 110,
                     screenHeight / 2 - 20, 30, LIME);
            DrawText("Appuyer 'R' (Ready) pour challenge suivant",
                     screenWidth / 2 - 150, screenHeight / 2 + 20, 20,
                     DARKGRAY);
            break;
        case GameState::WAITING_FOR_CHALLENGE:
        case GameState::CHALLENGE_ACTIVE:
            DrawText(challenge_text.c_str(), 100, y_pos, 40, DARKBLUE);
            if (!result_text.empty()) {
                DrawText(result_text.c_str(), 100, y_pos + 60, 20, GRAY);
            }
            break;
        case GameState::SHOWING_RESULT:
            DrawText(challenge_text.c_str(), 100, y_pos, 40, LIGHTGRAY);
            DrawText(result_text.c_str(), 100, y_pos + 60, 30, MAROON);
            DrawText("Appuyer 'R' (Ready) pour challenge suivant", 100,
                     y_pos + 120, 20, DARKGRAY);
            break;
        case GameState::GAME_OVER:
            DrawText(game_over_text.c_str(), 100, y_pos, 40, BLUE);
            DrawText("Appuyer 'S' pour lancer (Start) une partie", 100,
                     y_pos + 60, 20, DARKGRAY);
            break;
        }
        DrawText("S = Start, R = Ready, Q = Quit", 10, screenHeight - 30, 15,
                 GRAY);
        EndDrawing();
    }
    comm->disconnect();
    CloseWindow();
    return 0;
}
