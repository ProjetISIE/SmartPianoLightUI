#include "UI.hpp"
#include "AppController.hpp"
#include "MusicUtils.hpp"
#include <format>

using namespace Colors;

void UI::draw(AppController& app, Vector2 mouse, float screenW, float screenH) {
    if (app.errorTimer_ > 0.0f) {
        float alpha = (app.errorTimer_ < 1.0f) ? app.errorTimer_ : 1.0f;
        DrawRectangle(0, (int)screenH - 50, (int)screenW, 50,
                      Fade(kRougeErreur, alpha * 0.85f));
        DrawText(app.errorMsg_.c_str(),
                 (int)screenW / 2 - MeasureText(app.errorMsg_.c_str(), 18) / 2,
                 (int)screenH - 36, 18, Fade(WHITE, alpha));
    }

    switch (app.appState_) {
    case AppState::PROFILE_SELECT:
        drawProfileSelect(app, mouse, screenW, screenH);
        break;
    case AppState::MENU: drawMenu(app, mouse, screenW, screenH); break;
    case AppState::PLAY: drawPlay(app, mouse, screenW, screenH); break;
    case AppState::GAME_OVER: drawGameOver(app, mouse, screenW, screenH); break;
    }
}

bool UI::drawButton(Rectangle rec, const char* text, Color color, Vector2 mouse,
                    int32_t fontSize) {
    bool hover = CheckCollisionPointRec(mouse, rec);
    if (hover) {
        DrawRectangleRec(rec, Fade(color, 0.3f));
        DrawRectangleLinesEx(rec, 3, color);
    } else {
        DrawRectangleLinesEx(rec, 2, Fade(color, 0.6f));
    }
    int32_t textWidth = MeasureText(text, fontSize);
    DrawText(text, (int32_t)(rec.x + rec.width / 2 - textWidth / 2),
             (int32_t)(rec.y + rec.height / 2 - fontSize / 2), fontSize,
             hover ? WHITE : color);
    return hover;
}

void UI::drawStaff(Rectangle rec, const std::vector<std::string>& notes,
                   Color color) {
    float lineSpacing = rec.height / 6.0f;
    float centerY = rec.y + rec.height / 2.0f;

    // Dessin des 5 lignes (E4, G4, B4, D5, F5 en clé de sol simplifiée)
    for (int i = 0; i < 5; i++) {
        float y = centerY + (2 - i) * lineSpacing;
        DrawLineEx({rec.x, y}, {rec.x + rec.width, y}, 2, color);
    }

    float noteX = rec.x + rec.width / 2.0f;
    float noteRadius = lineSpacing * 0.45f;

    for (const auto& note : notes) {
        NoteKey nk = MusicUtils::resolveKey(note);
        if (!nk.valid) continue;

        float whitePos = static_cast<float>(nk.index);
        float noteY = centerY + (2.0f - whitePos / 2.0f) * lineSpacing;

        // Ledger lines (lignes supplémentaires)
        if (nk.index <= 0) { // C4 et en dessous
            for (int p = 0; p >= nk.index; p -= 2) {
                float ly = centerY +
                           (2.0f - static_cast<float>(p) / 2.0f) * lineSpacing;
                DrawLineEx({noteX - noteRadius * 1.5f, ly},
                           {noteX + noteRadius * 1.5f, ly}, 2, color);
            }
        } else if (nk.index >= 12) { // A5 et au dessus
            for (int p = 12; p <= nk.index; p += 2) {
                float ly = centerY +
                           (2.0f - static_cast<float>(p) / 2.0f) * lineSpacing;
                DrawLineEx({noteX - noteRadius * 1.5f, ly},
                           {noteX + noteRadius * 1.5f, ly}, 2, color);
            }
        }

        // Tête de note (noire)
        DrawEllipse((int)noteX, (int)noteY, noteRadius * 1.2f, noteRadius,
                    color);

        // Hampe (stem)
        float stemLen = lineSpacing * 3.0f;
        if (nk.index < 6) { // Sous la ligne du milieu (B4)
            DrawLineEx({noteX + noteRadius * 1.1f, noteY},
                       {noteX + noteRadius * 1.1f, noteY - stemLen}, 2, color);
        } else {
            DrawLineEx({noteX - noteRadius * 1.1f, noteY},
                       {noteX - noteRadius * 1.1f, noteY + stemLen}, 2, color);
        }

        // Altérations (si note noire)
        if (nk.isBlack) {
            bool isSharp = (note.find('#') != std::string::npos);
            const char* altTxt = isSharp ? "#" : "b";
            DrawText(altTxt, (int)(noteX - noteRadius * 2.5f),
                     (int)(noteY - noteRadius), (int)(lineSpacing * 1.5f),
                     color);
        }
    }
}

void UI::drawProfileSelect(AppController& app, Vector2 mouse, float screenW,
                           float screenH) {
    DrawText("SESSIONS UTILISATEURS",
             (int)screenW / 2 - MeasureText("SESSIONS UTILISATEURS", 30) / 2,
             (int)(screenH * 0.15f), 30, kVertEclatant);

    float totalW = app.profiles_.size() * 200.0f +
                   ((app.profiles_.size() < 4) ? 200.0f : 0.0f) - 20.0f;
    float startX = screenW / 2.0f - totalW / 2.0f;

    for (size_t i = 0; i < app.profiles_.size(); i++) {
        Rectangle r = {startX + i * 200.0f, screenH / 2.0f - 110.0f, 180.0f,
                       220.0f};
        Rectangle rDel = {r.x + 150.0f, r.y + 5.0f, 25.0f, 25.0f};
        bool hov = CheckCollisionPointRec(mouse, r);
        bool hovDel = CheckCollisionPointRec(mouse, rDel);

        DrawRectangleRec(r, Fade(app.profiles_[i].color, hov ? 0.3f : 0.1f));
        DrawRectangleLinesEx(r, hov ? 3 : 2, app.profiles_[i].color);
        DrawText(app.profiles_[i].name.c_str(), (int)r.x + 15, (int)r.y + 100,
                 22, hov ? WHITE : app.profiles_[i].color);
        DrawText(std::format("Record: {}", app.profiles_[i].topScore).c_str(),
                 (int)r.x + 15, (int)r.y + 140, 15,
                 Fade(app.profiles_[i].color, 0.8f));

        DrawRectangleRec(rDel,
                         hovDel ? kRougeErreur : Fade(kRougeErreur, 0.6f));
        DrawText("X", (int)rDel.x + 7, (int)rDel.y + 4, 20, WHITE);
    }

    if (app.profiles_.size() < 4 && !app.isNamingProfile_) {
        Rectangle rP = {startX + app.profiles_.size() * 200.0f,
                        screenH / 2.0f - 110.0f, 180.0f, 220.0f};
        bool hov = CheckCollisionPointRec(mouse, rP);
        DrawRectangleRec(rP, Fade(kVertEclatant, hov ? 0.3f : 0.1f));
        DrawRectangleLinesEx(rP, hov ? 3 : 2, Fade(kVertEclatant, 0.5f));
        DrawText("+", (int)rP.x + 75, (int)rP.y + 80, 60,
                 hov ? WHITE : Fade(kVertEclatant, 0.5f));
    }

    if (app.isNamingProfile_) {
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.8f));
        DrawText("NOM DU NOUVEAU PROFIL :", (int)screenW / 2 - 140,
                 (int)screenH / 2 - 50, 20, kVertEclatant);
        DrawText(app.inputName_.c_str(),
                 (int)screenW / 2 - MeasureText(app.inputName_.c_str(), 40) / 2,
                 (int)screenH / 2, 40, WHITE);
        DrawText("ESC pour annuler", (int)screenW / 2 - 70,
                 (int)screenH / 2 + 60, 15, GRAY);
    }
}

void UI::drawMenu(AppController& app, Vector2 mouse, float screenW,
                  float screenH) {
    DrawText(std::format("JOUEUR: {}", app.profiles_[app.currentUserIdx_].name)
                 .c_str(),
             40, 40, 25, app.profiles_[app.currentUserIdx_].color);
    DrawText(
        std::format("RECORD: {}", app.profiles_[app.currentUserIdx_].topScore)
            .c_str(),
        (int)screenW - 200, 40, 25, kVertEclatant);

    const char* gameTitles[] = {"JEU DE NOTES", "JEU D'ACCORDS",
                                "JEU D'ACCORDS RENVERSES"};
    for (int i = 0; i < 3; i++) {
        Rectangle r = {screenW / 2.0f - 250.0f, screenH * 0.3f + i * 100.0f,
                       500.0f, 80.0f};
        (void)drawButton(r, gameTitles[i], kVertEclatant, mouse, 25);
    }

    // Gamme
    float scaleStartX = screenW / 2.0f - (7.0f * 70.0f) / 2.0f;
    const char* scaleLabels[] = {"C", "D", "E", "F", "G", "A", "B"};
    for (int i = 0; i < 7; i++) {
        Rectangle r = {scaleStartX + i * 70.0f, screenH * 0.75f, 60.0f, 40.0f};
        bool sel = (app.selectedScale_ == static_cast<ScaleChoice>(i));
        bool hov = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r,
                         Fade(kVertEclatant, sel ? 0.3f : (hov ? 0.2f : 0.0f)));
        DrawRectangleLinesEx(r, hov || sel ? 3 : 2,
                             sel ? kVertEclatant : Fade(kVertEclatant, 0.4f));
        DrawText(scaleLabels[i],
                 (int)r.x + 30 - MeasureText(scaleLabels[i], 20) / 2,
                 (int)r.y + 10, 20,
                 (hov || sel) ? WHITE : Fade(kVertEclatant, 0.4f));
    }

    // Mode
    const char* modes[] = {"MAJ", "MIN"};
    for (int i = 0; i < 2; i++) {
        Rectangle r = {screenW / 2.0f - 75.0f + i * 80.0f, screenH * 0.85f,
                       70.0f, 40.0f};
        bool sel = ((i == 0) == (app.selectedMode_ == ModeChoice::MODE_MAJ));
        bool hov = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r,
                         Fade(kVertEclatant, sel ? 0.3f : (hov ? 0.2f : 0.0f)));
        DrawRectangleLinesEx(r, hov || sel ? 3 : 2,
                             sel ? kVertEclatant : Fade(kVertEclatant, 0.4f));
        DrawText(modes[i], (int)r.x + 35 - MeasureText(modes[i], 20) / 2,
                 (int)r.y + 10, 20,
                 (hov || sel) ? WHITE : Fade(kVertEclatant, 0.4f));
    }

    // Affichage interactif des notes de la gamme dans le menu
    std::vector<std::string> menuNotes =
        MusicUtils::getScaleNotesList(app.selectedScale_, app.selectedMode_);
    std::string menuNotesStr = "Notes de la gamme :";
    for (size_t idx = 0; idx < menuNotes.size(); ++idx) {
        menuNotesStr += " " + MusicUtils::noteDisplayLabel(
                                  menuNotes[idx], app.selectedNotation_);
        if (idx + 1 < menuNotes.size()) {
            menuNotesStr += "   ";
        }
    }
    DrawText(menuNotesStr.c_str(),
             (int)screenW / 2 - MeasureText(menuNotesStr.c_str(), 18) / 2,
             (int)(screenH * 0.81f), 18, Fade(kVertEclatant, 0.8f));

    Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.92f, 300.0f,
                         30.0f};
    (void)drawButton(btnBack, "CHANGER D'UTILISATEUR", GRAY, mouse, 15);

    // Toggle clavier
    Rectangle rKbd = {40, screenH - 80, 250, 40};
    DrawRectangleLinesEx(rKbd, 2, app.showKeyboard_ ? kVertEclatant : GRAY);
    DrawText(app.showKeyboard_ ? "CLAVIER : ON" : "CLAVIER : OFF",
             (int)rKbd.x + 20, (int)rKbd.y + 10, 20,
             app.showKeyboard_ ? kVertEclatant : GRAY);

    // Toggle notation
    Rectangle rNotation = {screenW - 290, screenH - 80, 250, 40};
    const char* notationText =
        app.selectedNotation_ == NotationMode::SYLLABIC ? "NOTATION : DO RE MI"
        : app.selectedNotation_ == NotationMode::LETTER ? "NOTATION : A B C"
                                                        : "NOTATION : PORTEE";
    DrawRectangleLinesEx(rNotation, 2, kVertEclatant);
    DrawText(notationText, (int)rNotation.x + 20, (int)rNotation.y + 10, 20,
             kVertEclatant);
}

void UI::drawPlay(AppController& app, Vector2 mouse, float screenW,
                  float screenH) {
    if (!app.isPaused_) {
        // Zone de défi
        Rectangle rChal = {screenW / 2.0f - 175.0f, screenH * 0.25f, 350.0f,
                           200.0f};
        DrawRectangleLinesEx(rChal, 3, kVertEclatant);

        if (app.selectedNotation_ == NotationMode::STAFF &&
            !app.currentChallenge_.expectedNotes.empty()) {
            drawStaff(rChal, app.currentChallenge_.expectedNotes,
                      kVertEclatant);
        } else {
            std::string display;
            if (app.currentChallenge_.expectedNotes.empty()) {
                display = "Attente…";
            } else if (app.currentChallenge_.isChord) {
                display = app.currentChallenge_.displayText;
            } else {
                display = MusicUtils::noteDisplayLabel(
                    app.currentChallenge_.expectedNotes[0],
                    app.selectedNotation_);
            }
            const char* chalTxt = display.c_str();
            int txtSize = (display.size() > 8) ? 28 : 36;
            DrawText(chalTxt,
                     (int)screenW / 2 - MeasureText(chalTxt, txtSize) / 2,
                     (int)rChal.y + 80, txtSize, kVertEclatant);
        }

        if (app.feedbackAlpha_ > 0.0f) {
            DrawText(app.feedbackMsg_,
                     (int)screenW / 2 - MeasureText(app.feedbackMsg_, 40) / 2,
                     (int)(screenH * 0.2f), 40,
                     Fade(app.feedbackColor_, app.feedbackAlpha_));
        }

        Rectangle btnMenu = {25.0f, 25.0f, 120.0f, 45.0f};
        (void)drawButton(btnMenu, "MENU", kVertEclatant, mouse);

        if (app.engState_ == EngineState::ENG_PLAYED) {
            Rectangle btnReady = {160.0f, 25.0f, 160.0f, 45.0f};
            (void)drawButton(btnReady, "SUIVANT", kOrEclatant, mouse);
        }

        // Affichage interactif des notes de la gamme active en jeu
        std::vector<std::string> scaleNotes = MusicUtils::getScaleNotesList(
            app.selectedScale_, app.selectedMode_);
        std::string scaleNameStr =
            "Gamme active : " +
            MusicUtils::getScaleNameFormatted(
                app.selectedScale_, app.selectedMode_, app.selectedNotation_);
        DrawText(scaleNameStr.c_str(),
                 (int)screenW / 2 - MeasureText(scaleNameStr.c_str(), 18) / 2,
                 (int)(rChal.y + rChal.height + 15), 18,
                 Fade(kVertEclatant, 0.7f));

        float boxW = 50.0f;
        float boxH = 40.0f;
        float spacing = 12.0f;
        float startX = screenW / 2.0f - (7.0f * boxW + 6.0f * spacing) / 2.0f;
        float startY = rChal.y + rChal.height + 42.0f;

        for (size_t i = 0; i < scaleNotes.size() && i < 7; ++i) {
            const std::string& scaleNote = scaleNotes[i];
            Rectangle boxRec = {startX + (float)i * (boxW + spacing), startY,
                                boxW, boxH};

            // Check if note is expected in the current challenge
            bool isExpected = false;
            for (const auto& n : app.currentChallenge_.expectedNotes) {
                if (MusicUtils::isSameNoteClass(n, scaleNote)) {
                    isExpected = true;
                    break;
                }
            }

            // Check if note is part of the last result correct/incorrect
            bool isCorrect = false;
            bool isIncorrect = false;
            if (app.lastResult_.active) {
                for (const auto& n : app.lastResult_.correct) {
                    if (MusicUtils::isSameNoteClass(n, scaleNote)) {
                        isCorrect = true;
                        break;
                    }
                }
                for (const auto& n : app.lastResult_.incorrect) {
                    if (MusicUtils::isSameNoteClass(n, scaleNote)) {
                        isIncorrect = true;
                        break;
                    }
                }
            }

            // Check if note is currently pressed on virtual keyboard
            bool isPressed = false;
            if (app.showKeyboard_) {
                int32_t baseKeyboardOctave = MusicUtils::getChallengeBaseOctave(
                    app.currentChallenge_.expectedNotes);
                int32_t numKeys =
                    (app.selectedGame_ == GameType::GAME_NOTE) ? 7 : 14;
                for (int k = 0; k < numKeys; k++) {
                    if (app.blanchesAppuyees_[k]) {
                        int octave = baseKeyboardOctave + (k / 7);
                        std::string noteName =
                            std::string(1, "cdefgab"[k % 7]) +
                            std::to_string(octave);
                        if (MusicUtils::isSameNoteClass(noteName, scaleNote)) {
                            isPressed = true;
                            break;
                        }
                    }
                }
                int32_t numBlack =
                    (app.selectedGame_ == GameType::GAME_NOTE) ? 5 : 10;
                for (int k = 0; k < numBlack; k++) {
                    if (app.noiresAppuyees_[k]) {
                        static constexpr char WHITE_LETTERS[7] = {
                            'c', 'd', 'e', 'f', 'g', 'a', 'b'};
                        int octave =
                            baseKeyboardOctave + (app.kBlackKeyIndices[k] / 7);
                        std::string sharpNote =
                            std::string(
                                1, WHITE_LETTERS[app.kBlackKeyIndices[k] % 7]) +
                            "#" + std::to_string(octave);
                        if (MusicUtils::isSameNoteClass(sharpNote, scaleNote)) {
                            isPressed = true;
                            break;
                        }
                    }
                }
            }

            bool hov = CheckCollisionPointRec(mouse, boxRec);

            // Choose colors based on state
            Color fillCol, borderCol, textCol;
            if (isIncorrect) {
                fillCol = Fade(kRougeErreur, 0.4f);
                borderCol = kRougeErreur;
                textCol = WHITE;
            } else if (isCorrect) {
                fillCol = Fade(kVertEclatant, 0.4f);
                borderCol = kVertEclatant;
                textCol = WHITE;
            } else if (isExpected) {
                fillCol = Fade(kOrangeNote, 0.4f);
                borderCol = kOrangeNote;
                textCol = WHITE;
            } else if (isPressed) {
                fillCol = Fade(app.profiles_[app.currentUserIdx_].color, 0.4f);
                borderCol = app.profiles_[app.currentUserIdx_].color;
                textCol = WHITE;
            } else {
                fillCol = Fade(kVertEclatant, hov ? 0.15f : 0.05f);
                borderCol = hov ? WHITE : Fade(kVertEclatant, 0.3f);
                textCol = hov ? WHITE : Fade(kVertEclatant, 0.7f);
            }

            DrawRectangleRec(boxRec, fillCol);
            DrawRectangleLinesEx(boxRec, hov ? 3 : 2, borderCol);

            std::string dispLabel =
                MusicUtils::noteDisplayLabel(scaleNote, app.selectedNotation_);
            int fontSz = (dispLabel.size() > 2) ? 14 : 18;
            DrawText(dispLabel.c_str(),
                     (int)(boxRec.x + boxRec.width / 2 -
                           MeasureText(dispLabel.c_str(), fontSz) / 2),
                     (int)(boxRec.y + boxRec.height / 2 - fontSz / 2), fontSz,
                     textCol);
        }

        DrawText(std::format("{}", app.scoreActuel_).c_str(),
                 (int)screenW / 2 - 20, 45, 60, kVertEclatant);

        Rectangle btnPause = {screenW - 85.0f, 25.0f, 60.0f, 60.0f};
        (void)drawButton(btnPause, "", kVertEclatant, mouse);
        DrawRectangle((int)btnPause.x + 18, (int)btnPause.y + 18, 8, 24,
                      kVertEclatant);
        DrawRectangle((int)btnPause.x + 34, (int)btnPause.y + 18, 8, 24,
                      kVertEclatant);

        if (app.showKeyboard_) {
            drawVirtualKeyboard(app, screenW, screenH, mouse);
        }
    } else {
        // Overlay de Pause
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.85f));
        Rectangle mB = {screenW / 2.0f - 200.0f, screenH / 2.0f - 150.0f,
                        400.0f, 300.0f};
        DrawRectangleLinesEx(mB, 3, kVertEclatant);
        DrawText("PAUSE", (int)screenW / 2 - 60, (int)screenH / 2 - 110, 40,
                 kVertEclatant);

        Rectangle btnResume = {screenW / 2.0f - 150.0f, screenH / 2.0f - 50.0f,
                               300.0f, 60.0f};
        Rectangle btnQuit = {screenW / 2.0f - 150.0f, screenH / 2.0f + 30.0f,
                             300.0f, 60.0f};
        (void)drawButton(btnResume, "REPRENDRE", kVertEclatant, mouse, 25);
        (void)drawButton(btnQuit, "QUITTER", kRougeErreur, mouse, 25);
    }
}

void UI::drawGameOver(AppController& app, Vector2 mouse, float screenW,
                      float screenH) {
    DrawText("FIN DE PARTIE",
             (int)screenW / 2 - MeasureText("FIN DE PARTIE", 40) / 2,
             (int)(screenH * 0.15f), 40, kVertEclatant);
    DrawText(
        std::format("SCORE FINAL : {}", app.scoreActuel_).c_str(),
        (int)screenW / 2 -
            MeasureText(
                std::format("SCORE FINAL : {}", app.scoreActuel_).c_str(), 30) /
                2,
        (int)(screenH * 0.3f), 30, kOrEclatant);

    DrawText(std::format("Parfaits   : {} / {}", app.gameStats_.perfect,
                         app.gameStats_.total)
                 .c_str(),
             (int)screenW / 2 - 130, (int)(screenH * 0.45f), 22, kVertEclatant);
    DrawText(std::format("Partiels   : {} / {}", app.gameStats_.partial,
                         app.gameStats_.total)
                 .c_str(),
             (int)screenW / 2 - 130, (int)(screenH * 0.5f), 22, kVertEclatant);
    DrawText(std::format("Durée      : {} s", app.gameStats_.duration / 1000)
                 .c_str(),
             (int)screenW / 2 - 130, (int)(screenH * 0.55f), 22, kVertEclatant);

    Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.7f, 300.0f,
                         55.0f};
    (void)drawButton(btnBack, "RETOUR AU MENU", kVertEclatant, mouse, 22);
}

void UI::drawVirtualKeyboard(AppController& app, float screenW, float screenH,
                             Vector2 mouse) {
    int32_t baseKeyboardOctave =
        MusicUtils::getChallengeBaseOctave(app.currentChallenge_.expectedNotes);
    int32_t numKeys = (app.selectedGame_ == GameType::GAME_NOTE) ? 7 : 14;
    int32_t numBlack = (app.selectedGame_ == GameType::GAME_NOTE) ? 5 : 10;
    float wW = screenW / (float)numKeys;
    float pianoY = screenH * 0.7f;
    float pianoH = screenH - pianoY;
    float bW = wW * 0.6f;

    int hovWhite = -1;
    int hovBlack = -1;

    if (!app.isPaused_ && mouse.y > pianoY) {
        for (int i = 0; i < numBlack; i++) {
            Rectangle rN = {(app.kBlackKeyIndices[i] + 1) * wW - bW / 2.0f,
                            pianoY, bW, pianoH * 0.6f};
            if (CheckCollisionPointRec(mouse, rN)) {
                hovBlack = i;
                break;
            }
        }
        if (hovBlack == -1) {
            hovWhite = (int)(mouse.x / wW);
            if (hovWhite < 0 || hovWhite >= numKeys) {
                hovWhite = -1;
            }
        }
    }

    for (int i = 0; i < numKeys; i++) {
        Rectangle r = {i * wW, pianoY, wW - 2.0f, pianoH};
        int octave = baseKeyboardOctave + (i / 7);
        std::string noteName =
            std::string(1, "cdefgab"[i % 7]) + std::to_string(octave);
        NoteKey nk = MusicUtils::resolveKey(noteName, baseKeyboardOctave);
        bool isExpected = false, isCorrectKey = false, isWrongKey = false;

        if (nk.valid && !nk.isBlack) {
            for (const auto& n : app.currentChallenge_.expectedNotes) {
                NoteKey ek = MusicUtils::resolveKey(n, baseKeyboardOctave);
                if (ek.valid && !ek.isBlack && ek.index == nk.index) {
                    isExpected = true;
                }
            }
            if (app.lastResult_.active) {
                for (const auto& n : app.lastResult_.correct) {
                    NoteKey ck = MusicUtils::resolveKey(n, baseKeyboardOctave);
                    if (ck.valid && !ck.isBlack && ck.index == nk.index) {
                        isCorrectKey = true;
                    }
                }
                for (const auto& n : app.lastResult_.incorrect) {
                    NoteKey ik = MusicUtils::resolveKey(n, baseKeyboardOctave);
                    if (ik.valid && !ik.isBlack && ik.index == nk.index) {
                        isWrongKey = true;
                    }
                }
            }
        }

        Color keyColor =
            isWrongKey     ? kRougeErreur
            : isCorrectKey ? kVertEclatant
            : isExpected   ? kOrangeNote
            : app.blanchesAppuyees_[i]
                ? app.profiles_[app.currentUserIdx_].color
            : (hovWhite == i)
                ? Fade(app.profiles_[app.currentUserIdx_].color, 0.3f)
                : BLANK;

        if (keyColor.a != 0) {
            DrawRectangleRec(r, keyColor);
        }
        DrawRectangleLinesEx(
            r, 2, app.isPaused_ ? Fade(kVertEclatant, 0.2f) : kVertEclatant);

        if (numKeys <= 7) {
            std::string label =
                MusicUtils::getNotationLabel(i, app.selectedNotation_);
            DrawText(
                label.c_str(),
                (int)r.x + (int)(wW / 2) - MeasureText(label.c_str(), 18) / 2,
                (int)r.y + (int)pianoH - 30, 18,
                app.isPaused_ ? Fade(kVertEclatant, 0.2f)
                              : (keyColor.a != 0 ? kVertFonce : kVertEclatant));
        }
    }

    for (int i = 0; i < numBlack; i++) {
        Rectangle rN = {(app.kBlackKeyIndices[i] + 1) * wW - bW / 2.0f, pianoY,
                        bW, pianoH * 0.6f};
        static constexpr char WHITE_LETTERS[7] = {'c', 'd', 'e', 'f',
                                                  'g', 'a', 'b'};
        int octave = baseKeyboardOctave + (app.kBlackKeyIndices[i] / 7);
        std::string sharpNote =
            std::string(1, WHITE_LETTERS[app.kBlackKeyIndices[i] % 7]) + "#" +
            std::to_string(octave);
        bool bkExpected = false, bkCorrect = false, bkWrong = false;

        for (const auto& n : app.currentChallenge_.expectedNotes) {
            if (MusicUtils::noteInList(sharpNote, {n})) {
                bkExpected = true;
            }
        }
        if (app.lastResult_.active) {
            for (const auto& n : app.lastResult_.correct) {
                if (MusicUtils::noteInList(sharpNote, {n})) {
                    bkCorrect = true;
                }
            }
            for (const auto& n : app.lastResult_.incorrect) {
                if (MusicUtils::noteInList(sharpNote, {n})) {
                    bkWrong = true;
                }
            }
        }

        Color bkFill =
            bkWrong                  ? kRougeErreur
            : bkCorrect              ? kVertEclatant
            : bkExpected             ? kOrangeNote
            : app.noiresAppuyees_[i] ? app.profiles_[app.currentUserIdx_].color
            : (hovBlack == i)
                ? Fade(app.profiles_[app.currentUserIdx_].color, 0.6f)
            : app.isPaused_ ? Fade(DARKGRAY, 0.5f)
                            : Color{30, 60, 30, 255};

        DrawRectangleRec(rN, bkFill);
        DrawRectangleLinesEx(
            rN, 2, app.isPaused_ ? Fade(kVertEclatant, 0.2f) : kVertEclatant);
    }
}
