#include "raylib.h"
#include <vector>

typedef enum GameState { MENU, PLAY_NOTES, PLAY_ACCORDS, PLAY_RENVERSES } GameState;

int main() {
    const int screenWidth = 1024;
    const int screenHeight = 768;
    InitWindow(screenWidth, screenHeight, "Piano Tiles - Version Finale");

    // Couleurs
    Color vertFonce = (Color){ 20, 40, 20, 255 };    
    Color vertEclatant = (Color){ 100, 255, 100, 255 }; 
    Color orEclatant = (Color){ 255, 215, 0, 255 };
    Color rougeErreur = (Color){ 230, 41, 55, 255 };

    GameState currentState = MENU;
    bool isPaused = false;
    
    // Messages Feedback (L / R)
    const char* feedbackMsg = "";
    Color feedbackColor = WHITE;
    float feedbackAlpha = 0.0f;
    const char* encouragements[] = { "MAGNIFIQUE !", "QUEL TALENT !", "PARFAIT !", "VIRTUOSE !", "INCROYABLE !" };
    const char* consolations[] = { "COURAGE !", "CONTINUE !", "PRESQUE !", "ENCORE !", "RYTHME !" };

    // Données de score et temps
    int scoreActuel = 0; 
    int scoreMax = 231; // Ton record
    float noteTimer = 0.0f;
    const float noteTimeLimit = 5.0f; 
    int currentStep = 0;

    // Partitions
    std::vector<const char*> partitionNotes = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};
    std::vector<const char*> partitionAccords = {"DO MAJ", "RE MIN", "SOL MAJ", "LA MIN"};
    std::vector<const char*> partitionRenverses = {"DO (1er)", "FA (2eme)", "SOL (1er)"};
    
    // UI Rectangles
    Rectangle btnPause = (Rectangle){ (float)screenWidth - 85, 25, 60, 60 };
    Rectangle btnRetourJeu = (Rectangle){ 25, 25, 120, 45 };
    Rectangle btnSuivant = (Rectangle){ 160, 25, 160, 45 };
    Rectangle btnResume = (Rectangle){ (float)screenWidth/2 - 150, (float)screenHeight/2 - 50, 300, 60 };
    Rectangle btnQuit = (Rectangle){ (float)screenWidth/2 - 150, (float)screenHeight/2 + 30, 300, 60 };

    const char* nomsNotes[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};
    bool blanchesAppuyees[8] = { false };
    bool noiresAppuyees[5] = { false };
    int blackKeyIndices[] = {0, 1, 3, 4, 5}; 

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (feedbackAlpha > 0) feedbackAlpha -= dt * 0.7f; 

        if (currentState != MENU && !isPaused) {
            noteTimer += dt;
            if (noteTimer >= noteTimeLimit) { noteTimer = 0.0f; currentStep++; }

            // Raccourcis clavier pour le feedback
            if (IsKeyPressed(KEY_L)) { feedbackMsg = encouragements[GetRandomValue(0, 4)]; feedbackColor = orEclatant; feedbackAlpha = 1.0f; scoreActuel += 10; }
            if (IsKeyPressed(KEY_R)) { feedbackMsg = consolations[GetRandomValue(0, 4)]; feedbackColor = rougeErreur; feedbackAlpha = 1.0f; }
        }

        if (currentState == MENU) {
            if (clicked && mouse.x > screenWidth/2 - 250 && mouse.x < screenWidth/2 + 250) {
                if (mouse.y > 250 && mouse.y < 330) currentState = PLAY_NOTES;
                else if (mouse.y > 350 && mouse.y < 430) currentState = PLAY_ACCORDS;
                else if (mouse.y > 450 && mouse.y < 530) currentState = PLAY_RENVERSES;
                scoreActuel = 0; currentStep = 0; noteTimer = 0.0f; isPaused = false;
            }
        } else {
            if (clicked) {
                if (CheckCollisionPointRec(mouse, btnPause)) isPaused = !isPaused;
                else if (isPaused) {
                    if (CheckCollisionPointRec(mouse, btnResume)) isPaused = false;
                    if (CheckCollisionPointRec(mouse, btnQuit)) { currentState = MENU; isPaused = false; }
                }
                if (!isPaused) {
                    if (CheckCollisionPointRec(mouse, btnRetourJeu)) currentState = MENU;
                    if (CheckCollisionPointRec(mouse, btnSuivant)) { currentStep++; noteTimer = 0.0f; }
                }
            }

            if (!isPaused) {
                // Feedback visuel des touches
                for (int i = 0; i < 8; i++) { blanchesAppuyees[i] = false; if (i < 5) noiresAppuyees[i] = false; }
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse.y > 550) {
                    float wW = (float)screenWidth / 8; float bW = wW * 0.6f; bool hitBlack = false;
                    for (int i = 0; i < 5; i++) {
                        Rectangle rN = (Rectangle){ (blackKeyIndices[i] + 1) * wW - bW/2, 550, bW, 130 };
                        if (CheckCollisionPointRec(mouse, rN)) { noiresAppuyees[i] = true; hitBlack = true; break; }
                    }
                    if (!hitBlack) { int idx = (int)(mouse.x / wW); if (idx >= 0 && idx < 8) blanchesAppuyees[idx] = true; }
                }
            }
        }

        BeginDrawing();
        ClearBackground(vertFonce);

        if (currentState == MENU) {
            // AFFICHAGE DU TOP SCORE DANS LE MENU
            DrawText(TextFormat("TOP SCORE: %i", scoreMax), 780, 40, 25, vertEclatant);
            
            DrawText("SELECTIONNER UN JEU", screenWidth/2 - 180, 150, 30, vertEclatant);
            for(int i=0; i<3; i++) DrawRectangleLines(screenWidth/2 - 250, 250 + (i*100), 500, 80, vertEclatant);
            DrawText("JEU DE NOTES", screenWidth/2 - 90, 275, 25, vertEclatant);
            DrawText("JEU D'ACCORDS", screenWidth/2 - 105, 375, 25, vertEclatant);
            DrawText("JEU D'ACCORDS RENVERSES", screenWidth/2 - 175, 475, 25, vertEclatant);
        } else {
            if (!isPaused) {
                const char* txt = (currentState == PLAY_NOTES) ? partitionNotes[currentStep % partitionNotes.size()] : 
                                  (currentState == PLAY_ACCORDS) ? partitionAccords[currentStep % partitionAccords.size()] : 
                                  partitionRenverses[currentStep % partitionRenverses.size()];

                DrawRectangleLinesEx((Rectangle){(float)screenWidth/2 - 125, (float)screenHeight/2 - 120, 250, 180}, 3, vertEclatant);
                DrawText(txt, screenWidth/2 - MeasureText(txt, 40)/2, screenHeight/2 - 60, 40, vertEclatant);
                DrawRectangle(screenWidth/2 - 125, screenHeight/2 + 70, (noteTimer/noteTimeLimit) * 250, 8, vertEclatant);

                if (feedbackAlpha > 0) DrawText(feedbackMsg, screenWidth/2 - MeasureText(feedbackMsg, 40)/2, 210, 40, Fade(feedbackColor, feedbackAlpha));
                DrawRectangleLinesEx(btnRetourJeu, 2, vertEclatant); DrawText("RETOUR", 45, 37, 18, vertEclatant);
                DrawRectangleLinesEx(btnSuivant, 2, vertEclatant); DrawText("SUIVANT >>", 185, 37, 18, vertEclatant);
            }

            // AFFICHAGE SCORE ET TOP SCORE EN JEU
            DrawText(TextFormat("%i", scoreActuel), screenWidth/2 - 20, 45, 60, vertEclatant);
            DrawText(TextFormat("TOP: %i", scoreMax), screenWidth/2 - 35, 15, 20, vertEclatant);

            DrawRectangleLinesEx(btnPause, 2, vertEclatant);
            DrawRectangle(btnPause.x + 18, btnPause.y + 18, 8, 24, vertEclatant); DrawRectangle(btnPause.x + 34, btnPause.y + 18, 8, 24, vertEclatant);

            // Clavier
            float wW = (float)screenWidth / 8;
            for (int i = 0; i < 8; i++) {
                Rectangle r = (Rectangle){ i * wW, 550, wW - 2, 218 };
                if (blanchesAppuyees[i]) DrawRectangleRec(r, vertEclatant);
                else DrawRectangleLinesEx(r, 2, isPaused ? Fade(vertEclatant, 0.2f) : vertEclatant);
                DrawText(nomsNotes[i], r.x + wW/2 - 15, r.y + 185, 18, isPaused ? Fade(vertEclatant, 0.2f) : (blanchesAppuyees[i] ? vertFonce : vertEclatant));
            }
            float bW = wW * 0.6f;
            for (int i = 0; i < 5; i++) {
                Rectangle rN = (Rectangle){ (blackKeyIndices[i] + 1) * wW - bW/2, 550, bW, 130 };
                if (noiresAppuyees[i]) DrawRectangleRec(rN, vertEclatant);
                else { 
                    DrawRectangleRec(rN, isPaused ? Fade(vertFonce, 0.5f) : (Color){30, 60, 30, 255}); 
                    DrawRectangleLinesEx(rN, 2, isPaused ? Fade(vertEclatant, 0.2f) : vertEclatant); 
                }
            }

            if (isPaused) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(vertFonce, 0.85f));
                Rectangle menuBox = (Rectangle){screenWidth/2 - 200, screenHeight/2 - 150, 400, 300};
                DrawRectangleRec(menuBox, vertFonce); DrawRectangleLinesEx(menuBox, 3, vertEclatant);
                DrawText("PAUSE", screenWidth/2 - MeasureText("PAUSE", 40)/2, screenHeight/2 - 110, 40, vertEclatant);
                DrawRectangleLinesEx(btnResume, 2, vertEclatant); DrawText("REPRENDRE", btnResume.x + 80, btnResume.y + 18, 25, vertEclatant);
                DrawRectangleLinesEx(btnQuit, 2, rougeErreur); DrawText("QUITTER", btnQuit.x + 95, btnQuit.y + 18, 25, rougeErreur);
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}