#include "AppController.hpp"
#include "Logger.hpp"
#include "MusicUtils.hpp"
#include "UI.hpp"
#include <GLFW/glfw3.h>
#include <csignal>
#include <cstring>
#include <format>
#include <sys/wait.h>

namespace {
constexpr float kConnRetryInterval{2.0f};     ///< Secondes entre tentatives
constexpr float kResultDisplayDuration{2.5f}; ///< Durée affichage résultat
} // namespace

AppController::AppController() : comm_() {}

AppController::~AppController() { this->cleanup(); }

void AppController::init(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            timeoutMs_ = std::stoi(argv[i + 1]);
            ++i;
        } else if (std::strcmp(argv[i], "--verbose") == 0 ||
                   std::strcmp(argv[i], "-v") == 0) {
            verbose_ = true;
        } else if (std::strcmp(argv[i], "--fullscreen") == 0 ||
                   std::strcmp(argv[i], "-f") == 0) {
            fullscreen_ = true;
        }
    }

    Logger::init();
    Logger::setVerbose(verbose_);
    Logger::log("[App] Initialisation de l'application");

    std::string enginePath = findEngineBinary();
    Logger::log("[App] Chemin moteur trouvé : {}", enginePath);
    if (!enginePath.empty() && enginePath[0] == '/') {
        enginePid_ = spawnEngine(enginePath);
        usleep(500'000); // 500 ms wait for daemon startup
    }
}

void AppController::run() {
    // Configuration Raylib & GLFW hints
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    InitWindow(1024, 768, "Smart Piano");

    if (fullscreen_) {
        ToggleFullscreen();
    } else {
        MaximizeWindow();
    }

    if (!IsWindowReady()) {
        Logger::err("[App] Échec initialisation fenêtre Raylib");
        return;
    }
    SetTargetFPS(60);

    // Initialisation session par défaut
    profiles_ = {{"Utilisateur", 0, SKYBLUE}};
    currentUserIdx_ = 0;

    Logger::log("[App] Boucle principale lancée");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        Vector2 dpiScale = GetWindowScaleDPI();
        mouse.x *= dpiScale.x;
        mouse.y *= dpiScale.y;

        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        float screenW = static_cast<float>(GetScreenWidth());
        float screenH = static_cast<float>(GetScreenHeight());

        // Gestion timeout de sécurité
        if (timeoutMs_ > 0) {
            timeoutTimer_ += dt * 1000.0f;
            if (timeoutTimer_ >= static_cast<float>(timeoutMs_)) {
                Logger::log("[App] Timeout de sécurité expiré. Arrêt.");
                break;
            }
        }

        // Décrémentation des minuteries
        if (feedbackAlpha_ > 0.0f) {
            feedbackAlpha_ -= dt * 0.7f;
        }
        if (errorTimer_ > 0.0f) {
            errorTimer_ -= dt;
        }
        if (lastResult_.active) {
            lastResult_.displayTimer -= dt;
            if (lastResult_.displayTimer <= 0.0f) {
                lastResult_.active = false;
                lastResult_.correct.clear();
                lastResult_.incorrect.clear();
                if (engState_ == EngineState::ENG_PLAYED) {
                    comm_.send(Message("ready"));
                    engState_ = EngineState::ENG_PLAYING;
                }
            }
        }

        // Process socket messages
        processIncomingMessages();

        // Gestion déconnexion inattendue
        if (engState_ != EngineState::ENG_DISCONNECTED &&
            !comm_.isConnected()) {
            engState_ = EngineState::ENG_DISCONNECTED;
            availableGames_.clear();
            if (appState_ == AppState::PLAY ||
                appState_ == AppState::GAME_OVER) {
                appState_ = AppState::MENU;
            }
        }

        // Tentatives périodiques de reconnexion
        if (engState_ == EngineState::ENG_DISCONNECTED) {
            connRetryTimer_ -= dt;
            if (connRetryTimer_ <= 0.0f) {
                if (comm_.connect()) {
                    engState_ = EngineState::ENG_CONNECTED;
                }
                connRetryTimer_ = kConnRetryInterval;
            }
        }

        // Logique de l'application
        updateLogic(dt, mouse, clicked, screenW, screenH);

        // Rendu graphique
        BeginDrawing();
        ClearBackground(BLACK);
        UI::draw(*this, mouse, screenW, screenH);
        EndDrawing();
    }
}

void AppController::cleanup() {
    Logger::log("[App] Nettoyage et fermeture…");
    if (appState_ == AppState::PLAY) {
        comm_.send(Message("quit"));
    }
    comm_.disconnect();
    availableGames_.clear();

    // Check if window was initialized to avoid crash on close
    if (IsWindowReady()) {
        CloseWindow();
    }

    if (enginePid_ > 0) {
        kill(enginePid_, SIGTERM);
        waitpid(enginePid_, nullptr, 0);
        enginePid_ = -1;
    }
}

void AppController::processIncomingMessages() {
    auto msgOpt = comm_.popMessage();
    while (msgOpt.has_value()) {
        const Message& msg = msgOpt.value();
        const std::string& type = msg.getType();

        if (type == "ack") {
            if (msg.getField("status") == "ok") {
                engState_ = EngineState::ENG_CONFIGURED;
                comm_.send(Message("ready"));
                engState_ = EngineState::ENG_PLAYING;
            } else {
                errorMsg_ = std::format("Config invalide : {}",
                                        msg.getField("message"));
                errorTimer_ = 5.0f;
                appState_ = AppState::MENU;
            }
        } else if (type == "gametype") {
            availableGames_.push_back(
                {msg.getField("id"), msg.getField("name"),
                 msg.hasField("keys") ? std::stoi(msg.getField("keys")) : 7});
        } else if (type == "note") {
            currentChallenge_.id =
                msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
            std::string noteName = msg.getField("note");
            currentChallenge_.rawName = noteName;
            currentChallenge_.displayText =
                MusicUtils::noteDisplayLabel(noteName, selectedNotation_);
            currentChallenge_.expectedNotes = {noteName};
            currentChallenge_.isChord = false;
            engState_ = EngineState::ENG_PLAYING;
        } else if (type == "chord") {
            currentChallenge_.id =
                msg.hasField("id") ? std::stoi(msg.getField("id")) : 0;
            currentChallenge_.rawName = msg.getField("name");
            currentChallenge_.displayText = MusicUtils::chordDisplayLabel(
                currentChallenge_.rawName, selectedNotation_);
            currentChallenge_.expectedNotes =
                MusicUtils::splitNotes(msg.getField("notes"));
            currentChallenge_.isChord = true;
            engState_ = EngineState::ENG_PLAYING;
        } else if (type == "result") {
            lastResult_.correct =
                MusicUtils::splitNotes(msg.getField("correct"));
            lastResult_.incorrect =
                MusicUtils::splitNotes(msg.getField("incorrect"));
            lastResult_.displayTimer = kResultDisplayDuration;
            lastResult_.active = true;

            bool isCorrect =
                lastResult_.incorrect.empty() && !lastResult_.correct.empty();
            bool isPartial =
                !lastResult_.incorrect.empty() && !lastResult_.correct.empty();

            static const char* encouragements[] = {
                "MAGNIFIQUE !", "QUEL TALENT !", "PARFAIT !", "VIRTUOSE !"};
            static const char* consolations[] = {"COURAGE !", "CONTINUE !",
                                                 "PRESQUE !", "RYTHME !"};

            if (isCorrect) {
                feedbackMsg_ = encouragements[GetRandomValue(0, 3)];
                feedbackColor_ = Colors::kOrEclatant;
                scoreActuel_ += 10;
            } else if (isPartial) {
                feedbackMsg_ = consolations[GetRandomValue(0, 3)];
                feedbackColor_ = Colors::kOrangeNote;
                scoreActuel_ += 5;
            } else {
                feedbackMsg_ = consolations[GetRandomValue(0, 3)];
                feedbackColor_ = Colors::kRougeErreur;
            }
            feedbackAlpha_ = 1.0f;
            engState_ = EngineState::ENG_PLAYED;
        } else if (type == "over") {
            gameStats_.perfect = msg.hasField("perfect")
                                     ? std::stoi(msg.getField("perfect"))
                                     : 0;
            gameStats_.partial = msg.hasField("partial")
                                     ? std::stoi(msg.getField("partial"))
                                     : 0;
            gameStats_.total =
                msg.hasField("total") ? std::stoi(msg.getField("total")) : 0;
            gameStats_.duration = msg.hasField("duration")
                                      ? std::stoll(msg.getField("duration"))
                                      : 0LL;

            if (scoreActuel_ > profiles_[currentUserIdx_].topScore) {
                profiles_[currentUserIdx_].topScore = scoreActuel_;
            }
            engState_ = EngineState::ENG_CONNECTED;
            appState_ = AppState::GAME_OVER;
        } else if (type == "error") {
            errorMsg_ = std::format("Erreur : {}", msg.getField("message"));
            errorTimer_ = 5.0f;
            if (msg.getField("code") == "internal") {
                engState_ = EngineState::ENG_CONNECTED;
                appState_ = AppState::MENU;
            }
        }
        msgOpt = comm_.popMessage();
    }
}

void AppController::updateLogic(float /*dt*/, Vector2 mouse, bool clicked,
                                float screenW, float screenH) {
    if (appState_ == AppState::PROFILE_SELECT) {
        if (isNamingProfile_) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 125 && inputName_.length() < 10) {
                    inputName_ += static_cast<char>(key);
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && !inputName_.empty()) {
                inputName_.pop_back();
            }
            if (IsKeyPressed(KEY_ENTER) && !inputName_.empty()) {
                Color c = {static_cast<unsigned char>(GetRandomValue(100, 255)),
                           static_cast<unsigned char>(GetRandomValue(100, 255)),
                           static_cast<unsigned char>(GetRandomValue(100, 255)),
                           255};
                profiles_.push_back({inputName_, 0, c});
                isNamingProfile_ = false;
                inputName_.clear();
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                isNamingProfile_ = false;
                inputName_.clear();
            }
        } else if (clicked) {
            float totalW = profiles_.size() * 200.0f +
                           ((profiles_.size() < 4) ? 200.0f : 0.0f) - 20.0f;
            float startX = screenW / 2.0f - totalW / 2.0f;

            for (size_t i = 0; i < profiles_.size(); i++) {
                Rectangle r = {startX + i * 200.0f, screenH / 2.0f - 110.0f,
                               180.0f, 220.0f};
                Rectangle rDel = {r.x + 150.0f, r.y + 5.0f, 25.0f, 25.0f};

                if (CheckCollisionPointRec(mouse, rDel)) {
                    profiles_.erase(profiles_.begin() + i);
                    break;
                }
                if (CheckCollisionPointRec(mouse, r)) {
                    currentUserIdx_ = static_cast<int32_t>(i);
                    scoreActuel_ = 0;
                    appState_ = AppState::MENU;
                    break;
                }
            }

            if (profiles_.size() < 4) {
                Rectangle btnPlus = {startX + profiles_.size() * 200.0f,
                                     screenH / 2.0f - 110.0f, 180.0f, 220.0f};
                if (CheckCollisionPointRec(mouse, btnPlus)) {
                    isNamingProfile_ = true;
                }
            }
        }
    } else if (appState_ == AppState::MENU) {
        if (clicked) {
            // Toggle clavier
            Rectangle rKbd = {40, screenH - 80, 220, 40};
            if (CheckCollisionPointRec(mouse, rKbd)) {
                showKeyboard_ = !showKeyboard_;
            }

            // Sélection de gamme
            float scaleStartX = screenW / 2.0f - (7.0f * 70.0f) / 2.0f;
            for (int i = 0; i < 7; i++) {
                Rectangle r = {scaleStartX + i * 70.0f, screenH * 0.75f, 60.0f,
                               40.0f};
                if (CheckCollisionPointRec(mouse, r)) {
                    selectedScale_ = static_cast<ScaleChoice>(i);
                }
            }

            // Sélection de mode
            Rectangle rMaj = {screenW / 2.0f - 75.0f, screenH * 0.85f, 70.0f,
                              40.0f};
            Rectangle rMin = {screenW / 2.0f + 5.0f, screenH * 0.85f, 70.0f,
                              40.0f};
            if (CheckCollisionPointRec(mouse, rMaj)) {
                selectedMode_ = ModeChoice::MODE_MAJ;
            }
            if (CheckCollisionPointRec(mouse, rMin)) {
                selectedMode_ = ModeChoice::MODE_MIN;
            }

            // Clics sur les modes de jeu
            for (size_t i = 0; i < availableGames_.size(); i++) {
                Rectangle r = {screenW / 2.0f - 250.0f,
                               screenH * 0.3f + i * 100.0f, 500.0f, 80.0f};
                if (CheckCollisionPointRec(mouse, r)) {
                    startGame(availableGames_[i].id);
                }
            }

            Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.92f,
                                 300.0f, 30.0f};
            if (CheckCollisionPointRec(mouse, btnBack)) {
                appState_ = AppState::PROFILE_SELECT;
            }

            // Toggle notation
            Rectangle rNotation = {screenW - 290, screenH - 80, 250, 40};
            if (CheckCollisionPointRec(mouse, rNotation)) {
                selectedNotation_ = static_cast<NotationMode>(
                    (static_cast<int>(selectedNotation_) + 1) % 3);
            }
        }
    } else if (appState_ == AppState::PLAY) {
        Rectangle btnPause = {screenW - 85.0f, 25.0f, 60.0f, 60.0f};
        Rectangle btnMenu = {25.0f, 25.0f, 120.0f, 45.0f};
        Rectangle btnReady = {160.0f, 25.0f, 160.0f, 45.0f};

        if (clicked) {
            if (CheckCollisionPointRec(mouse, btnPause)) {
                isPaused_ = !isPaused_;
            } else if (isPaused_) {
                Rectangle btnResume = {screenW / 2.0f - 150.0f,
                                       screenH / 2.0f - 50.0f, 300.0f, 60.0f};
                Rectangle btnQuit = {screenW / 2.0f - 150.0f,
                                     screenH / 2.0f + 30.0f, 300.0f, 60.0f};
                if (CheckCollisionPointRec(mouse, btnResume)) {
                    isPaused_ = false;
                }
                if (CheckCollisionPointRec(mouse, btnQuit)) {
                    quitGame();
                }
            } else {
                if (CheckCollisionPointRec(mouse, btnMenu)) {
                    quitGame();
                }
                if (engState_ == EngineState::ENG_PLAYED &&
                    CheckCollisionPointRec(mouse, btnReady)) {
                    comm_.send(Message("ready"));
                    engState_ = EngineState::ENG_PLAYING;
                    lastResult_.active = false;
                }
            }
        }

        if (!isPaused_ && showKeyboard_) {
            handleVirtualKeyboardInput(screenH * 0.7f, mouse, screenW, screenH);
        }
    } else if (appState_ == AppState::GAME_OVER) {
        if (clicked) {
            Rectangle btnBack = {screenW / 2.0f - 150.0f, screenH * 0.7f,
                                 300.0f, 55.0f};
            if (CheckCollisionPointRec(mouse, btnBack)) {
                appState_ = AppState::MENU;
                currentChallenge_ = {};
                lastResult_ = {};
            }
        }
    }
}

void AppController::handleVirtualKeyboardInput(float pianoY, Vector2 mouse,
                                               float screenW, float screenH) {
    for (auto& b : blanchesAppuyees_) b = false;
    for (auto& b : noiresAppuyees_) b = false;

    int32_t numKeys = getSelectedGameKeys();
    int32_t numBlack = (numKeys / 7) * 5;
    float wW = screenW / (float)numKeys;
    float bW = wW * 0.6f;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse.y > pianoY) {
        bool hitBlack = false;
        for (int i = 0; i < numBlack; i++) {
            Rectangle rN = {(kBlackKeyIndices[i] + 1) * wW - bW / 2, pianoY, bW,
                            (screenH - pianoY) * 0.6f};
            if (CheckCollisionPointRec(mouse, rN)) {
                noiresAppuyees_[i] = true;
                hitBlack = true;
                break;
            }
        }
        if (!hitBlack) {
            int idx = static_cast<int>(mouse.x / wW);
            if (idx >= 0 && idx < numKeys) {
                blanchesAppuyees_[idx] = true;
            }
        }
    }
}

int32_t AppController::getSelectedGameKeys() const {
    for (const auto& g : availableGames_) {
        if (g.id == selectedGameId_) return g.keys;
    }
    return 7;
}

void AppController::startGame(const std::string& gtId) {
    if (engState_ == EngineState::ENG_DISCONNECTED) {
        errorMsg_ = "Moteur non connecté";
        errorTimer_ = 3.0f;
        return;
    }
    selectedGameId_ = gtId;
    scoreActuel_ = 0;
    currentChallenge_ = {};
    lastResult_ = {};
    feedbackAlpha_ = 0.0f;
    appState_ = AppState::PLAY;

    static const char* scaleStr[] = {"c", "d", "e", "f", "g", "a", "b"};
    comm_.send(Message(
        "config",
        {{"game", gtId},
         {"scale", scaleStr[static_cast<int>(selectedScale_)]},
         {"mode", selectedMode_ == ModeChoice::MODE_MAJ ? "maj" : "min"}}));
}

void AppController::quitGame() {
    comm_.send(Message("quit"));
    comm_.clearQueue();
    engState_ = EngineState::ENG_CONNECTED;
    appState_ = AppState::MENU;
    isPaused_ = false;
}

std::string AppController::findEngineBinary() {
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

pid_t AppController::spawnEngine(const std::string& enginePath) {
    pid_t pid = fork();
    if (pid == 0) {
        if (verbose_) {
            execl(enginePath.c_str(), "engine", "--verbose", nullptr);
            execlp("engine", "engine", "--verbose", nullptr);
        } else {
            execl(enginePath.c_str(), "engine", nullptr);
            execlp("engine", "engine", nullptr);
        }
        _exit(1);
    }
    if (pid > 0) {
        Logger::log("[App] Moteur démarré en arrière-plan (PID {})", pid);
    } else {
        Logger::err("[App] Échec fork pour lancer le moteur");
    }
    return pid;
}
