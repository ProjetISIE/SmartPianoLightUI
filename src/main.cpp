#include "Communication.hpp"
#include "Logger.hpp"
#include "raylib.h"
#include <csignal>
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

static constexpr int SCREEN_W{1024};
static constexpr int SCREEN_H{768};
static constexpr float CONN_RETRY_INTERVAL{2.0f}; ///< Secondes entre tentatives
static constexpr float RESULT_DISPLAY_DURATION{
    2.5f}; ///< Durée affichage résultat
static constexpr int MAX_PROFILES{4};

// ---------------------------------------------------------------------------
// Énumérations
// ---------------------------------------------------------------------------

/// États de l'application
typedef enum { PROFILE_SELECT, MENU, PLAY, GAME_OVER } AppState;

/// États du protocole (côté client)
typedef enum {
    ENG_DISCONNECTED,
    ENG_CONNECTED,
    ENG_CONFIGURED,
    ENG_PLAYING,
    ENG_PLAYED
} EngineState;

/// Gamme sélectionnée
typedef enum {
    SCALE_C,
    SCALE_D,
    SCALE_E,
    SCALE_F,
    SCALE_G,
    SCALE_A,
    SCALE_B
} ScaleChoice;

/// Mode de la gamme
typedef enum { MODE_MAJ, MODE_MIN } ModeChoice;

/// Type de jeu
typedef enum { GAME_NOTE, GAME_CHORD, GAME_INVERSED } GameType;

// ---------------------------------------------------------------------------
// Structures de données
// ---------------------------------------------------------------------------

struct UserProfile {
    std::string name;
    int topScore;
    Color color;
};

/// Défi en cours reçu du moteur
struct Challenge {
    int id{0};
    std::string displayText; ///< Texte affiché (nom note ou accord)
    std::vector<std::string> expectedNotes; ///< Notes à jouer
    bool isChord{false};
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
    int perfect{0};
    int partial{0};
    int total{0};
    long long duration{0};
};

// ---------------------------------------------------------------------------
// Fonctions utilitaires
// ---------------------------------------------------------------------------

/// Retourne le chemin du binaire moteur, dans l'ordre de priorité
static std::string findEngineBinary() {
    // 1) Même répertoire que l'exécutable en cours (installation Nix)
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
    // 2) Chemin défini à la compilation par -DENGINE_BIN_PATH=…
#ifdef ENGINE_BIN_PATH
    if (access(ENGINE_BIN_PATH, X_OK) == 0) return ENGINE_BIN_PATH;
#endif
    // 3) Chemin dans le PATH
    return "engine";
}

/// Lance le moteur en arrière-plan ; retourne son PID ou -1
static pid_t spawnEngine(const std::string& enginePath) {
    pid_t pid = fork();
    if (pid == 0) {
        // Processus enfant : remplace son image par le moteur
        execl(enginePath.c_str(), "engine", nullptr);
        // Si execl échoue, essai via PATH
        execlp("engine", "engine", nullptr);
        _exit(1);
    }
    if (pid > 0) Logger::log("[Main] Moteur démarré (PID {})", pid);
    else Logger::err("[Main] Échec fork pour lancer le moteur");
    return pid;
}

/// Découpe une chaîne de notes séparées par des espaces
static std::vector<std::string> splitNotes(const std::string& s) {
    std::vector<std::string> notes;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) notes.push_back(tok);
    return notes;
}

/// Convertit la lettre d'une note (a-g) en nom français
static std::string noteLetterToFrench(char c) {
    switch (c) {
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

/// Construit le label d'affichage pour une note (ex: "c4" → "DO 4")
static std::string noteDisplayLabel(const std::string& note) {
    if (note.empty()) return note;
    std::string label = noteLetterToFrench(note[0]);
    // Dièse ou bémol
    size_t i = 1;
    if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
        label += (note[i] == '#') ? "#" : "b";
        ++i;
    }
    // Octave
    if (i < note.size()) label += " " + note.substr(i);
    return label;
}

// ---------------------------------------------------------------------------
// Résolution d'une note vers une touche du piano affiché (7 touches blanches,
// 5 touches noires sur une octave C-B)
// ---------------------------------------------------------------------------

struct NoteKey {
    bool isBlack{false};
    int index{-1}; ///< Index touche blanche [0-7] ou noire [0-4]
    bool valid{false};
};

static NoteKey resolveKey(const std::string& note) {
    if (note.empty()) return {};
    char letter = note[0];
    size_t i = 1;
    std::string mod;
    if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
        mod = note[i];
        ++i;
    }

    // Indice de la touche blanche
    static constexpr int WHITE_MAP[7] = {
        // Indexé par (letter - 'a') : a=5(LA), b=6(SI), c=0(DO), d=1(RE),
        // e=2(MI), f=3(FA), g=4(SOL)
        5, 6, 0, 1, 2, 3, 4};
    if (letter < 'a' || letter > 'g') return {};
    int whiteIdx = WHITE_MAP[static_cast<int>(letter - 'a')];

    if (mod.empty()) return {false, whiteIdx, true};

    // Table : touche blanche → indice touche noire à sa droite
    // c(0)→bk0, d(1)→bk1, f(3)→bk2, g(4)→bk3, a(5)→bk4
    // e et b n'ont pas de dièse (ou plutôt pas de touche noire entre elles)
    static const int WHITE_TO_BLACK[7] = {0, 1, -1, 2, 3, 4, -1};

    if (mod == "#") {
        int bk = WHITE_TO_BLACK[whiteIdx];
        if (bk < 0) return {};
        return {true, bk, true};
    }
    // Bémol : touche noire à gauche
    if (mod == "b") {
        if (whiteIdx == 0) return {}; // Cb = B : pas de touche noire
        int prev = whiteIdx - 1;
        int bk = WHITE_TO_BLACK[prev];
        if (bk < 0) return {};
        return {true, bk, true};
    }
    return {};
}

// ---------------------------------------------------------------------------
// Vérifie si une note est dans la liste (pour colorier les touches)
// ---------------------------------------------------------------------------
static bool noteInList(const std::string& noteBase,
                       const std::vector<std::string>& list) {
    // Extrait lettre+modificateur (sans octave) pour comparaison souple
    auto strip = [](const std::string& n) {
        if (n.empty()) return n;
        std::string s;
        s += n[0]; // lettre
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
    // --- Argument --timeout N (ms) pour le profilage de couverture
    int timeoutMs = -1;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            timeoutMs = std::stoi(argv[i + 1]);
            ++i;
        }
    }

    Logger::init();
    Logger::log("[Main] Démarrage Smart Piano UI");

    // --- Moteur : lancement automatique puis connexion
    std::string enginePath = findEngineBinary();
    Logger::log("[Main] Chemin moteur: {}", enginePath);
    pid_t enginePid = -1;
    if (!enginePath.empty() && enginePath[0] == '/') {
        enginePid = spawnEngine(enginePath);
        usleep(500'000); // laisse le moteur démarrer et créer son socket
    } else {
        Logger::log("[Main] Moteur non trouvé localement, connexion différée");
    }

    Communication comm;

    // --- Raylib
    InitWindow(SCREEN_W, SCREEN_H, "Smart Piano");
    SetTargetFPS(60);

    // --- Couleurs
    const Color vertFonce = {20, 40, 20, 255};
    const Color vertEclatant = {100, 255, 100, 255};
    const Color orEclatant = {255, 215, 0, 255};
    const Color rougeErreur = {230, 41, 55, 255};
    const Color bleuInfo = {100, 180, 255, 255};
    const Color orangeNote = {255, 140, 0, 255};

    // --- Profils utilisateurs
    std::vector<UserProfile> profiles{{"Utilisateur", 0, SKYBLUE}};
    int currentUserIdx = 0;
    bool isNamingProfile = false;
    char inputName[11] = "\0";
    int letterCount = 0;

    // --- État application
    AppState appState = PROFILE_SELECT;
    bool isPaused = false;

    // --- Sélection de jeu
    ScaleChoice selectedScale = SCALE_C;
    ModeChoice selectedMode = MODE_MAJ;
    GameType selectedGame = GAME_NOTE;

    // --- Score et session
    int scoreActuel = 0;

    // --- Défi en cours
    Challenge currentChallenge;
    ChallengeResult lastResult;
    GameStats gameStats;

    // --- Messages d'encouragement / consolation
    const char* encouragements[] = {"MAGNIFIQUE !", "QUEL TALENT !",
                                    "PARFAIT !", "VIRTUOSE !"};
    const char* consolations[] = {"COURAGE !", "CONTINUE !", "PRESQUE !",
                                  "RYTHME !"};
    const char* feedbackMsg = "";
    Color feedbackColor = WHITE;
    float feedbackAlpha = 0.0f;

    // --- Message d'erreur moteur
    std::string errorMsg;
    float errorTimer = 0.0f;

    // --- État du protocole
    EngineState engState = ENG_DISCONNECTED;
    float connRetryTimer = 0.0f;

    // --- Piano
    const char* nomsNotes[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};
    bool blanchesAppuyees[8]{};
    bool noiresAppuyees[5]{};
    const int blackKeyIndices[] = {0, 1, 3, 4, 5};

    // --- Boutons UI
    const Rectangle btnPause = {(float)SCREEN_W - 85.0f, 25.0f, 60.0f, 60.0f};
    const Rectangle btnMenu = {25.0f, 25.0f, 120.0f, 45.0f};
    const Rectangle btnReady = {160.0f, 25.0f, 160.0f, 45.0f};
    const Rectangle btnResume = {(float)SCREEN_W / 2.0f - 150.0f,
                                 (float)SCREEN_H / 2.0f - 50.0f, 300.0f, 60.0f};
    const Rectangle btnQuit = {(float)SCREEN_W / 2.0f - 150.0f,
                               (float)SCREEN_H / 2.0f + 30.0f, 300.0f, 60.0f};

    // --- Timer pour le timeout de couverture
    float timeoutTimer = 0.0f;

    // -----------------------------------------------------------------------
    // Boucle principale
    // -----------------------------------------------------------------------
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        // Timeout de couverture
        if (timeoutMs > 0) {
            timeoutTimer += dt * 1000.0f;
            if (timeoutTimer >= (float)timeoutMs) break;
        }

        // --- Timers de feedback
        if (feedbackAlpha > 0.0f) feedbackAlpha -= dt * 0.7f;
        if (errorTimer > 0.0f) errorTimer -= dt;
        if (lastResult.active) {
            lastResult.displayTimer -= dt;
            if (lastResult.displayTimer <= 0.0f) {
                lastResult.active = false;
                lastResult.correct.clear();
                lastResult.incorrect.clear();
            }
        }

        // ===================================================================
        // Traitement des messages entrants du moteur
        // ===================================================================
        {
            auto msgOpt = comm.popMessage();
            while (msgOpt.has_value()) {
                const Message& msg = msgOpt.value();
                const std::string& type = msg.getType();

                if (type == "ack") {
                    if (msg.getField("status") == "ok") {
                        engState = ENG_CONFIGURED;
                        Logger::log("[Main] ACK ok → configuré");
                        // Envoie 'ready' immédiatement
                        comm.send(Message("ready"));
                        engState = ENG_PLAYING;
                    } else {
                        std::string code = msg.getField("code");
                        std::string text = msg.getField("message");
                        errorMsg =
                            std::format("Config invalide ({}): {}", code, text);
                        errorTimer = 5.0f;
                        appState = MENU;
                        Logger::err("[Main] ACK erreur: {} - {}", code, text);
                    }
                } else if (type == "note") {
                    std::string noteName = msg.getField("note");
                    currentChallenge.id =
                        msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
                    currentChallenge.displayText = noteDisplayLabel(noteName);
                    currentChallenge.expectedNotes = {noteName};
                    currentChallenge.isChord = false;
                    engState = ENG_PLAYING;
                    Logger::log("[Main] Défi note: {}", noteName);
                } else if (type == "chord") {
                    currentChallenge.id =
                        msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
                    currentChallenge.displayText = msg.getField("name");
                    currentChallenge.expectedNotes =
                        splitNotes(msg.getField("notes"));
                    currentChallenge.isChord = true;
                    engState = ENG_PLAYING;
                    Logger::log("[Main] Défi accord: {}",
                                currentChallenge.displayText);
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
                        feedbackAlpha = 1.0f;
                        scoreActuel += 10;
                    } else if (isPartial) {
                        feedbackMsg = consolations[GetRandomValue(0, 3)];
                        feedbackColor = orangeNote;
                        feedbackAlpha = 1.0f;
                        scoreActuel += 5;
                    } else {
                        feedbackMsg = consolations[GetRandomValue(0, 3)];
                        feedbackColor = rougeErreur;
                        feedbackAlpha = 1.0f;
                    }
                    engState = ENG_PLAYED;
                    Logger::log(
                        "[Main] Résultat reçu (correct: {}, incorrect: {})",
                        msg.getField("correct"), msg.getField("incorrect"));
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
                    // Mise à jour record
                    if (scoreActuel > profiles[currentUserIdx].topScore)
                        profiles[currentUserIdx].topScore = scoreActuel;
                    engState = ENG_CONNECTED;
                    appState = GAME_OVER;
                    Logger::log("[Main] Partie terminée: parfait={}, total={}",
                                gameStats.perfect, gameStats.total);
                } else if (type == "error") {
                    std::string code = msg.getField("code");
                    std::string text = msg.getField("message");
                    errorMsg =
                        std::format("Erreur moteur ({}): {}", code, text);
                    errorTimer = 5.0f;
                    Logger::err("[Main] Erreur moteur: {} - {}", code, text);
                    // Erreur interne → retour au menu
                    if (code == "internal") {
                        engState = ENG_CONNECTED;
                        appState = MENU;
                    }
                }
                msgOpt = comm.popMessage();
            }
        }

        // Détection déconnexion
        if (engState != ENG_DISCONNECTED && !comm.isConnected()) {
            Logger::log("[Main] Connexion perdue");
            engState = ENG_DISCONNECTED;
            if (appState == PLAY || appState == GAME_OVER) appState = MENU;
        }

        // Tentative de (re)connexion périodique
        if (engState == ENG_DISCONNECTED) {
            connRetryTimer -= dt;
            if (connRetryTimer <= 0.0f) {
                if (comm.connect()) {
                    engState = ENG_CONNECTED;
                    Logger::log("[Main] Connecté au moteur");
                }
                connRetryTimer = CONN_RETRY_INTERVAL;
            }
        }

        // ===================================================================
        // Logique par état
        // ===================================================================

        // --- PROFIL SÉLECTION
        if (appState == PROFILE_SELECT) {
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
            } else if (clicked) {
                for (int i = 0; i < (int)profiles.size(); i++) {
                    Rectangle r = {(float)(100 + i * 220), 300.0f, 180.0f,
                                   220.0f};
                    Rectangle rDel = {r.x + 150.0f, r.y + 5.0f, 25.0f, 25.0f};
                    if (CheckCollisionPointRec(mouse, rDel)) {
                        profiles.erase(profiles.begin() + i);
                        break;
                    }
                    if (CheckCollisionPointRec(mouse, r)) {
                        currentUserIdx = i;
                        scoreActuel = 0;
                        appState = MENU;
                        break;
                    }
                }
                if ((int)profiles.size() < MAX_PROFILES) {
                    Rectangle btnPlus = {
                        (float)(100 + (int)profiles.size() * 220), 300, 180,
                        220};
                    if (!isNamingProfile &&
                        CheckCollisionPointRec(mouse, btnPlus))
                        isNamingProfile = true;
                }
            }
        }

        // --- MENU
        else if (appState == MENU) {
            if (clicked) {
                // Sélection gamme
                for (int i = 0; i < 7; i++) {
                    Rectangle r = {(float)(SCREEN_W / 2 - 245 + i * 70), 560.0f,
                                   60.0f, 40.0f};
                    if (CheckCollisionPointRec(mouse, r))
                        selectedScale = static_cast<ScaleChoice>(i);
                }
                // Sélection mode
                {
                    Rectangle rMaj = {(float)SCREEN_W / 2.0f - 75.0f, 620.0f,
                                      70.0f, 40.0f};
                    Rectangle rMin = {(float)SCREEN_W / 2.0f + 5.0f, 620.0f,
                                      70.0f, 40.0f};
                    if (CheckCollisionPointRec(mouse, rMaj))
                        selectedMode = MODE_MAJ;
                    if (CheckCollisionPointRec(mouse, rMin))
                        selectedMode = MODE_MIN;
                }
                // Boutons de jeu
                auto startGame = [&](GameType gt) {
                    if (engState == ENG_DISCONNECTED) {
                        errorMsg = "Moteur non connecté. Démarrage en cours…";
                        errorTimer = 3.0f;
                        return;
                    }
                    selectedGame = gt;
                    scoreActuel = 0;
                    currentChallenge = Challenge{};
                    lastResult = ChallengeResult{};
                    feedbackAlpha = 0.0f;
                    appState = PLAY;

                    const char* scaleStr[] = {"c", "d", "e", "f",
                                              "g", "a", "b"};
                    Message cfg(
                        "config",
                        {{"game", gt == GAME_NOTE    ? "note"
                                  : gt == GAME_CHORD ? "chord"
                                                     : "inversed"},
                         {"scale", scaleStr[static_cast<int>(selectedScale)]},
                         {"mode", selectedMode == MODE_MAJ ? "maj" : "min"}});
                    comm.send(cfg);
                    engState = ENG_CONFIGURED;
                };
                if (mouse.x > SCREEN_W / 2 - 250 &&
                    mouse.x < SCREEN_W / 2 + 250) {
                    if (mouse.y > 250 && mouse.y < 330) startGame(GAME_NOTE);
                    else if (mouse.y > 350 && mouse.y < 430)
                        startGame(GAME_CHORD);
                    else if (mouse.y > 450 && mouse.y < 530)
                        startGame(GAME_INVERSED);
                }
                if (mouse.y > 680) appState = PROFILE_SELECT;
            }
        }

        // --- JEU
        else if (appState == PLAY) {
            if (clicked) {
                if (CheckCollisionPointRec(mouse, btnPause))
                    isPaused = !isPaused;
                else if (isPaused) {
                    if (CheckCollisionPointRec(mouse, btnResume))
                        isPaused = false;
                    if (CheckCollisionPointRec(mouse, btnQuit)) {
                        comm.send(Message("quit"));
                        engState = ENG_CONNECTED;
                        appState = MENU;
                        isPaused = false;
                    }
                } else {
                    if (CheckCollisionPointRec(mouse, btnMenu)) {
                        comm.send(Message("quit"));
                        engState = ENG_CONNECTED;
                        appState = MENU;
                    }
                    // Bouton "SUIVANT" / PRÊT (seulement après résultat)
                    if (engState == ENG_PLAYED &&
                        CheckCollisionPointRec(mouse, btnReady)) {
                        comm.send(Message("ready"));
                        engState = ENG_PLAYING;
                        lastResult.active = false;
                        lastResult.correct.clear();
                        lastResult.incorrect.clear();
                    }
                }
            }

            // Touches piano (visuel seulement)
            if (!isPaused) {
                for (auto& b : blanchesAppuyees) b = false;
                for (auto& b : noiresAppuyees) b = false;
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse.y > 550) {
                    float wW = (float)SCREEN_W / 8;
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

        // --- FIN DE PARTIE
        else if (appState == GAME_OVER) {
            if (clicked) {
                Rectangle btnBack = {(float)SCREEN_W / 2 - 150,
                                     (float)SCREEN_H / 2 + 120, 300, 55};
                if (CheckCollisionPointRec(mouse, btnBack)) {
                    appState = MENU;
                    currentChallenge = Challenge{};
                    lastResult = ChallengeResult{};
                }
            }
        }

        // ===================================================================
        // Rendu
        // ===================================================================
        BeginDrawing();
        ClearBackground(vertFonce);

        // --- Indicateur connexion (coin haut gauche, sauf PROFILE_SELECT)
        if (appState != PROFILE_SELECT) {
            Color indColor = (engState == ENG_DISCONNECTED) ? rougeErreur
                             : (engState == ENG_CONNECTED)  ? orEclatant
                                                            : vertEclatant;
            DrawCircle(12, 12, 6, indColor);
            if (engState == ENG_DISCONNECTED)
                DrawText("Connexion…", 24, 5, 14, rougeErreur);
        }

        // --- Message d'erreur moteur
        if (errorTimer > 0.0f) {
            float alpha = (errorTimer < 1.0f) ? errorTimer : 1.0f;
            DrawRectangle(0, SCREEN_H - 50, SCREEN_W, 50,
                          Fade(rougeErreur, alpha * 0.85f));
            DrawText(errorMsg.c_str(),
                     SCREEN_W / 2 - MeasureText(errorMsg.c_str(), 18) / 2,
                     SCREEN_H - 36, 18, Fade(WHITE, alpha));
        }

        // -------------------------------------------------------------------
        if (appState == PROFILE_SELECT) {
            DrawText("SESSIONS UTILISATEURS",
                     SCREEN_W / 2 -
                         MeasureText("SESSIONS UTILISATEURS", 30) / 2,
                     100, 30, vertEclatant);
            for (int i = 0; i < (int)profiles.size(); i++) {
                Rectangle r = {(float)(100 + i * 220), 300, 180, 220};
                Rectangle rDel = {r.x + 150, r.y + 5, 25, 25};
                DrawRectangleLinesEx(r, 2, profiles[i].color);
                DrawText(profiles[i].name.c_str(), (int)r.x + 15,
                         (int)r.y + 100, 20, profiles[i].color);
                DrawText(TextFormat("Record: %i", profiles[i].topScore),
                         (int)r.x + 15, (int)r.y + 140, 15,
                         Fade(profiles[i].color, 0.6f));
                DrawRectangle((int)rDel.x, (int)rDel.y, 25, 25, rougeErreur);
                DrawText("X", (int)rDel.x + 7, (int)rDel.y + 4, 20, WHITE);
            }
            if ((int)profiles.size() < MAX_PROFILES && !isNamingProfile) {
                Rectangle rP = {(float)(100 + (int)profiles.size() * 220), 300,
                                180, 220};
                DrawRectangleLinesEx(rP, 2, Fade(vertEclatant, 0.3f));
                DrawText("+", (int)rP.x + 75, (int)rP.y + 80, 60,
                         Fade(vertEclatant, 0.3f));
            }
            if (isNamingProfile) {
                DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.8f));
                DrawText("NOM DU NOUVEAU PROFIL :", SCREEN_W / 2 - 140,
                         SCREEN_H / 2 - 50, 20, vertEclatant);
                DrawText(inputName,
                         SCREEN_W / 2 - MeasureText(inputName, 40) / 2,
                         SCREEN_H / 2, 40, WHITE);
                DrawText("ENTREE pour valider", SCREEN_W / 2 - 80,
                         SCREEN_H / 2 + 60, 15, GRAY);
            }
        }

        // -------------------------------------------------------------------
        else if (appState == MENU) {
            DrawText(
                TextFormat("JOUEUR: %s", profiles[currentUserIdx].name.c_str()),
                40, 40, 25, profiles[currentUserIdx].color);
            DrawText(
                TextFormat("RECORD: %i", profiles[currentUserIdx].topScore),
                750, 40, 25, vertEclatant);
            DrawText("SELECTIONNER UN JEU",
                     SCREEN_W / 2 - MeasureText("SELECTIONNER UN JEU", 30) / 2,
                     150, 30, vertEclatant);

            // Boutons jeu
            const char* gameTitles[] = {"JEU DE NOTES", "JEU D'ACCORDS",
                                        "JEU D'ACCORDS RENVERSES"};
            for (int i = 0; i < 3; i++) {
                DrawRectangleLines(SCREEN_W / 2 - 250, 250 + i * 100, 500, 80,
                                   vertEclatant);
                DrawText(gameTitles[i],
                         SCREEN_W / 2 - MeasureText(gameTitles[i], 25) / 2,
                         275 + i * 100, 25, vertEclatant);
            }

            // Sélection gamme
            DrawText("GAMME :", SCREEN_W / 2 - 245, 540, 18, GRAY);
            const char* scaleLabels[] = {"C", "D", "E", "F", "G", "A", "B"};
            for (int i = 0; i < 7; i++) {
                Rectangle r = {(float)(SCREEN_W / 2 - 245 + i * 70), 560, 60,
                               40};
                bool sel = (selectedScale == static_cast<ScaleChoice>(i));
                DrawRectangleLinesEx(
                    r, 2, sel ? vertEclatant : Fade(vertEclatant, 0.35f));
                if (sel)
                    DrawRectangle((int)r.x + 1, (int)r.y + 1, 58, 38,
                                  Fade(vertEclatant, 0.15f));
                DrawText(scaleLabels[i],
                         (int)r.x + 30 - MeasureText(scaleLabels[i], 20) / 2,
                         (int)r.y + 10, 20,
                         sel ? vertEclatant : Fade(vertEclatant, 0.35f));
            }

            // Sélection mode
            DrawText("MODE :", SCREEN_W / 2 - 75, 600, 18, GRAY);
            {
                const char* modes[] = {"MAJ", "MIN"};
                int offsets[] = {0, 80};
                for (int i = 0; i < 2; i++) {
                    Rectangle r = {(float)(SCREEN_W / 2 - 75 + offsets[i]), 620,
                                   70, 40};
                    bool sel = ((i == 0) == (selectedMode == MODE_MAJ));
                    DrawRectangleLinesEx(
                        r, 2, sel ? vertEclatant : Fade(vertEclatant, 0.35f));
                    if (sel)
                        DrawRectangle((int)r.x + 1, (int)r.y + 1, 68, 38,
                                      Fade(vertEclatant, 0.15f));
                    DrawText(modes[i],
                             (int)r.x + 35 - MeasureText(modes[i], 20) / 2,
                             (int)r.y + 10, 20,
                             sel ? vertEclatant : Fade(vertEclatant, 0.35f));
                }
            }

            DrawText("< CHANGER D'UTILISATEUR",
                     SCREEN_W / 2 -
                         MeasureText("< CHANGER D'UTILISATEUR", 15) / 2,
                     690, 15, GRAY);
        }

        // -------------------------------------------------------------------
        else if (appState == PLAY) {
            if (!isPaused) {
                // Zone de défi
                const char* chalTxt =
                    currentChallenge.displayText.empty()
                        ? "Attente…"
                        : currentChallenge.displayText.c_str();
                Rectangle rChal = {(float)SCREEN_W / 2 - 175,
                                   (float)SCREEN_H / 2 - 160, 350, 200};
                DrawRectangleLinesEx(rChal, 3, vertEclatant);

                // Étiquette type de jeu
                const char* gameLbl = selectedGame == GAME_NOTE ? "NOTE"
                                      : selectedGame == GAME_CHORD
                                          ? "ACCORD"
                                          : "RENVERSEMENT";
                DrawText(gameLbl, (int)rChal.x + 10, (int)rChal.y + 10, 14,
                         Fade(vertEclatant, 0.6f));

                // Texte du défi (note ou accord)
                int txtSize =
                    (int)currentChallenge.displayText.size() > 8 ? 28 : 36;
                DrawText(chalTxt,
                         SCREEN_W / 2 - MeasureText(chalTxt, txtSize) / 2,
                         SCREEN_H / 2 - 90, txtSize, vertEclatant);

                // État connexion moteur
                if (engState == ENG_PLAYING) {
                    DrawText("JOUEZ…",
                             (int)rChal.x + (int)rChal.width / 2 -
                                 MeasureText("JOUEZ…", 18) / 2,
                             (int)rChal.y + (int)rChal.height - 30, 18,
                             Fade(bleuInfo, 0.7f));
                }

                // Feedback
                if (feedbackAlpha > 0.0f)
                    DrawText(feedbackMsg,
                             SCREEN_W / 2 - MeasureText(feedbackMsg, 40) / 2,
                             180, 40, Fade(feedbackColor, feedbackAlpha));

                // Boutons haut
                DrawRectangleLinesEx(btnMenu, 2, vertEclatant);
                DrawText("MENU", 55, 37, 18, vertEclatant);

                // Bouton SUIVANT/PRÊT (visible seulement après résultat)
                if (engState == ENG_PLAYED) {
                    DrawRectangleLinesEx(btnReady, 2, orEclatant);
                    DrawText("SUIVANT", (int)btnReady.x + 35,
                             (int)btnReady.y + 13, 18, orEclatant);
                }
            }

            // Score
            DrawText(TextFormat("%i", scoreActuel), SCREEN_W / 2 - 20, 45, 60,
                     vertEclatant);
            DrawText(TextFormat("TOP: %i", profiles[currentUserIdx].topScore),
                     SCREEN_W / 2 - 35, 15, 20,
                     Fade(profiles[currentUserIdx].color, 0.8f));

            // Bouton pause
            DrawRectangleLinesEx(btnPause, 2, vertEclatant);
            DrawRectangle((int)btnPause.x + 18, (int)btnPause.y + 18, 8, 24,
                          vertEclatant);
            DrawRectangle((int)btnPause.x + 34, (int)btnPause.y + 18, 8, 24,
                          vertEclatant);

            // ----- Piano -----
            float wW = (float)SCREEN_W / 8;
            for (int i = 0; i < 8; i++) {
                Rectangle r = {i * wW, 550, wW - 2, 218};

                // Couleur de base / mise en évidence
                NoteKey nk =
                    resolveKey(i < 7 ? std::string(1, "cdefgab"[i]) : "c");

                bool isExpected = false;
                bool isCorrectKey = false;
                bool isWrongKey = false;
                if (nk.valid && !nk.isBlack) {
                    for (const auto& n : currentChallenge.expectedNotes) {
                        NoteKey ek = resolveKey(n);
                        if (ek.valid && !ek.isBlack && ek.index == nk.index)
                            isExpected = true;
                    }
                    if (lastResult.active) {
                        for (const auto& n : lastResult.correct) {
                            NoteKey ck = resolveKey(n);
                            if (ck.valid && !ck.isBlack && ck.index == nk.index)
                                isCorrectKey = true;
                        }
                        for (const auto& n : lastResult.incorrect) {
                            NoteKey ik = resolveKey(n);
                            if (ik.valid && !ik.isBlack && ik.index == nk.index)
                                isWrongKey = true;
                        }
                    }
                }

                Color keyColor;
                if (isWrongKey) keyColor = rougeErreur;
                else if (isCorrectKey) keyColor = vertEclatant;
                else if (isExpected) keyColor = orangeNote;
                else if (blanchesAppuyees[i])
                    keyColor = profiles[currentUserIdx].color;
                else keyColor = BLANK; // transparent → juste le contour

                if (isWrongKey || isCorrectKey || isExpected ||
                    blanchesAppuyees[i])
                    DrawRectangleRec(r, keyColor);
                else
                    DrawRectangleLinesEx(r, 2,
                                         isPaused ? Fade(vertEclatant, 0.2f)
                                                  : vertEclatant);

                DrawText(nomsNotes[i], (int)r.x + (int)(wW / 2) - 15,
                         (int)r.y + 185, 18,
                         isPaused ? Fade(vertEclatant, 0.2f)
                         : (isExpected || isCorrectKey || isWrongKey ||
                            blanchesAppuyees[i])
                             ? vertFonce
                             : vertEclatant);
            }

            // Touches noires
            float bW = wW * 0.6f;
            for (int i = 0; i < 5; i++) {
                Rectangle rN = {(blackKeyIndices[i] + 1) * wW - bW / 2, 550, bW,
                                130};

                // Résolution de la note associée à cette touche noire
                // blackKeyIndices[i] correspond à la touche blanche à sa gauche
                static constexpr char WHITE_LETTERS[7] = {'c', 'd', 'e', 'f',
                                                          'g', 'a', 'b'};
                std::string sharpNote =
                    std::string(1, WHITE_LETTERS[blackKeyIndices[i]]) + "#";

                bool bkExpected = false;
                bool bkCorrect = false;
                bool bkWrong = false;

                for (const auto& n : currentChallenge.expectedNotes)
                    if (noteInList(sharpNote, {n})) bkExpected = true;
                if (lastResult.active) {
                    for (const auto& n : lastResult.correct)
                        if (noteInList(sharpNote, {n})) bkCorrect = true;
                    for (const auto& n : lastResult.incorrect)
                        if (noteInList(sharpNote, {n})) bkWrong = true;
                }

                Color bkFill = (bkWrong)      ? rougeErreur
                               : (bkCorrect)  ? vertEclatant
                               : (bkExpected) ? orangeNote
                               : (noiresAppuyees[i])
                                   ? profiles[currentUserIdx].color
                               : isPaused ? Fade(vertFonce, 0.5f)
                                          : Color{30, 60, 30, 255};
                DrawRectangleRec(rN, bkFill);
                DrawRectangleLinesEx(
                    rN, 2, isPaused ? Fade(vertEclatant, 0.2f) : vertEclatant);
            }

            // Overlay pause
            if (isPaused) {
                DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(vertFonce, 0.85f));
                Rectangle mB = {SCREEN_W / 2 - 200, SCREEN_H / 2 - 150, 400,
                                300};
                DrawRectangleRec(mB, vertFonce);
                DrawRectangleLinesEx(mB, 3, vertEclatant);
                DrawText("PAUSE", SCREEN_W / 2 - 60, SCREEN_H / 2 - 110, 40,
                         vertEclatant);
                DrawRectangleLinesEx(btnResume, 2, vertEclatant);
                DrawText("REPRENDRE", (int)btnResume.x + 80,
                         (int)btnResume.y + 18, 25, vertEclatant);
                DrawRectangleLinesEx(btnQuit, 2, rougeErreur);
                DrawText("QUITTER", (int)btnQuit.x + 95, (int)btnQuit.y + 18,
                         25, rougeErreur);
            }
        }

        // -------------------------------------------------------------------
        else if (appState == GAME_OVER) {
            DrawText("FIN DE PARTIE",
                     SCREEN_W / 2 - MeasureText("FIN DE PARTIE", 40) / 2, 80,
                     40, vertEclatant);

            int cx = SCREEN_W / 2;
            DrawText(TextFormat("SCORE FINAL : %i", scoreActuel),
                     cx - MeasureText(
                              TextFormat("SCORE FINAL : %i", scoreActuel), 30) /
                              2,
                     180, 30, orEclatant);
            DrawText(TextFormat("Parfaits   : %i / %i", gameStats.perfect,
                                gameStats.total),
                     cx - 130, 260, 22, vertEclatant);
            DrawText(TextFormat("Partiels   : %i / %i", gameStats.partial,
                                gameStats.total),
                     cx - 130, 295, 22, vertEclatant);
            long long secs = gameStats.duration / 1000;
            DrawText(TextFormat("Durée      : %lld s", secs), cx - 130, 330, 22,
                     vertEclatant);
            DrawText(TextFormat("Record     : %i",
                                profiles[currentUserIdx].topScore),
                     cx - 130, 365, 22, orEclatant);

            Rectangle btnBack = {(float)cx - 150, (float)SCREEN_H / 2 + 120,
                                 300, 55};
            DrawRectangleLinesEx(btnBack, 2, vertEclatant);
            DrawText("RETOUR AU MENU",
                     cx - MeasureText("RETOUR AU MENU", 22) / 2,
                     (int)btnBack.y + 16, 22, vertEclatant);
        }

        EndDrawing();
    }

    // -----------------------------------------------------------------------
    // Nettoyage
    // -----------------------------------------------------------------------
    Logger::log("[Main] Fermeture…");
    if (appState == PLAY) comm.send(Message("quit"));
    comm.disconnect();
    CloseWindow();

    if (enginePid > 0) {
        kill(enginePid, SIGTERM);
        waitpid(enginePid, nullptr, 0);
    }
    Logger::log("[Main] Arrêté proprement");
    return 0;
}
