#ifndef CODE_UI_INCLUDE_APPCONTROLLER_HPP_
#define CODE_UI_INCLUDE_APPCONTROLLER_HPP_

#include "Communication.hpp"
#include "Types.hpp"
#include "raylib.h"
#include <cstdint>
#include <string>
#include <unistd.h>
#include <vector>

class AppController {
  private:
    // Process & System variables
    pid_t enginePid_{-1};
    bool verbose_{false};
    bool fullscreen_{false};
    int32_t timeoutMs_{-1};
    float timeoutTimer_{0.0f};

    // Connection & Communication variables
    Communication comm_;
    EngineState engState_{EngineState::ENG_DISCONNECTED};
    float connRetryTimer_{0.0f};

    // User session state
    std::vector<UserProfile> profiles_;
    int32_t currentUserIdx_{0};
    bool isNamingProfile_{false};
    std::string inputName_;

    // Game state
    AppState appState_{AppState::PROFILE_SELECT};
    bool isPaused_{false};
    ScaleChoice selectedScale_{ScaleChoice::SCALE_C};
    ModeChoice selectedMode_{ModeChoice::MODE_MAJ};
    std::string selectedGameId_;
    std::vector<GameInfo> availableGames_;
    int32_t scoreActuel_{0};
    NotationMode selectedNotation_{NotationMode::SYLLABIC};
    bool showKeyboard_{true};

    // Challenges and stats
    Challenge currentChallenge_;
    ChallengeResult lastResult_;
    GameStats gameStats_;

    // UI feedback
    const char* feedbackMsg_{""};
    Color feedbackColor_{WHITE};
    float feedbackAlpha_{0.0f};
    std::string errorMsg_;
    float errorTimer_{0.0f};

    // Piano key presses (virtual)
    bool blanchesAppuyees_[14]{};
    bool noiresAppuyees_[10]{};

    // Index mappings for keyboard black keys
    static constexpr int32_t kBlackKeyIndices[10] = {0, 1, 3,  4,  5,
                                                     7, 8, 10, 11, 12};

  public:
    AppController();
    ~AppController();

    // Explicitly delete copy operations to enforce thread/resource safety
    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;
    AppController(AppController&&) = delete;
    AppController& operator=(AppController&&) = delete;

    /**
     * @brief Initialise les paramètres de l'application via les arguments du
     * main
     */
    void init(int argc, char* argv[]);

    /**
     * @brief Démarre et exécute la boucle principale du jeu
     */
    void run();

    /**
     * @brief Nettoie proprement les ressources (arrête le moteur, déconnecte le
     * socket)
     */
    void cleanup();

  private:
    void processIncomingMessages();
    void updateLogic(float dt, Vector2 mouse, bool clicked, float screenW,
                     float screenH);
    void handleVirtualKeyboardInput(float pianoY, Vector2 mouse, float screenW,
                                    float screenH);
    void startGame(const std::string& gtId);
    void quitGame();
    [[nodiscard]] int32_t getSelectedGameKeys() const;

    [[nodiscard]] std::string findEngineBinary();
    [[nodiscard]] pid_t spawnEngine(const std::string& enginePath);

    // Give UI class full read/write access to state details
    friend class UI;
};

#endif // CODE_UI_INCLUDE_APPCONTROLLER_HPP_
