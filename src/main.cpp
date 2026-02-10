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

std::string challengeText;
std::string resultText;
std::string gameOverText;

/**
 * @brief Gestionnaire de signaux pour arrêts propres
 * @param signum Numéro du signal reçu
 */
void signalHandler(int signum) {
    Logger::log("[MAIN] Signal reçu: {}", signum);
}

int main() {
    Logger::log("[MAIN] Démarrage UI Smart Piano");
    GameState currentState = GameState::DISCONNECTED;
    auto comm = std::make_unique<Communication>();
    if (comm->connect()) currentState = GameState::CONNECTED;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Smart Piano Trainer");
    SetWindowMinSize(600, 400);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (comm->isConnected()) {
            if (auto msgOpt = comm->popMessage()) {
                Message msg = *msgOpt;
                Logger::log("[MAIN] Traitement message: {}", msg.getType());
                if (msg.getType() == "ack") {
                    if (msg.getField("status") == "ok") {
                        currentState = GameState::CONFIGURED;
                    } else {
                        currentState = GameState::CONNECTED;
                        challengeText =
                            "Erreur configuration: " + msg.getField("message");
                    }
                } else if (msg.getType() == "note") {
                    challengeText = "Jouez la note: " + msg.getField("note");
                    currentState = GameState::CHALLENGE_ACTIVE;
                } else if (msg.getType() == "chord") {
                    challengeText = "Jouez l'accord: " + msg.getField("name") +
                                    " (" + msg.getField("notes") + ")";
                    currentState = GameState::CHALLENGE_ACTIVE;
                } else if (msg.getType() == "result") {
                    resultText = "Correct: " + msg.getField("correct") +
                                 " | Incorrect: " + msg.getField("incorrect");
                    currentState = GameState::SHOWING_RESULT;
                } else if (msg.getType() == "over") {
                    gameOverText =
                        "Partie terminée ! Total: " + msg.getField("total") +
                        ", Parfait: " + msg.getField("perfect");
                    currentState = GameState::GAME_OVER;
                } else if (msg.getType() == "error") {
                    challengeText = "Erreur moteur: " + msg.getField("message");
                }
            }
        } else currentState = GameState::DISCONNECTED;

        if (IsKeyPressed(KEY_S)) { // Start Game
            if (currentState == GameState::CONNECTED ||
                currentState == GameState::CONFIGURED ||
                currentState == GameState::GAME_OVER) {
                Message configMsg("config", {{"game", "note"}});
                comm->send(configMsg);
                currentState = GameState::WAITING_FOR_ACK;
            }
        }
        if (IsKeyPressed(KEY_R)) { // Ready for next challenge
            if (currentState == GameState::CONFIGURED ||
                currentState == GameState::SHOWING_RESULT) {
                Message readyMsg("ready");
                comm->send(readyMsg);
                challengeText = "En attente du défi...";
                currentState = GameState::WAITING_FOR_CHALLENGE;
            }
        }
        if (IsKeyPressed(KEY_Q)) { // Quit game
            if (currentState != GameState::CONNECTED &&
                currentState != GameState::DISCONNECTED) {
                Message quitMsg("quit");
                comm->send(quitMsg);
                currentState = GameState::CONNECTED;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawFPS(10, 10);

        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        int yPos = 50;

        switch (currentState) {
        case GameState::DISCONNECTED:
            DrawText("Connexion au moteur...", screenWidth / 2 - 150,
                     screenHeight / 2 - 10, 30, LIGHTGRAY);
            break;
        case GameState::CONNECTED:
            DrawText("Connecté !", screenWidth / 2 - 80, screenHeight / 2 - 20,
                     30, LIME);
            DrawText("Appuyer sur 'S' pour commencer", screenWidth / 2 - 150,
                     screenHeight / 2 + 20, 20, DARKGRAY);
            break;
        case GameState::WAITING_FOR_ACK:
            DrawText("Configuration de la partie...", screenWidth / 2 - 120,
                     screenHeight / 2 - 10, 20, DARKGRAY);
            break;
        case GameState::CONFIGURED:
            DrawText("Partie configurée !", screenWidth / 2 - 110,
                     screenHeight / 2 - 20, 30, LIME);
            DrawText("Appuyer sur 'R' pour le prochain défi",
                     screenWidth / 2 - 160, screenHeight / 2 + 20, 20,
                     DARKGRAY);
            break;
        case GameState::WAITING_FOR_CHALLENGE:
        case GameState::CHALLENGE_ACTIVE:
            DrawText(challengeText.c_str(), 100, yPos, 40, DARKBLUE);
            if (!resultText.empty())
                DrawText(resultText.c_str(), 100, yPos + 60, 20, GRAY);
            break;
        case GameState::SHOWING_RESULT:
            DrawText(challengeText.c_str(), 100, yPos, 40, LIGHTGRAY);
            DrawText(resultText.c_str(), 100, yPos + 60, 30, MAROON);
            DrawText("Appuyer sur 'R' pour le prochain défi", 100, yPos + 120,
                     20, DARKGRAY);
            break;
        case GameState::GAME_OVER:
            DrawText(gameOverText.c_str(), 100, yPos, 40, BLUE);
            DrawText("Appuyer sur 'S' pour une nouvelle partie", 100, yPos + 60,
                     20, DARKGRAY);
            break;
        }
        DrawText("S = Commencer, R = Prêt, Q = Quitter", 10, screenHeight - 30,
                 15, GRAY);
        EndDrawing();
    }
    comm->disconnect();
    CloseWindow();
    return 0;
}
