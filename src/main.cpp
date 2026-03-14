#include "raylib.h"
#include <vector>

typedef enum GameState {
    MENU,
    PLAY_NOTES,
    PLAY_ACCORDS,
    PLAY_RENVERSES,
    IMPORT_MENU
} GameState;

int main() {
    const int screenWidth = 1024;
    const int screenHeight = 768;
    InitWindow(screenWidth, screenHeight, "Piano Tiles - Tablet Pro");

    Color vertFonce = {20, 40, 20, 255};
    Color vertEclatant = {100, 255, 100, 255};

    GameState currentState = MENU;
    bool isPaused = false;

    // Scores
    int scoreActuel = 0;
    int scoreMax = 231;

    // Logique de contenu (différent selon le mode)
    std::vector<const char*> partitionNotes = {"DO",  "RE", "MI", "FA",
                                               "SOL", "LA", "SI", "DO"};
    std::vector<const char*> partitionAccords = {"DO MAJ", "RE MIN", "SOL MAJ",
                                                 "LA MIN"};
    std::vector<const char*> partitionRenverses = {"DO (1er)", "FA (2eme)",
                                                   "SOL (1er)"};

    int noteIndex = 0;
    float noteTimer = 0.0f;
    const float noteTimeLimit = 10.0f;

    // Boutons navigation
    Rectangle btnPause = {screenWidth - 80, 25, 60, 60};
    Rectangle btnRetourJeu = {25, 25, 120, 45};
    Rectangle btnSuivant = {160, 25, 160, 45};

    bool touchesAppuyees[8] = {false};
    const char* nomsNotes[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 finger = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        float dt = GetFrameTime();

        // --- LOGIQUE ---
        if (currentState == MENU) {
            if (clicked) {
                // Détection boutons menu
                if (finger.x > screenWidth / 2 - 250 &&
                    finger.x < screenWidth / 2 + 250) {
                    bool modeChoisi = false;
                    if (finger.y > 250 && finger.y < 330) {
                        currentState = PLAY_NOTES;
                        modeChoisi = true;
                    } else if (finger.y > 350 && finger.y < 430) {
                        currentState = PLAY_ACCORDS;
                        modeChoisi = true;
                    } else if (finger.y > 450 && finger.y < 530) {
                        currentState = PLAY_RENVERSES;
                        modeChoisi = true;
                    }

                    if (modeChoisi) {
                        // REINITIALISATION TOTALE (Modes indépendants)
                        scoreActuel = 0;
                        noteIndex = 0;
                        noteTimer = 0.0f;
                        isPaused = false;
                    }

                    if (finger.y > 630 && finger.y < 680)
                        currentState = IMPORT_MENU;
                }
            }
        } else if (currentState != IMPORT_MENU) {
            // LOGIQUE EN JEU
            if (clicked) {
                if (CheckCollisionPointRec(finger, btnPause))
                    isPaused = !isPaused;
                if (CheckCollisionPointRec(finger, btnRetourJeu))
                    currentState = MENU;
                if (!isPaused && CheckCollisionPointRec(finger, btnSuivant)) {
                    noteIndex++;
                    noteTimer = 0.0f;
                    scoreActuel += 10;
                }
            }

            if (!isPaused) {
                noteTimer += dt;
                if (noteTimer >= noteTimeLimit) {
                    noteIndex++;
                    noteTimer = 0.0f;
                }

                // Limites selon le mode
                int maxNotes = (currentState == PLAY_NOTES)
                                   ? partitionNotes.size()
                               : (currentState == PLAY_ACCORDS)
                                   ? partitionAccords.size()
                                   : partitionRenverses.size();
                if (noteIndex >= maxNotes) noteIndex = 0;

                // Piano
                for (int i = 0; i < 8; i++) touchesAppuyees[i] = false;
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && finger.y > 600) {
                    int idx = finger.x / (screenWidth / 8);
                    if (idx >= 0 && idx < 8) touchesAppuyees[idx] = true;
                }
            }
        }

        // --- DESSIN ---
        BeginDrawing();
        ClearBackground(vertFonce);

        if (currentState == MENU) {
            DrawText(TextFormat("TOP SCORE: %i", scoreMax), 780, 40, 25,
                     vertEclatant);
            DrawText("SELECTIONNER UN JEU", screenWidth / 2 - 180, 150, 30,
                     vertEclatant);

            // Boutons
            DrawRectangleLines(screenWidth / 2 - 250, 250, 500, 80,
                               vertEclatant);
            DrawText("JEU DE NOTES", screenWidth / 2 - 90, 275, 25,
                     vertEclatant);
            DrawRectangleLines(screenWidth / 2 - 250, 350, 500, 80,
                               vertEclatant);
            DrawText("JEU D'ACCORDS", screenWidth / 2 - 105, 375, 25,
                     vertEclatant);
            DrawRectangleLines(screenWidth / 2 - 250, 450, 500, 80,
                               vertEclatant);
            DrawText("JEU D'ACCORDS RENVERSES", screenWidth / 2 - 175, 475, 25,
                     vertEclatant);
            DrawRectangleLines(screenWidth / 2 - 250, 630, 500, 50,
                               vertEclatant);
            DrawText("+ IMPORTER NOUVEAUX JEUX", screenWidth / 2 - 145, 645, 20,
                     vertEclatant);
        } else {
            // AFFICHAGE SCORES
            DrawText(TextFormat("TOP: %i", scoreMax), screenWidth / 2 - 40, 15,
                     20, vertEclatant);
            DrawText(TextFormat("%i", scoreActuel),
                     screenWidth / 2 -
                         MeasureText(TextFormat("%i", scoreActuel), 60) / 2,
                     45, 60, vertEclatant);

            // SYMBOLE PAUSE PROPRE
            DrawRectangleLinesEx(btnPause, 2, vertEclatant);
            if (isPaused) { // Triangle Play
                DrawTriangle({btnPause.x + 20, btnPause.y + 15},
                             {btnPause.x + 20, btnPause.y + 45},
                             {btnPause.x + 45, btnPause.y + 30}, vertEclatant);
            } else { // Deux barres Pause
                DrawRectangle(btnPause.x + 18, btnPause.y + 18, 8, 24,
                              vertEclatant);
                DrawRectangle(btnPause.x + 34, btnPause.y + 18, 8, 24,
                              vertEclatant);
            }

            // CONTENU STATIQUE (Dépend du mode)
            const char* texteAffiche = "";
            if (currentState == PLAY_NOTES)
                texteAffiche = partitionNotes[noteIndex];
            else if (currentState == PLAY_ACCORDS)
                texteAffiche = partitionAccords[noteIndex];
            else if (currentState == PLAY_RENVERSES)
                texteAffiche = partitionRenverses[noteIndex];

            DrawRectangleLinesEx((Rectangle){screenWidth / 2 - 125,
                                             screenHeight / 2 - 150, 250, 200},
                                 3, vertEclatant);
            DrawText(texteAffiche,
                     screenWidth / 2 - MeasureText(texteAffiche, 50) / 2,
                     screenHeight / 2 - 85, 50, vertEclatant);

            // Timer barre
            DrawRectangle(screenWidth / 2 - 125, screenHeight / 2 + 60,
                          (noteTimer / noteTimeLimit) * 250, 8, vertEclatant);

            // Retour / Suivant
            DrawRectangleLinesEx(btnRetourJeu, 2, vertEclatant);
            DrawText("RETOUR", btnRetourJeu.x + 20, btnRetourJeu.y + 12, 18,
                     vertEclatant);
            DrawRectangleLinesEx(btnSuivant, 2, vertEclatant);
            DrawText("SUIVANT >>", btnSuivant.x + 25, btnSuivant.y + 12, 18,
                     vertEclatant);

            // Clavier
            for (int i = 0; i < 8; i++) {
                Rectangle r = {i * (screenWidth / 8.0f) + 5, 620,
                               (screenWidth / 8.0f) - 10, 120};
                if (touchesAppuyees[i]) {
                    DrawRectangleRec(r, vertEclatant);
                    DrawText(nomsNotes[i], r.x + 25, r.y + 45, 25, vertFonce);
                } else {
                    DrawRectangleLinesEx(r, 2, vertEclatant);
                    DrawText(nomsNotes[i], r.x + 25, r.y + 45, 25,
                             vertEclatant);
                }
            }

            if (isPaused)
                DrawRectangle(0, 0, screenWidth, screenHeight,
                              Fade(vertFonce, 0.8f));
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
