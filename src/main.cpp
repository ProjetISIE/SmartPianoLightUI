#include "Communication.hpp"
#include "Logger.hpp"
#include "raylib.h"
#include <csignal>
#include <cstdint>
#include <cstring>
#include <format>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Constantes
// ---------------------------------------------------------------------------

static constexpr int32_t DEFAULT_SCREEN_W{1024};
static constexpr int32_t DEFAULT_SCREEN_H{768};
static constexpr float CONN_RETRY_INTERVAL{2.0f}; ///< Secondes entre tentatives
static constexpr float RESULT_DISPLAY_DURATION{
    2.5f}; ///< Durée affichage résultat
static constexpr int32_t MAX_PROFILES{4};

// ---------------------------------------------------------------------------
// Énumérations
// ---------------------------------------------------------------------------

/// États de l'application
enum class AppState { PROFILE_SELECT, MENU, PLAY, GAME_OVER };

/// États du protocole (côté client)
enum class EngineState {
    ENG_DISCONNECTED,
    ENG_CONNECTED,
    ENG_CONFIGURED,
    ENG_PLAYING,
    ENG_PLAYED
};

/// Gamme sélectionnée
enum class ScaleChoice {
    SCALE_C,
    SCALE_D,
    SCALE_E,
    SCALE_F,
    SCALE_G,
    SCALE_A,
    SCALE_B
};

/// Mode de la gamme
enum class ModeChoice { MODE_MAJ, MODE_MIN };

/// Type de notation
enum class NotationMode { SYLLABIC, LETTER, STAFF };

/// Type de jeu
enum class GameType { GAME_NOTE, GAME_CHORD, GAME_INVERSED };

// ---------------------------------------------------------------------------
// Structures de données
// ---------------------------------------------------------------------------

struct UserProfile {
    std::string name;
    int32_t topScore;
    Color color;
};

/// Défi en cours reçu du moteur
struct Challenge {
    int32_t id{0};
    std::string displayText; ///< Texte affiché (nom note ou accord)
    std::vector<std::string> expectedNotes; ///< Notes à jouer
    bool isChord{false};
    std::string rawName; ///< Nom brut envoyé par le moteur (ex: "c", "c#", "C maj")
};

/// Résultat du dernier challenge
struct ChallengeResult {
    std::vector<std::string> correct;
    std::vector<std::string> incorrect;
    float displayTimer{0.0f}; ///< Minuterie d'affichage
    bool active{false};
};

/// Statistiques de fin de partie
struct GameStats {
    int32_t perfect{0};
    int32_t partial{0};
    int32_t total{0};
    int64_t duration{0};
};

// ---------------------------------------------------------------------------
// Fonctions utilitaires
// ---------------------------------------------------------------------------

/**
 * Dessine un bouton avec effet de survol
 * @param rec Rectangle du bouton
 * @param text Texte à afficher
 * @param color Couleur principale
 * @param mouse Position de la souris
 * @param fontSize Taille de la police
 * @return true si le bouton est survolé
 */
[[nodiscard]] static bool drawButton(Rectangle rec, const char* text,
                                     Color color, Vector2 mouse,
                                     int32_t fontSize = 20) {
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

/// Retourne le chemin du binaire moteur, dans l'ordre de priorité
[[nodiscard]] static std::string findEngineBinary() {
    char exePath[4096]{};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        std::string dir(exePath, static_cast<size_t>(len));
        size_t slash = dir.rfind('/');
        if (slash != std::string::npos) {
            std::string sibling = dir.substr(0, slash + 1) + "engine";
            if (access(sibling.c_str(), X_OK) == 0) return sibling;
        }
    }
#ifdef ENGINE_BIN_PATH
    if (access(ENGINE_BIN_PATH, X_OK) == 0) return ENGINE_BIN_PATH;
#endif
    return "engine";
}

/// Lance le moteur en arrière-plan ; retourne son PID ou -1
[[nodiscard]] static pid_t spawnEngine(const std::string& enginePath) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(enginePath.c_str(), "engine", nullptr);
        execlp("engine", "engine", nullptr);
        _exit(1);
    }
    if (pid > 0) Logger::log("[Main] Moteur démarré (PID {})", pid);
    else Logger::err("[Main] Échec fork pour lancer le moteur");
    return pid;
}

[[nodiscard]] static std::vector<std::string> splitNotes(const std::string& s) {
    std::vector<std::string> notes;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) notes.push_back(tok);
    return notes;
}

[[nodiscard]] static std::string noteLetterToFrench(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
    case 'c': return "DO";
    case 'd': return "RE";
    case 'e': return "MI";
    case 'f': return "FA";
    case 'g': return "SOL";
    case 'a': return "LA";
    case 'b': return "SI";
    default: return std::string(1, c);
    }
}

[[nodiscard]] static std::string noteDisplayLabel(const std::string& note, NotationMode mode) {
    if (note.empty()) return note;
    if (mode == NotationMode::LETTER) {
        std::string label;
        label += (char)std::toupper(static_cast<unsigned char>(note[0]));
        size_t i = 1;
        if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
            label += note[i];
            ++i;
        }
        if (i < note.size()) label += " " + note.substr(i);
        return label;
    } else {
        std::string label = noteLetterToFrench(note[0]);
        size_t i = 1;
        if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
            label += (note[i] == '#') ? "#" : "b";
            ++i;
        }
        if (i < note.size()) label += " " + note.substr(i);
        return label;
    }
}

[[nodiscard]] static std::string getNotationLabel(int32_t whiteIdx, NotationMode mode) {
    static const char* syllabic[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI"};
    static const char* letters[] = {"C", "D", "E", "F", "G", "A", "B"};
    if (mode == NotationMode::LETTER) return letters[whiteIdx % 7];
    return syllabic[whiteIdx % 7];
}

struct NoteKey {
    bool isBlack{false};
    int32_t index{-1};
    bool valid{false};
};

[[nodiscard]] static NoteKey resolveKey(const std::string& note) {
    if (note.empty()) return {};
    char letter = note[0];
    size_t i = 1;
    std::string mod;
    if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
        mod = note[i];
        ++i;
    }

    int32_t octave = 4; // Par défaut octave 4
    if (i < note.size() && std::isdigit(note[i])) {
        octave = note[i] - '0';
    }

    static constexpr int32_t WHITE_MAP[7] = {5, 6, 0, 1, 2, 3, 4};
    if (letter < 'a' || letter > 'g') return {};
    int32_t whiteIdx = WHITE_MAP[static_cast<int>(letter - 'a')];

    // On décale selon l'octave (on supporte octave 4 et 5 pour le 2-octaves)
    // On normalise : octave 4 -> base 0, octave 5 -> base 7
    int32_t baseIdx = (octave - 4) * 7;
    whiteIdx += baseIdx;

    if (mod.empty()) return {false, whiteIdx, true};

    static const int32_t WHITE_TO_BLACK_PER_OCTAVE[7] = {0, 1, -1, 2, 3, 4, -1};
    int32_t localWhiteIdx = whiteIdx % 7;
    int32_t bkLocal = WHITE_TO_BLACK_PER_OCTAVE[localWhiteIdx];
    if (bkLocal < 0) return {};

    int32_t bkIdx = (whiteIdx / 7) * 5 + bkLocal;
    return {true, bkIdx, true};
}

/**
 * Dessine une portée de 5 lignes avec les notes indiquées.
 * @param rec Zone d'affichage
 * @param notes Liste des notes à afficher (format "c4", "c#4", etc.)
 * @param color Couleur des lignes et des notes
 */
static void drawStaff(Rectangle rec, const std::vector<std::string>& notes,
                      Color color) {
    float lineSpacing = rec.height / 6.0f;
    float centerY = rec.y + rec.height / 2.0f;

    // Dessin des 5 lignes (E4, G4, B4, D5, F5 en clé de sol simplifiée)
    // On centre la portée verticalement
    for (int i = 0; i < 5; i++) {
        float y = centerY + (2 - i) * lineSpacing;
        DrawLineEx({rec.x, y}, {rec.x + rec.width, y}, 2, color);
    }

    // Dessin des notes
    float noteX = rec.x + rec.width / 2.0f;
    float noteRadius = lineSpacing * 0.45f;

    for (const auto& note : notes) {
        NoteKey nk = resolveKey(note);
        if (!nk.valid) continue;

        // On calcule la position verticale par rapport à C4 (pos 0)
        // En clé de sol, E4 est sur la première ligne (i=0, pos 2)
        // C4 est 2 positions sous la première ligne (pos 0)
        // Position relative en demi-espaces : C4=0, D4=1, E4=2...
        // y = centerY + (2 - pos/2) * lineSpacing
        float whitePos = static_cast<float>(nk.index);
        float noteY = centerY + (2.0f - whitePos / 2.0f) * lineSpacing;

        // Ledger lines (lignes supplémentaires)
        if (nk.index <= 0) { // C4 et en dessous
            for (int p = 0; p >= nk.index; p -= 2) {
                float ly = centerY + (2.0f - static_cast<float>(p) / 2.0f) * lineSpacing;
                DrawLineEx({noteX - noteRadius * 1.5f, ly},
                           {noteX + noteRadius * 1.5f, ly}, 2, color);
            }
        } else if (nk.index >= 12) { // A5 et au dessus
            for (int p = 12; p <= nk.index; p += 2) {
                float ly = centerY + (2.0f - static_cast<float>(p) / 2.0f) * lineSpacing;
                DrawLineEx({noteX - noteRadius * 1.5f, ly},
                           {noteX + noteRadius * 1.5f, ly}, 2, color);
            }
        }

        // Tête de note (noire/quarter note)
        DrawEllipse((int)noteX, (int)noteY, noteRadius * 1.2f, noteRadius, color);

        // Hampe (stem) - vers le haut si note basse, vers le bas si note haute
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
            // On cherche si c'est un # ou b dans la chaîne originale
            bool isSharp = (note.find('#') != std::string::npos);
            const char* altTxt = isSharp ? "#" : "b";
            DrawText(altTxt, (int)(noteX - noteRadius * 2.5f),
                     (int)(noteY - noteRadius), (int)(lineSpacing * 1.5f),
                     color);
        }
    }
}

[[nodiscard]] static bool noteInList(const std::string& noteBase,
                                     const std::vector<std::string>& list) {
    auto strip = [](const std::string& n) {
        if (n.empty()) return n;
        std::string s;
        s += n[0];
        if (n.size() > 1 && (n[1] == '#' || n[1] == 'b')) s += n[1];
        return s;
    };
    std::string base = strip(noteBase);
    for (const auto& n : list)
        if (strip(n) == base) return true;
    return false;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    int timeoutMs = -1;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            timeoutMs = std::stoi(argv[i + 1]);
            ++i;
        }
    }

    Logger::init();
    Logger::log("[Main] Démarrage Smart Piano UI");

    std::string enginePath = findEngineBinary();
    Logger::log("[Main] Chemin moteur: {}", enginePath);
    pid_t enginePid = -1;
    if (!enginePath.empty() && enginePath[0] == '/') {
        enginePid = spawnEngine(enginePath);
        usleep(500'000);
    }

    Communication comm;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(DEFAULT_SCREEN_W, DEFAULT_SCREEN_H, "Smart Piano");
    SetTargetFPS(60);

    const Color vertFonce = {20, 40, 20, 255};
    const Color vertEclatant = {100, 255, 100, 255};
    const Color orEclatant = {255, 215, 0, 255};
    const Color rougeErreur = {230, 41, 55, 255};
    const Color orangeNote = {255, 140, 0, 255};

    std::vector<UserProfile> profiles{{"Utilisateur", 0, SKYBLUE}};
    int currentUserIdx = 0;
    bool isNamingProfile = false;
    char inputName[11] = "\0";
    int letterCount = 0;

    AppState appState = AppState::PROFILE_SELECT;
    bool isPaused = false;

    ScaleChoice selectedScale = ScaleChoice::SCALE_C;
    ModeChoice selectedMode = ModeChoice::MODE_MAJ;
    GameType selectedGame = GameType::GAME_NOTE;

    int scoreActuel = 0;
    NotationMode selectedNotation = NotationMode::SYLLABIC;

    Challenge currentChallenge;
    ChallengeResult lastResult;
    GameStats gameStats;

    const char* encouragements[] = {"MAGNIFIQUE !", "QUEL TALENT !",
                                    "PARFAIT !", "VIRTUOSE !"};
    const char* consolations[] = {"COURAGE !", "CONTINUE !", "PRESQUE !",
                                  "RYTHME !"};
    const char* feedbackMsg = "";
    Color feedbackColor = WHITE;
    float feedbackAlpha = 0.0f;

    std::string errorMsg;
    float errorTimer = 0.0f;

    EngineState engState = EngineState::ENG_DISCONNECTED;
    float connRetryTimer = 0.0f;

    bool blanchesAppuyees[14]{};
    bool noiresAppuyees[10]{};
    const int blackKeyIndices[] = {0, 1, 3, 4, 5, 7, 8, 10, 11, 12};
    bool showKeyboard = true;

    float timeoutTimer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        Vector2 dpiScale = GetWindowScaleDPI();
        mouse.x *= dpiScale.x;
        mouse.y *= dpiScale.y;
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();

        if (timeoutMs > 0) {
            timeoutTimer += dt * 1000.0f;
            if (timeoutTimer >= (float)timeoutMs) break;
        }

        if (feedbackAlpha > 0.0f) feedbackAlpha -= dt * 0.7f;
        if (errorTimer > 0.0f) errorTimer -= dt;
        if (lastResult.active) {
            lastResult.displayTimer -= dt;
            if (lastResult.displayTimer <= 0.0f) {
                lastResult.active = false;
                lastResult.correct.clear();
                lastResult.incorrect.clear();
                if (engState == EngineState::ENG_PLAYED) {
                    comm.send(Message("ready"));
                    engState = EngineState::ENG_PLAYING;
                }
            }
        }

        // ===================================================================
        // Messages entrants
        // ===================================================================
        {
            auto msgOpt = comm.popMessage();
            while (msgOpt.has_value()) {
                const Message& msg = msgOpt.value();
                const std::string& type = msg.getType();

                if (type == "ack") {
                    if (msg.getField("status") == "ok") {
                        engState = EngineState::ENG_CONFIGURED;
                        comm.send(Message("ready"));
                        engState = EngineState::ENG_PLAYING;
                    } else {
                        errorMsg = std::format("Config invalide: {}",
                                               msg.getField("message"));
                        errorTimer = 5.0f;
                        appState = AppState::MENU;
                    }
                } else if (type == "note") {
                    currentChallenge.id =
                        msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
                    std::string noteName = msg.getField("note");
                    currentChallenge.rawName = noteName;
                    currentChallenge.displayText = noteDisplayLabel(noteName, selectedNotation);
                    currentChallenge.expectedNotes = {noteName};
                    currentChallenge.isChord = false;
                    engState = EngineState::ENG_PLAYING;
                } else if (type == "chord") {
                    currentChallenge.id =
                        msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
                    currentChallenge.rawName = msg.getField("name");
                    currentChallenge.displayText = currentChallenge.rawName;
                    currentChallenge.expectedNotes =
                        splitNotes(msg.getField("notes"));
                    currentChallenge.isChord = true;
                    engState = EngineState::ENG_PLAYING;
                } else if (type == "result") {
                    lastResult.correct = splitNotes(msg.getField("correct"));
                    lastResult.incorrect =
                        splitNotes(msg.getField("incorrect"));
                    lastResult.displayTimer = RESULT_DISPLAY_DURATION;
                    lastResult.active = true;

                    bool isCorrect = lastResult.incorrect.empty() &&
                                     !lastResult.correct.empty();
                    bool isPartial = !lastResult.incorrect.empty() &&
                                     !lastResult.correct.empty();

                    if (isCorrect) {
                        feedbackMsg = encouragements[GetRandomValue(0, 3)];
                        feedbackColor = orEclatant;
                        scoreActuel += 10;
                    } else if (isPartial) {
                        feedbackMsg = consolations[GetRandomValue(0, 3)];
                        feedbackColor = orangeNote;
                        scoreActuel += 5;
                    } else {
                        feedbackMsg = consolations[GetRandomValue(0, 3)];
                        feedbackColor = rougeErreur;
                    }
                    feedbackAlpha = 1.0f;
                    engState = EngineState::ENG_PLAYED;
                } else if (type == "over") {
                    gameStats.perfect = msg.hasField("perfect")
                                            ? std::stoi(msg.getField("perfect"))
                                            : 0;
                    gameStats.partial = msg.hasField("partial")
                                            ? std::stoi(msg.getField("partial"))
                                            : 0;
                    gameStats.total = msg.hasField("total")
                                          ? std::stoi(msg.getField("total"))
                                          : 0;
                    gameStats.duration =
                        msg.hasField("duration")
                            ? std::stoll(msg.getField("duration"))
                            : 0LL;
                    if (scoreActuel > profiles[currentUserIdx].topScore)
                        profiles[currentUserIdx].topScore = scoreActuel;
                    engState = EngineState::ENG_CONNECTED;
                    appState = AppState::GAME_OVER;
                } else if (type == "error") {
                    errorMsg =
                        std::format("Erreur: {}", msg.getField("message"));
                    errorTimer = 5.0f;
                    if (msg.getField("code") == "internal") {
                        engState = EngineState::ENG_CONNECTED;
                        appState = AppState::MENU;
                    }
                }
                msgOpt = comm.popMessage();
            }
        }

        if (engState != EngineState::ENG_DISCONNECTED && !comm.isConnected()) {
            engState = EngineState::ENG_DISCONNECTED;
            if (appState == AppState::PLAY || appState == AppState::GAME_OVER)
                appState = AppState::MENU;
        }

        if (engState == EngineState::ENG_DISCONNECTED) {
            connRetryTimer -= dt;
            if (connRetryTimer <= 0.0f) {
                if (comm.connect()) engState = EngineState::ENG_CONNECTED;
                connRetryTimer = CONN_RETRY_INTERVAL;
            }
        }

        // ===================================================================
        // Logique
        // ===================================================================

        if (appState == AppState::PROFILE_SELECT) {
            if (isNamingProfile) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= 32 && key <= 125 && letterCount < 10) {
                        inputName[letterCount++] = (char)key;
                        inputName[letterCount] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0)
                    inputName[--letterCount] = '\0';
                if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                    Color c = {(unsigned char)GetRandomValue(100, 255),
                               (unsigned char)GetRandomValue(100, 255),
                               (unsigned char)GetRandomValue(100, 255), 255};
                    profiles.push_back({inputName, 0, c});
                    isNamingProfile = false;
                    inputName[0] = '\0';
                    letterCount = 0;
                }
                if (IsKeyPressed(KEY_ESCAPE)) isNamingProfile = false;
            } else if (clicked) {
                float totalW =
                    profiles.size() * 200.0f +
                    (profiles.size() < MAX_PROFILES ? 200.0f : 0.0f) - 20.0f;
                float startX = screenW / 2.0f - totalW / 2.0f;
                for (int i = 0; i < (int)profiles.size(); i++) {
                    Rectangle r = {startX + i * 200.0f, screenH / 2.0f - 110.0f,
                                   180.0f, 220.0f};
                    Rectangle rDel = {r.x + 150.0f, r.y + 5.0f, 25.0f, 25.0f};
                    if (CheckCollisionPointRec(mouse, rDel)) {
                        profiles.erase(profiles.begin() + i);
                        break;
                    }
                    if (CheckCollisionPointRec(mouse, r)) {
                        currentUserIdx = i;
                        scoreActuel = 0;
                        appState = AppState::MENU;
                        break;
                    }
                }
                if ((int)profiles.size() < MAX_PROFILES) {
                    Rectangle btnPlus = {startX + (int)profiles.size() * 200.0f,
                                         screenH / 2.0f - 110.0f, 180.0f,
                                         220.0f};
                    if (CheckCollisionPointRec(mouse, btnPlus))
                        isNamingProfile = true;
                }
            }
        } else if (appState == AppState::MENU) {
            if (clicked) {
                // Toggle clavier
                Rectangle rKbd = {40, screenH - 80, 220, 40};
                if (CheckCollisionPointRec(mouse, rKbd)) {
                    showKeyboard = !showKeyboard;
                }

                // Gamme
                float scaleStartX = screenW / 2.0f - (7.0f * 70.0f) / 2.0f;
                for (int i = 0; i < 7; i++) {
                    Rectangle r = {scaleStartX + i * 70.0f, screenH * 0.75f,
                                   60.0f, 40.0f};
                    if (CheckCollisionPointRec(mouse, r))
                        selectedScale = static_cast<ScaleChoice>(i);
                }
                // Mode
                Rectangle rMaj = {screenW / 2.0f - 75.0f, screenH * 0.85f,
                                  70.0f, 40.0f};
                Rectangle rMin = {screenW / 2.0f + 5.0f, screenH * 0.85f, 70.0f,
                                  40.0f};
                if (CheckCollisionPointRec(mouse, rMaj))
                    selectedMode = ModeChoice::MODE_MAJ;
                if (CheckCollisionPointRec(mouse, rMin))
                    selectedMode = ModeChoice::MODE_MIN;

                // Jeu
                auto startGame = [&](GameType gt) {
                    if (engState == EngineState::ENG_DISCONNECTED) {
                        errorMsg = "Moteur non connecté";
                        errorTimer = 3.0f;
                        return;
                    }
                    selectedGame = gt;
                    scoreActuel = 0;
                    currentChallenge = {};
                    lastResult = {};
                    feedbackAlpha = 0.0f;
                    appState = AppState::PLAY;

                    const char* scaleStr[] = {"c", "d", "e", "f",
                                              "g", "a", "b"};
                    comm.send(Message(
                        "config",
                        {{"game", gt == GameType::GAME_NOTE    ? "note"
                                  : gt == GameType::GAME_CHORD ? "chord"
                                                               : "inversed"},
                         {"scale", scaleStr[static_cast<int>(selectedScale)]},
                         {"mode", selectedMode == ModeChoice::MODE_MAJ
                                      ? "maj"
                                      : "min"}}));
                };

                for (int i = 0; i < 3; i++) {
                    Rectangle r = {screenW / 2.0f - 250.0f,
                                   screenH * 0.3f + i * 100.0f, 500.0f, 80.0f};
                    if (CheckCollisionPointRec(mouse, r))
                        startGame(static_cast<GameType>(i));
                }

                Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.92f,
                                     300.0f, 30.0f};
                if (CheckCollisionPointRec(mouse, btnBack))
                    appState = AppState::PROFILE_SELECT;

                // Toggle notation
                Rectangle rNotation = {screenW - 290, screenH - 80, 250, 40};
                if (CheckCollisionPointRec(mouse, rNotation)) {
                    selectedNotation = static_cast<NotationMode>(
                        (static_cast<int>(selectedNotation) + 1) % 3);
                }
            }
        } else if (appState == AppState::PLAY) {
            Rectangle btnPause = {screenW - 85.0f, 25.0f, 60.0f, 60.0f};
            Rectangle btnMenu = {25.0f, 25.0f, 120.0f, 45.0f};
            Rectangle btnReady = {160.0f, 25.0f, 160.0f, 45.0f};

            if (clicked) {
                if (CheckCollisionPointRec(mouse, btnPause))
                    isPaused = !isPaused;
                else if (isPaused) {
                    Rectangle btnResume = {screenW / 2.0f - 150.0f,
                                           screenH / 2.0f - 50.0f, 300.0f,
                                           60.0f};
                    Rectangle btnQuit = {screenW / 2.0f - 150.0f,
                                         screenH / 2.0f + 30.0f, 300.0f, 60.0f};
                    if (CheckCollisionPointRec(mouse, btnResume))
                        isPaused = false;
                    if (CheckCollisionPointRec(mouse, btnQuit)) {
                        comm.send(Message("quit"));
                        comm.clearQueue();
                        engState = EngineState::ENG_CONNECTED;
                        appState = AppState::MENU;
                        isPaused = false;
                    }
                } else {
                    if (CheckCollisionPointRec(mouse, btnMenu)) {
                        comm.send(Message("quit"));
                        comm.clearQueue();
                        engState = EngineState::ENG_CONNECTED;
                        appState = AppState::MENU;
                    }
                    if (engState == EngineState::ENG_PLAYED &&
                        CheckCollisionPointRec(mouse, btnReady)) {
                        comm.send(Message("ready"));
                        engState = EngineState::ENG_PLAYING;
                        lastResult.active = false;
                    }
                }
            }

            if (!isPaused && showKeyboard) {
                for (auto& b : blanchesAppuyees) b = false;
                for (auto& b : noiresAppuyees) b = false;
                float pianoY = screenH * 0.7f;
                int32_t numKeys =
                    (selectedGame == GameType::GAME_NOTE) ? 7 : 14;
                int32_t numBlack =
                    (selectedGame == GameType::GAME_NOTE) ? 5 : 10;
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse.y > pianoY) {
                    float wW = screenW / (float)numKeys;
                    float bW = wW * 0.6f;
                    bool hitBlack = false;
                    for (int i = 0; i < numBlack; i++) {
                        Rectangle rN = {(blackKeyIndices[i] + 1) * wW - bW / 2,
                                        pianoY, bW, (screenH - pianoY) * 0.6f};
                        if (CheckCollisionPointRec(mouse, rN)) {
                            noiresAppuyees[i] = true;
                            hitBlack = true;
                            break;
                        }
                    }
                    if (!hitBlack) {
                        int idx = (int)(mouse.x / wW);
                        if (idx >= 0 && idx < numKeys)
                            blanchesAppuyees[idx] = true;
                    }
                }
            }
        } else if (appState == AppState::GAME_OVER) {
            if (clicked) {
                Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.7f,
                                     300.0f, 55.0f};
                if (CheckCollisionPointRec(mouse, btnBack)) {
                    appState = AppState::MENU;
                    currentChallenge = {};
                    lastResult = {};
                }
            }
        }

        // ===================================================================
        // Rendu
        // ===================================================================
        BeginDrawing();
        ClearBackground(BLACK);

        if (errorTimer > 0.0f) {
            float alpha = (errorTimer < 1.0f) ? errorTimer : 1.0f;
            DrawRectangle(0, (int)screenH - 50, (int)screenW, 50,
                          Fade(rougeErreur, alpha * 0.85f));
            DrawText(errorMsg.c_str(),
                     (int)screenW / 2 - MeasureText(errorMsg.c_str(), 18) / 2,
                     (int)screenH - 36, 18, Fade(WHITE, alpha));
        }

        if (appState == AppState::PROFILE_SELECT) {
            DrawText("SESSIONS UTILISATEURS",
                     (int)screenW / 2 -
                         MeasureText("SESSIONS UTILISATEURS", 30) / 2,
                     (int)(screenH * 0.15f), 30, vertEclatant);
            float totalW = profiles.size() * 200.0f +
                           (profiles.size() < MAX_PROFILES ? 200.0f : 0.0f) -
                           20.0f;
            float startX = screenW / 2.0f - totalW / 2.0f;
            for (int i = 0; i < (int)profiles.size(); i++) {
                Rectangle r = {startX + i * 200.0f, screenH / 2.0f - 110.0f,
                               180.0f, 220.0f};
                Rectangle rDel = {r.x + 150.0f, r.y + 5.0f, 25.0f, 25.0f};
                bool hov = CheckCollisionPointRec(mouse, r);
                bool hovDel = CheckCollisionPointRec(mouse, rDel);

                DrawRectangleRec(r, Fade(profiles[i].color, hov ? 0.3f : 0.1f));
                DrawRectangleLinesEx(r, hov ? 3 : 2, profiles[i].color);
                DrawText(profiles[i].name.c_str(), (int)r.x + 15,
                         (int)r.y + 100, 22, hov ? WHITE : profiles[i].color);
                DrawText(TextFormat("Record: %i", profiles[i].topScore),
                         (int)r.x + 15, (int)r.y + 140, 15,
                         Fade(profiles[i].color, 0.8f));

                DrawRectangleRec(rDel, hovDel ? rougeErreur
                                              : Fade(rougeErreur, 0.6f));
                DrawText("X", (int)rDel.x + 7, (int)rDel.y + 4, 20, WHITE);
            }
            if ((int)profiles.size() < MAX_PROFILES && !isNamingProfile) {
                Rectangle rP = {startX + (int)profiles.size() * 200.0f,
                                screenH / 2.0f - 110.0f, 180.0f, 220.0f};
                bool hov = CheckCollisionPointRec(mouse, rP);
                DrawRectangleRec(rP, Fade(vertEclatant, hov ? 0.3f : 0.1f));
                DrawRectangleLinesEx(rP, hov ? 3 : 2, Fade(vertEclatant, 0.5f));
                DrawText("+", (int)rP.x + 75, (int)rP.y + 80, 60,
                         hov ? WHITE : Fade(vertEclatant, 0.5f));
            }
            if (isNamingProfile) {
                DrawRectangle(0, 0, (int)screenW, (int)screenH,
                              Fade(BLACK, 0.8f));
                DrawText("NOM DU NOUVEAU PROFIL :", (int)screenW / 2 - 140,
                         (int)screenH / 2 - 50, 20, vertEclatant);
                DrawText(inputName,
                         (int)screenW / 2 - MeasureText(inputName, 40) / 2,
                         (int)screenH / 2, 40, WHITE);
                DrawText("ESC pour annuler", (int)screenW / 2 - 70,
                         (int)screenH / 2 + 60, 15, GRAY);
            }
        } else if (appState == AppState::MENU) {
            DrawText(
                TextFormat("JOUEUR: %s", profiles[currentUserIdx].name.c_str()),
                40, 40, 25, profiles[currentUserIdx].color);
            DrawText(
                TextFormat("RECORD: %i", profiles[currentUserIdx].topScore),
                (int)screenW - 200, 40, 25, vertEclatant);

            const char* gameTitles[] = {"JEU DE NOTES", "JEU D'ACCORDS",
                                        "JEU D'ACCORDS RENVERSES"};
            for (int i = 0; i < 3; i++) {
                Rectangle r = {screenW / 2.0f - 250.0f,
                               screenH * 0.3f + i * 100.0f, 500.0f, 80.0f};
                (void)drawButton(r, gameTitles[i], vertEclatant, mouse, 25);
            }

            // Gamme
            float scaleStartX = screenW / 2.0f - (7.0f * 70.0f) / 2.0f;
            const char* scaleLabels[] = {"C", "D", "E", "F", "G", "A", "B"};
            for (int i = 0; i < 7; i++) {
                Rectangle r = {scaleStartX + i * 70.0f, screenH * 0.75f, 60.0f,
                               40.0f};
                bool sel = (selectedScale == static_cast<ScaleChoice>(i));
                bool hov = CheckCollisionPointRec(mouse, r);
                DrawRectangleRec(
                    r, Fade(vertEclatant, sel ? 0.3f : (hov ? 0.2f : 0.0f)));
                DrawRectangleLinesEx(r, hov || sel ? 3 : 2,
                                     sel ? vertEclatant
                                         : Fade(vertEclatant, 0.4f));
                DrawText(scaleLabels[i],
                         (int)r.x + 30 - MeasureText(scaleLabels[i], 20) / 2,
                         (int)r.y + 10, 20,
                         (hov || sel) ? WHITE : Fade(vertEclatant, 0.4f));
            }

            // Mode
            const char* modes[] = {"MAJ", "MIN"};
            for (int i = 0; i < 2; i++) {
                Rectangle r = {screenW / 2.0f - 75.0f + i * 80.0f,
                               screenH * 0.85f, 70.0f, 40.0f};
                bool sel = ((i == 0) == (selectedMode == ModeChoice::MODE_MAJ));
                bool hov = CheckCollisionPointRec(mouse, r);
                DrawRectangleRec(
                    r, Fade(vertEclatant, sel ? 0.3f : (hov ? 0.2f : 0.0f)));
                DrawRectangleLinesEx(r, hov || sel ? 3 : 2,
                                     sel ? vertEclatant
                                         : Fade(vertEclatant, 0.4f));
                DrawText(modes[i],
                         (int)r.x + 35 - MeasureText(modes[i], 20) / 2,
                         (int)r.y + 10, 20,
                         (hov || sel) ? WHITE : Fade(vertEclatant, 0.4f));
            }

            Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.92f,
                                 300.0f, 30.0f};
            (void)drawButton(btnBack, "CHANGER D'UTILISATEUR", GRAY, mouse, 15);

            // Toggle clavier
            Rectangle rKbd = {40, screenH - 80, 250, 40};
            DrawRectangleLinesEx(rKbd, 2, showKeyboard ? vertEclatant : GRAY);
            DrawText(showKeyboard ? "CLAVIER : ON" : "CLAVIER : OFF",
                     (int)rKbd.x + 20, (int)rKbd.y + 10, 20,
                     showKeyboard ? vertEclatant : GRAY);

            // Toggle notation
            Rectangle rNotation = {screenW - 290, screenH - 80, 250, 40};
            const char* notationText =
                selectedNotation == NotationMode::SYLLABIC ? "NOTATION : DO RE MI"
                : selectedNotation == NotationMode::LETTER ? "NOTATION : A B C"
                                                           : "NOTATION : PORTEE";
            DrawRectangleLinesEx(rNotation, 2, vertEclatant);
            DrawText(notationText, (int)rNotation.x + 20, (int)rNotation.y + 10,
                     20, vertEclatant);
        } else if (appState == AppState::PLAY) {
            if (!isPaused) {
                // Zone de défi
                Rectangle rChal = {screenW / 2.0f - 175.0f, screenH * 0.25f,
                                   350.0f, 200.0f};
                DrawRectangleLinesEx(rChal, 3, vertEclatant);

                if (selectedNotation == NotationMode::STAFF && !currentChallenge.expectedNotes.empty()) {
                    drawStaff(rChal, currentChallenge.expectedNotes, vertEclatant);
                } else {
                    std::string display;
                    if (currentChallenge.expectedNotes.empty()) {
                        display = "Attente…";
                    } else if (currentChallenge.isChord) {
                        display = currentChallenge.displayText;
                    } else {
                        display = noteDisplayLabel(currentChallenge.expectedNotes[0], selectedNotation);
                    }
                    const char* chalTxt = display.c_str();
                    int txtSize = (int)display.size() > 8 ? 28 : 36;
                    DrawText(chalTxt,
                             (int)screenW / 2 - MeasureText(chalTxt, txtSize) / 2,
                             (int)rChal.y + 80, txtSize, vertEclatant);
                }

                if (feedbackAlpha > 0.0f)
                    DrawText(feedbackMsg,
                             (int)screenW / 2 -
                                 MeasureText(feedbackMsg, 40) / 2,
                             (int)(screenH * 0.2f), 40,
                             Fade(feedbackColor, feedbackAlpha));

                Rectangle btnMenu = {25.0f, 25.0f, 120.0f, 45.0f};
                (void)drawButton(btnMenu, "MENU", vertEclatant, mouse);

                if (engState == EngineState::ENG_PLAYED) {
                    Rectangle btnReady = {160.0f, 25.0f, 160.0f, 45.0f};
                    (void)drawButton(btnReady, "SUIVANT", orEclatant, mouse);
                }
            }

            DrawText(TextFormat("%i", scoreActuel), (int)screenW / 2 - 20, 45,
                     60, vertEclatant);

            Rectangle btnPause = {screenW - 85.0f, 25.0f, 60.0f, 60.0f};
            (void)drawButton(btnPause, "", vertEclatant, mouse);
            DrawRectangle((int)btnPause.x + 18, (int)btnPause.y + 18, 8, 24,
                          vertEclatant);
            DrawRectangle((int)btnPause.x + 34, (int)btnPause.y + 18, 8, 24,
                          vertEclatant);

            // Piano
            if (showKeyboard) {
                int32_t numKeys =
                    (selectedGame == GameType::GAME_NOTE) ? 7 : 14;
                int32_t numBlack =
                    (selectedGame == GameType::GAME_NOTE) ? 5 : 10;
                float wW = screenW / (float)numKeys;
                float pianoY = screenH * 0.7f;
                float pianoH = screenH - pianoY;
                float bW = wW * 0.6f;

                int hovWhite = -1;
                int hovBlack = -1;

                if (!isPaused && mouse.y > pianoY) {
                    for (int i = 0; i < numBlack; i++) {
                        Rectangle rN = {(blackKeyIndices[i] + 1) * wW -
                                            bW / 2.0f,
                                        pianoY, bW, pianoH * 0.6f};
                        if (CheckCollisionPointRec(mouse, rN)) {
                            hovBlack = i;
                            break;
                        }
                    }
                    if (hovBlack == -1) {
                        hovWhite = (int)(mouse.x / wW);
                        if (hovWhite < 0 || hovWhite >= numKeys) hovWhite = -1;
                    }
                }

                for (int i = 0; i < numKeys; i++) {
                    Rectangle r = {i * wW, pianoY, wW - 2.0f, pianoH};
                    // On recrée la note à partir de l'index i
                    int octave = 4 + (i / 7);
                    std::string noteName = std::string(1, "cdefgab"[i % 7]) +
                                           std::to_string(octave);
                    NoteKey nk = resolveKey(noteName);
                    bool isExpected = false, isCorrectKey = false,
                         isWrongKey = false;
                    if (nk.valid && !nk.isBlack) {
                        for (const auto& n : currentChallenge.expectedNotes) {
                            NoteKey ek = resolveKey(n);
                            if (ek.valid && !ek.isBlack && ek.index == nk.index)
                                isExpected = true;
                        }
                        if (lastResult.active) {
                            for (const auto& n : lastResult.correct) {
                                NoteKey ck = resolveKey(n);
                                if (ck.valid && !ck.isBlack &&
                                    ck.index == nk.index)
                                    isCorrectKey = true;
                            }
                            for (const auto& n : lastResult.incorrect) {
                                NoteKey ik = resolveKey(n);
                                if (ik.valid && !ik.isBlack &&
                                    ik.index == nk.index)
                                    isWrongKey = true;
                            }
                        }
                    }
                    Color keyColor =
                        isWrongKey            ? rougeErreur
                        : isCorrectKey        ? vertEclatant
                        : isExpected          ? orangeNote
                        : blanchesAppuyees[i] ? profiles[currentUserIdx].color
                        : (hovWhite == i)
                            ? Fade(profiles[currentUserIdx].color, 0.3f)
                            : BLANK;
                    if (keyColor.a != 0) DrawRectangleRec(r, keyColor);
                    DrawRectangleLinesEx(r, 2,
                                         isPaused ? Fade(vertEclatant, 0.2f)
                                                  : vertEclatant);
                    if (numKeys <= 7) {
                        std::string label = getNotationLabel(i, selectedNotation);
                        DrawText(label.c_str(), (int)r.x + (int)(wW / 2) - MeasureText(label.c_str(), 18) / 2,
                                 (int)r.y + (int)pianoH - 30, 18,
                                 isPaused ? Fade(vertEclatant, 0.2f)
                                          : (keyColor.a != 0 ? vertFonce
                                                             : vertEclatant));
                    }
                }
                for (int i = 0; i < numBlack; i++) {
                    Rectangle rN = {(blackKeyIndices[i] + 1) * wW - bW / 2.0f,
                                    pianoY, bW, pianoH * 0.6f};
                    static constexpr char WHITE_LETTERS[7] = {
                        'c', 'd', 'e', 'f', 'g', 'a', 'b'};
                    int octave = 4 + (blackKeyIndices[i] / 7);
                    std::string sharpNote =
                        std::string(1, WHITE_LETTERS[blackKeyIndices[i] % 7]) +
                        "#" + std::to_string(octave);
                    bool bkExpected = false, bkCorrect = false, bkWrong = false;
                    for (const auto& n : currentChallenge.expectedNotes)
                        if (noteInList(sharpNote, {n})) bkExpected = true;
                    if (lastResult.active) {
                        for (const auto& n : lastResult.correct)
                            if (noteInList(sharpNote, {n})) bkCorrect = true;
                        for (const auto& n : lastResult.incorrect)
                            if (noteInList(sharpNote, {n})) bkWrong = true;
                    }
                    Color bkFill =
                        bkWrong             ? rougeErreur
                        : bkCorrect         ? vertEclatant
                        : bkExpected        ? orangeNote
                        : noiresAppuyees[i] ? profiles[currentUserIdx].color
                        : (hovBlack == i)
                            ? Fade(profiles[currentUserIdx].color, 0.6f)
                        : isPaused ? Fade(DARKGRAY, 0.5f)
                                   : Color{30, 60, 30, 255};
                    DrawRectangleRec(rN, bkFill);
                    DrawRectangleLinesEx(rN, 2,
                                         isPaused ? Fade(vertEclatant, 0.2f)
                                                  : vertEclatant);
                }
            }

            if (isPaused) {
                DrawRectangle(0, 0, (int)screenW, (int)screenH,
                              Fade(BLACK, 0.85f));
                Rectangle mB = {screenW / 2.0f - 200.0f,
                                screenH / 2.0f - 150.0f, 400.0f, 300.0f};
                DrawRectangleLinesEx(mB, 3, vertEclatant);
                DrawText("PAUSE", (int)screenW / 2 - 60, (int)screenH / 2 - 110,
                         40, vertEclatant);
                Rectangle btnResume = {screenW / 2.0f - 150.0f,
                                       screenH / 2.0f - 50.0f, 300.0f, 60.0f};
                Rectangle btnQuit = {screenW / 2.0f - 150.0f,
                                     screenH / 2.0f + 30.0f, 300.0f, 60.0f};
                (void)drawButton(btnResume, "REPRENDRE", vertEclatant, mouse, 25);
                (void)drawButton(btnQuit, "QUITTER", rougeErreur, mouse, 25);
            }
        } else if (appState == AppState::GAME_OVER) {
            DrawText("FIN DE PARTIE",
                     (int)screenW / 2 - MeasureText("FIN DE PARTIE", 40) / 2,
                     (int)(screenH * 0.15f), 40, vertEclatant);
            DrawText(TextFormat("SCORE FINAL : %i", scoreActuel),
                     (int)screenW / 2 -
                         MeasureText(
                             TextFormat("SCORE FINAL : %i", scoreActuel), 30) /
                             2,
                     (int)(screenH * 0.3f), 30, orEclatant);
            DrawText(TextFormat("Parfaits   : %i / %i", gameStats.perfect,
                                gameStats.total),
                     (int)screenW / 2 - 130, (int)(screenH * 0.45f), 22,
                     vertEclatant);
            DrawText(TextFormat("Partiels   : %i / %i", gameStats.partial,
                                gameStats.total),
                     (int)screenW / 2 - 130, (int)(screenH * 0.5f), 22,
                     vertEclatant);
            DrawText(
                TextFormat("Durée      : %lld s", gameStats.duration / 1000),
                (int)screenW / 2 - 130, (int)(screenH * 0.55f), 22,
                vertEclatant);
            Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.7f,
                                 300.0f, 55.0f};
            (void)drawButton(btnBack, "RETOUR AU MENU", vertEclatant, mouse, 22);
        }

        EndDrawing();
    }

    Logger::log("[Main] Fermeture…");
    if (appState == AppState::PLAY) comm.send(Message("quit"));
    comm.disconnect();
    CloseWindow();
    if (enginePid > 0) {
        kill(enginePid, SIGTERM);
        waitpid(enginePid, nullptr, 0);
    }
    return 0;
}
