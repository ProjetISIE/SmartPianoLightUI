#include "raylib.h"
#include <string>
#include <vector>

struct UserProfile {
    std::string name;
    int topScore;
    Color color;
};

typedef enum GameState {
    PROFILE_SELECT,
    MENU,
    PLAY_NOTES,
    PLAY_ACCORDS,
    PLAY_RENVERSES
} GameState;

int main() {
    const int screenWidth = 1024;
    const int screenHeight = 768;
    InitWindow(screenWidth, screenHeight, "Piano Tiles - La Totale Sessions");

    Color vertFonce = (Color){20, 40, 20, 255};
    Color vertEclatant = (Color){100, 255, 100, 255};
    Color orEclatant = (Color){255, 215, 0, 255};
    Color rougeErreur = (Color){230, 41, 55, 255};

    std::vector<UserProfile> profiles = {{"JISELE", 250, SKYBLUE}};
    int currentUserIndex = 0;

    // Saisie de nom
    bool isNamingProfile = false;
    char inputName[11] = "\0";
    int letterCount = 0;

    GameState currentState = PROFILE_SELECT;
    bool isPaused = false;

    // Feedback & Jeu
    const char* feedbackMsg = "";
    Color feedbackColor = WHITE;
    float feedbackAlpha = 0.0f;
    const char* encouragements[] = {"MAGNIFIQUE !", "QUEL TALENT !",
                                    "PARFAIT !", "VIRTUOSE !"};
    const char* consolations[] = {"COURAGE !", "CONTINUE !", "PRESQUE !",
                                  "RYTHME !"};

    int scoreActuel = 0;
    float noteTimer = 0.0f;
    const float noteTimeLimit = 5.0f;
    int currentStep = 0;

    std::vector<const char*> partitionNotes = {"DO",  "RE", "MI", "FA",
                                               "SOL", "LA", "SI", "DO"};
    std::vector<const char*> partitionAccords = {"DO MAJ", "RE MIN", "SOL MAJ",
                                                 "LA MIN"};
    std::vector<const char*> partitionRenverses = {"DO (1er)", "FA (2eme)",
                                                   "SOL (1er)"};

    // Rectangles UI
    Rectangle btnPause = {(float)screenWidth - 85, 25, 60, 60};
    Rectangle btnRetourJeu = {25, 25, 120, 45};
    Rectangle btnSuivant = {160, 25, 160, 45};
    Rectangle btnResume = {(float)screenWidth / 2 - 150,
                           (float)screenHeight / 2 - 50, 300, 60};
    Rectangle btnQuit = {(float)screenWidth / 2 - 150,
                         (float)screenHeight / 2 + 30, 300, 60};

    const char* nomsNotes[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};
    bool blanchesAppuyees[8] = {false};
    bool noiresAppuyees[5] = {false};
    int blackKeyIndices[] = {0, 1, 3, 4, 5};

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (feedbackAlpha > 0) feedbackAlpha -= dt * 0.7f;

        // --- 1. LOGIQUE SÉLECTION PROFIL ---
        if (currentState == PROFILE_SELECT) {
            if (isNamingProfile) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (letterCount < 10)) {
                        inputName[letterCount] = (char)key;
                        inputName[letterCount + 1] = '\0';
                        letterCount++;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
                    letterCount--;
                    inputName[letterCount] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                    profiles.push_back(
                        {inputName, 0,
                         (Color){(unsigned char)GetRandomValue(100, 255),
                                 (unsigned char)GetRandomValue(100, 255),
                                 (unsigned char)GetRandomValue(100, 255),
                                 255}});
                    isNamingProfile = false;
                    inputName[0] = '\0';
                    letterCount = 0;
                }
            } else if (clicked) {
                for (int i = 0; i < (int)profiles.size(); i++) {
                    Rectangle r = {(float)100 + (i * 220), 300, 180, 220};
                    Rectangle rDel = {r.x + 150, r.y + 5, 25, 25};
                    if (CheckCollisionPointRec(mouse, rDel)) {
                        profiles.erase(profiles.begin() + i);
                        break;
                    } else if (CheckCollisionPointRec(mouse, r)) {
                        currentUserIndex = i;
                        currentState = MENU;
                        break;
                    }
                }
                Rectangle btnPlus = {(float)100 + (profiles.size() * 220), 300,
                                     180, 220};
                if (profiles.size() < 4 &&
                    CheckCollisionPointRec(mouse, btnPlus))
                    isNamingProfile = true;
            }
        }
        // --- 2. LOGIQUE MENU ---
        else if (currentState == MENU) {
            if (clicked) {
                if (mouse.x > screenWidth / 2 - 250 &&
                    mouse.x < screenWidth / 2 + 250) {
                    if (mouse.y > 250 && mouse.y < 330)
                        currentState = PLAY_NOTES;
                    else if (mouse.y > 350 && mouse.y < 430)
                        currentState = PLAY_ACCORDS;
                    else if (mouse.y > 450 && mouse.y < 530)
                        currentState = PLAY_RENVERSES;
                    scoreActuel = 0;
                    currentStep = 0;
                    noteTimer = 0.0f;
                }
                if (mouse.y > 650) currentState = PROFILE_SELECT;
            }
        }
        // --- 3. LOGIQUE JEU ---
        else {
            if (clicked) {
                if (CheckCollisionPointRec(mouse, btnPause))
                    isPaused = !isPaused;
                else if (isPaused) {
                    if (CheckCollisionPointRec(mouse, btnResume))
                        isPaused = false;
                    if (CheckCollisionPointRec(mouse, btnQuit)) {
                        currentState = MENU;
                        isPaused = false;
                    }
                }
                if (!isPaused) {
                    if (CheckCollisionPointRec(mouse, btnRetourJeu))
                        currentState = MENU;
                    if (CheckCollisionPointRec(mouse, btnSuivant)) {
                        currentStep++;
                        noteTimer = 0.0f;
                    }
                }
            }
            if (!isPaused) {
                noteTimer += dt;
                if (noteTimer >= noteTimeLimit) {
                    noteTimer = 0.0f;
                    currentStep++;
                }
                if (IsKeyPressed(KEY_L)) {
                    feedbackMsg = encouragements[GetRandomValue(0, 3)];
                    feedbackColor = orEclatant;
                    feedbackAlpha = 1.0f;
                    scoreActuel += 10;
                }
                if (IsKeyPressed(KEY_R)) {
                    feedbackMsg = consolations[GetRandomValue(0, 3)];
                    feedbackColor = rougeErreur;
                    feedbackAlpha = 1.0f;
                }

                for (int i = 0; i < 8; i++) {
                    blanchesAppuyees[i] = false;
                    if (i < 5) noiresAppuyees[i] = false;
                }
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse.y > 550) {
                    float wW = (float)screenWidth / 8;
                    float bW = wW * 0.6f;
                    bool hitBlack = false;
                    for (int i = 0; i < 5; i++) {
                        Rectangle rN = {(blackKeyIndices[i] + 1) * wW - bW / 2,
                                        550, bW, 130};
                        if (CheckCollisionPointRec(mouse, rN)) {
                            noiresAppuyees[i] = true;
                            hitBlack = true;
                            break;
                        }
                    }
                    if (!hitBlack) {
                        int idx = (int)(mouse.x / wW);
                        if (idx >= 0 && idx < 8) blanchesAppuyees[idx] = true;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(vertFonce);

        if (currentState == PROFILE_SELECT) {
            DrawText("SESSIONS UTILISATEURS", screenWidth / 2 - 180, 100, 30,
                     vertEclatant);
            for (int i = 0; i < (int)profiles.size(); i++) {
                Rectangle r = {(float)100 + (i * 220), 300, 180, 220};
                DrawRectangleLinesEx(r, 2, profiles[i].color);
                DrawText(profiles[i].name.c_str(), r.x + 15, r.y + 100, 20,
                         profiles[i].color);
                DrawText(TextFormat("Record: %i", profiles[i].topScore),
                         r.x + 15, r.y + 140, 15,
                         Fade(profiles[i].color, 0.6f));
                DrawRectangle(r.x + 150, r.y + 5, 25, 25, rougeErreur);
                DrawText("X", r.x + 157, r.y + 8, 20, WHITE);
            }
            if (profiles.size() < 4 && !isNamingProfile) {
                Rectangle rP = {(float)100 + ((int)profiles.size() * 220), 300,
                                180, 220};
                DrawRectangleLinesEx(rP, 2, Fade(vertEclatant, 0.3f));
                DrawText("+", rP.x + 75, rP.y + 80, 60,
                         Fade(vertEclatant, 0.3f));
            }
            if (isNamingProfile) {
                DrawRectangle(0, 0, screenWidth, screenHeight,
                              Fade(BLACK, 0.8f));
                DrawText("NOM DU NOUVEAU PROFIL :", screenWidth / 2 - 140,
                         screenHeight / 2 - 50, 20, vertEclatant);
                DrawText(inputName,
                         screenWidth / 2 - MeasureText(inputName, 40) / 2,
                         screenHeight / 2, 40, WHITE);
                DrawText("ENTREE pour valider", screenWidth / 2 - 80,
                         screenHeight / 2 + 60, 15, GRAY);
            }
        } else if (currentState == MENU) {
            DrawText(TextFormat("JOUEUR: %s",
                                profiles[currentUserIndex].name.c_str()),
                     40, 40, 25, profiles[currentUserIndex].color);
            DrawText(
                TextFormat("RECORD: %i", profiles[currentUserIndex].topScore),
                750, 40, 25, vertEclatant);
            DrawText("SELECTIONNER UN JEU", screenWidth / 2 - 180, 150, 30,
                     vertEclatant);
            for (int i = 0; i < 3; i++)
                DrawRectangleLines(screenWidth / 2 - 250, 250 + (i * 100), 500,
                                   80, vertEclatant);
            DrawText("JEU DE NOTES", screenWidth / 2 - 90, 275, 25,
                     vertEclatant);
            DrawText("JEU D'ACCORDS", screenWidth / 2 - 105, 375, 25,
                     vertEclatant);
            DrawText("JEU D'ACCORDS RENVERSES", screenWidth / 2 - 175, 475, 25,
                     vertEclatant);
            DrawText("< CHANGER D'UTILISATEUR", screenWidth / 2 - 100, 680, 15,
                     GRAY);
        } else {
            if (!isPaused) {
                const char* txt =
                    (currentState == PLAY_NOTES)
                        ? partitionNotes[currentStep % partitionNotes.size()]
                    : (currentState == PLAY_ACCORDS)
                        ? partitionAccords[currentStep %
                                           partitionAccords.size()]
                        : partitionRenverses[currentStep %
                                             partitionRenverses.size()];
                DrawRectangleLinesEx({(float)screenWidth / 2 - 125,
                                      (float)screenHeight / 2 - 120, 250, 180},
                                     3, vertEclatant);
                DrawText(txt, screenWidth / 2 - MeasureText(txt, 40) / 2,
                         screenHeight / 2 - 60, 40, vertEclatant);
                DrawRectangle(screenWidth / 2 - 125, screenHeight / 2 + 70,
                              (noteTimer / noteTimeLimit) * 250, 8,
                              vertEclatant);
                if (feedbackAlpha > 0)
                    DrawText(feedbackMsg,
                             screenWidth / 2 - MeasureText(feedbackMsg, 40) / 2,
                             210, 40, Fade(feedbackColor, feedbackAlpha));
                DrawRectangleLinesEx(btnRetourJeu, 2, vertEclatant);
                DrawText("MENU", 55, 37, 18, vertEclatant);
                DrawRectangleLinesEx(btnSuivant, 2, vertEclatant);
                DrawText("SUIVANT", 195, 37, 18, vertEclatant);
            }
            DrawText(TextFormat("%i", scoreActuel), screenWidth / 2 - 20, 45,
                     60, vertEclatant);
            DrawText(TextFormat("TOP: %i", profiles[currentUserIndex].topScore),
                     screenWidth / 2 - 35, 15, 20,
                     Fade(profiles[currentUserIndex].color, 0.8f));
            DrawRectangleLinesEx(btnPause, 2, vertEclatant);
            DrawRectangle(btnPause.x + 18, btnPause.y + 18, 8, 24,
                          vertEclatant);
            DrawRectangle(btnPause.x + 34, btnPause.y + 18, 8, 24,
                          vertEclatant);

            float wW = (float)screenWidth / 8;
            for (int i = 0; i < 8; i++) {
                Rectangle r = {i * wW, 550, wW - 2, 218};
                if (blanchesAppuyees[i])
                    DrawRectangleRec(r, profiles[currentUserIndex].color);
                else
                    DrawRectangleLinesEx(r, 2,
                                         isPaused ? Fade(vertEclatant, 0.2f)
                                                  : vertEclatant);
                DrawText(nomsNotes[i], r.x + wW / 2 - 15, r.y + 185, 18,
                         isPaused ? Fade(vertEclatant, 0.2f)
                                  : (blanchesAppuyees[i] ? vertFonce
                                                         : vertEclatant));
            }
            float bW = wW * 0.6f;
            for (int i = 0; i < 5; i++) {
                Rectangle rN = {(blackKeyIndices[i] + 1) * wW - bW / 2, 550, bW,
                                130};
                if (noiresAppuyees[i])
                    DrawRectangleRec(rN, profiles[currentUserIndex].color);
                else {
                    DrawRectangleRec(rN, isPaused ? Fade(vertFonce, 0.5f)
                                                  : (Color){30, 60, 30, 255});
                    DrawRectangleLinesEx(rN, 2,
                                         isPaused ? Fade(vertEclatant, 0.2f)
                                                  : vertEclatant);
                }
            }
            if (isPaused) {
                DrawRectangle(0, 0, screenWidth, screenHeight,
                              Fade(vertFonce, 0.85f));
                Rectangle mB = {screenWidth / 2 - 200, screenHeight / 2 - 150,
                                400, 300};
                DrawRectangleRec(mB, vertFonce);
                DrawRectangleLinesEx(mB, 3, vertEclatant);
                DrawText("PAUSE", screenWidth / 2 - 60, screenHeight / 2 - 110,
                         40, vertEclatant);
                DrawRectangleLinesEx(btnResume, 2, vertEclatant);
                DrawText("REPRENDRE", btnResume.x + 80, btnResume.y + 18, 25,
                         vertEclatant);
                DrawRectangleLinesEx(btnQuit, 2, rougeErreur);
                DrawText("QUITTER", btnQuit.x + 95, btnQuit.y + 18, 25,
                         rougeErreur);
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}