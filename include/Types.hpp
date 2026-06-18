#ifndef CODE_UI_INCLUDE_TYPES_HPP_
#define CODE_UI_INCLUDE_TYPES_HPP_

#include "raylib.h"
#include <string>
#include <vector>

namespace Colors {
inline constexpr Color kVertFonce = {20, 40, 20, 255};
inline constexpr Color kVertEclatant = {100, 255, 100, 255};
inline constexpr Color kOrEclatant = {255, 215, 0, 255};
inline constexpr Color kRougeErreur = {230, 41, 55, 255};
inline constexpr Color kOrangeNote = {255, 140, 0, 255};
} // namespace Colors

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

struct GameInfo {
    std::string id;
    std::string name;
    int32_t keys;
};

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
    std::string
        rawName; ///< Nom brut envoyé par le moteur (ex: "c", "c#", "C maj")
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

struct NoteKey {
    bool isBlack{false};
    int32_t index{-1};
    bool valid{false};
};

#endif // CODE_UI_INCLUDE_TYPES_HPP_
