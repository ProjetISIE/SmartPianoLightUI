#include "Communication.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <raylib.h>
#include <sstream>
#include <string>
#include <vector>

// ─── Palette de couleurs ──────────────────────────────────────────────────────
static constexpr Color BG_DARK    = {12,  12,  22,  255};
static constexpr Color BG_PANEL   = {24,  24,  42,  255};
static constexpr Color BG_CARD    = {36,  36,  58,  255};
static constexpr Color COL_ACCENT = {82,  160, 255, 255};
static constexpr Color COL_GOLD   = {255, 196, 64,  255};
static constexpr Color COL_OK     = {72,  210, 120, 255};
static constexpr Color COL_ERR    = {220, 72,  72,  255};
static constexpr Color TEXT_PRI   = {230, 230, 248, 255};
static constexpr Color TEXT_SEC   = {150, 150, 185, 255};
static constexpr Color BTN_DEF    = {52,  52,  82,  255};
static constexpr Color BTN_HOV    = {70,  70,  110, 255};
static constexpr Color BTN_SEL    = {82,  160, 255, 255};
static constexpr Color KEY_WHI    = {240, 235, 225, 255};
static constexpr Color KEY_BLK    = {22,  22,  34,  255};
static constexpr Color KEY_CHAL   = {82,  160, 255, 200};
static constexpr Color KEY_GOOD   = {72,  210, 120, 200};
static constexpr Color KEY_BAD    = {220, 72,  72,  200};

// ─── États du jeu ─────────────────────────────────────────────────────────────
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

// ─── Informations d'une note ──────────────────────────────────────────────────
struct NoteInfo {
    char letter{'c'}; ///< Lettre (c, d, e, f, g, a, b)
    int  alter{0};    ///< Altération: -1=bémol, 0=naturel, 1=dièse
    int  octave{4};   ///< Octave (0-8)
};

// ─── Configuration sélectionnée par l'utilisateur ────────────────────────────
struct GameConfig {
    int gameIdx{0};  ///< 0=note, 1=chord, 2=inversed
    int scaleIdx{0}; ///< 0=aléatoire, 1=c … 7=b
    int modeIdx{0};  ///< 0=aléatoire, 1=maj, 2=min
};

// ─── Défi en cours ────────────────────────────────────────────────────────────
struct ChallengeData {
    std::string id;
    std::string type;     ///< "note" ou "chord"
    std::string label;    ///< Texte d'affichage principal
    std::string notesRaw; ///< Notes brutes (ex: "c4 e4 g4")
    std::vector<std::string> notes; ///< Notes séparées
};

// ─── Résultat du dernier défi ─────────────────────────────────────────────────
struct ResultData {
    std::string id;
    std::vector<std::string> correct;
    std::vector<std::string> incorrect;
    std::string duration;
    bool hasResult{false};
};

// ─── Statistiques de fin de partie ───────────────────────────────────────────
struct GameStats {
    std::string total;
    std::string perfect;
    std::string partial;
    std::string duration;
};

// ─── Notification flottante ───────────────────────────────────────────────────
struct Notification {
    std::string message;
    Color color{COL_ERR};
    float timer{0.f}; ///< Temps restant d'affichage (secondes)
};

// ─── Utilitaires de notes ─────────────────────────────────────────────────────

/**
 * @brief Analyse une note depuis sa représentation textuelle
 * @param s Chaîne de note (ex: "c4", "d#5", "gb3")
 * @return Structure NoteInfo avec lettre, altération et octave
 */
NoteInfo parseNote(const std::string& s) {
    NoteInfo info;
    if (s.empty()) return info;
    info.letter = s[0];
    size_t i    = 1;
    if (i < s.size() && s[i] == '#') { info.alter = 1;  ++i; }
    else if (i < s.size() && s[i] == 'b') { info.alter = -1; ++i; }
    if (i < s.size() && s[i] >= '0' && s[i] <= '8') info.octave = s[i] - '0';
    return info;
}

/**
 * @brief Convertit une note en numéro MIDI (60 = Do4)
 * @param info Informations de la note
 * @return Numéro MIDI
 */
int noteToMidi(const NoteInfo& info) {
    // Demi-tons depuis Do pour chaque lettre dans l'ordre a-g
    static constexpr int SEMITONES[] = {9, 11, 0, 2, 4, 5, 7};
    int idx = info.letter - 'a';
    if (idx < 0 || idx > 6) return 0;
    return (info.octave + 1) * 12 + SEMITONES[idx] + info.alter;
}

/**
 * @brief Sépare une chaîne de notes en vecteur
 * @param s Chaîne de notes séparées par espaces (ex: "c4 e4 g4")
 * @return Vecteur de notes individuelles
 */
std::vector<std::string> splitNotes(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (ss >> token) result.push_back(token);
    return result;
}

/**
 * @brief Vérifie si une note est dans une liste (comparaison MIDI, gère les enharmoniques)
 * @param note Note à chercher (ex: "f#3")
 * @param list Liste de notes candidates
 * @return Vrai si la note ou son équivalent enharmonique est présente
 */
bool noteInList(const std::string& note, const std::vector<std::string>& list) {
    int midi = noteToMidi(parseNote(note));
    for (const auto& n : list)
        if (noteToMidi(parseNote(n)) == midi) return true;
    return false;
}

/**
 * @brief Traduit une note en notation syllabique française avec altération et octave
 * @param info Informations de la note
 * @return Chaîne d'affichage (ex: "Do#4", "Solb5")
 */
std::string noteDisplay(const NoteInfo& info) {
    std::string name;
    switch (info.letter) {
    case 'c': name = "Do";  break;
    case 'd': name = "Re";  break;
    case 'e': name = "Mi";  break;
    case 'f': name = "Fa";  break;
    case 'g': name = "Sol"; break;
    case 'a': name = "La";  break;
    case 'b': name = "Si";  break;
    default:  name = "?";   break;
    }
    if (info.alter == 1)  name += "#";
    if (info.alter == -1) name += "b";
    name += std::to_string(info.octave);
    return name;
}

// ─── Clavier de piano ─────────────────────────────────────────────────────────
static constexpr int PIANO_OCT_START = 3; ///< Première octave affichée
static constexpr int PIANO_OCT_COUNT = 3; ///< Nombre d'octaves affichées
static constexpr int PIANO_WHITE_N   = PIANO_OCT_COUNT * 7; ///< Touches blanches totales

/**
 * @brief Dessine le clavier de piano avec notes mises en évidence
 * @param x         Position X du clavier
 * @param y         Position Y du clavier
 * @param totalW    Largeur totale disponible
 * @param totalH    Hauteur totale
 * @param challenge Notes du défi à jouer (bleues)
 * @param correct   Notes correctement jouées (vertes)
 * @param incorrect Notes incorrectement jouées (rouges)
 */
void drawPiano(int x, int y, int totalW, int totalH,
               const std::vector<std::string>& challenge,
               const std::vector<std::string>& correct,
               const std::vector<std::string>& incorrect) {
    float ww = static_cast<float>(totalW) / PIANO_WHITE_N;
    float wh = static_cast<float>(totalH);
    float bw = ww * 0.58f;
    float bh = wh * 0.62f;

    DrawRectangle(x, y, totalW, totalH, BG_CARD);

    // Touches blanches
    static constexpr char WHITE_LET[] = {'c', 'd', 'e', 'f', 'g', 'a', 'b'};
    for (int oct = 0; oct < PIANO_OCT_COUNT; ++oct) {
        int octave = PIANO_OCT_START + oct;
        for (int wi = 0; wi < 7; ++wi) {
            float kx = static_cast<float>(x) + static_cast<float>(oct * 7 + wi) * ww;
            float ky = static_cast<float>(y);
            std::string noteStr(1, WHITE_LET[wi]);
            noteStr += std::to_string(octave);

            Color keyColor = KEY_WHI;
            if (!incorrect.empty() && noteInList(noteStr, incorrect))      keyColor = KEY_BAD;
            else if (!correct.empty() && noteInList(noteStr, correct))     keyColor = KEY_GOOD;
            else if (!challenge.empty() && noteInList(noteStr, challenge)) keyColor = KEY_CHAL;

            DrawRectangle(static_cast<int>(kx + 1), static_cast<int>(ky),
                          static_cast<int>(ww - 2),  static_cast<int>(wh), keyColor);
            DrawRectangleLines(static_cast<int>(kx + 1), static_cast<int>(ky),
                               static_cast<int>(ww - 2),  static_cast<int>(wh), BG_DARK);

            // Étiquette sur les touches Do (repère d'octave)
            if (wi == 0) {
                std::string lbl = "C" + std::to_string(octave);
                int sz = std::max(9, std::min(static_cast<int>(ww / 2.6f), 14));
                DrawText(lbl.c_str(),
                         static_cast<int>(kx + ww / 2) - MeasureText(lbl.c_str(), sz) / 2,
                         static_cast<int>(ky + wh - sz - 4), sz, DARKGRAY);
            }
        }
    }

    // Touches noires (par-dessus les blanches)
    static constexpr float BLACK_OFF[] = {0.6f, 1.6f, 3.6f, 4.6f, 5.6f};
    static constexpr char  BLACK_LET[] = {'c', 'd', 'f', 'g', 'a'}; // base pour le dièse
    for (int oct = 0; oct < PIANO_OCT_COUNT; ++oct) {
        int octave = PIANO_OCT_START + oct;
        for (int bi = 0; bi < 5; ++bi) {
            float kx = static_cast<float>(x) +
                       (static_cast<float>(oct * 7) + BLACK_OFF[bi]) * ww;
            float ky = static_cast<float>(y);
            // noteInList via MIDI gère automatiquement les enharmoniques (ex: f#3 == gb3)
            std::string noteStr(1, BLACK_LET[bi]);
            noteStr += "#" + std::to_string(octave);

            Color keyColor = KEY_BLK;
            if (!incorrect.empty() && noteInList(noteStr, incorrect))      keyColor = KEY_BAD;
            else if (!correct.empty() && noteInList(noteStr, correct))     keyColor = KEY_GOOD;
            else if (!challenge.empty() && noteInList(noteStr, challenge)) keyColor = KEY_CHAL;

            DrawRectangle(static_cast<int>(kx), static_cast<int>(ky),
                          static_cast<int>(bw),  static_cast<int>(bh), keyColor);
        }
    }
}

// ─── Helpers d'interface ──────────────────────────────────────────────────────

/**
 * @brief Dessine un bouton toggle et retourne vrai si cliqué
 * @param rec      Rectangle du bouton
 * @param label    Texte du bouton
 * @param selected État sélectionné/actif
 * @param fontSize Taille de police
 * @return Vrai si le bouton vient d'être cliqué
 */
bool drawToggleButton(Rectangle rec, const char* label, bool selected,
                      int fontSize = 17) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rec);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
    Color bg     = selected ? BTN_SEL : (hovered ? BTN_HOV : BTN_DEF);
    DrawRectangleRounded(rec, 0.3f, 6, bg);
    int tw = MeasureText(label, fontSize);
    DrawText(label,
             static_cast<int>(rec.x + rec.width / 2) - tw / 2,
             static_cast<int>(rec.y + rec.height / 2) - fontSize / 2,
             fontSize, selected ? WHITE : TEXT_PRI);
    return clicked;
}

/**
 * @brief Dessine un bouton d'action principal et retourne vrai si cliqué
 * @param rec      Rectangle du bouton
 * @param label    Texte du bouton
 * @param color    Couleur de fond
 * @param fontSize Taille de police
 * @return Vrai si le bouton vient d'être cliqué
 */
bool drawActionButton(Rectangle rec, const char* label, Color color,
                      int fontSize = 20) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rec);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
    Color bg     = color;
    if (hovered) {
        bg.r = static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + 28));
        bg.g = static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + 28));
        bg.b = static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + 28));
    }
    DrawRectangleRounded(rec, 0.25f, 6, bg);
    int tw = MeasureText(label, fontSize);
    DrawText(label,
             static_cast<int>(rec.x + rec.width / 2) - tw / 2,
             static_cast<int>(rec.y + rec.height / 2) - fontSize / 2,
             fontSize, WHITE);
    return clicked;
}

/**
 * @brief Construit le message config depuis la configuration sélectionnée
 * @param cfg Configuration courante
 * @return Message config prêt à envoyer
 */
Message buildConfigMsg(const GameConfig& cfg) {
    static const char* GAME[]  = {"note", "chord", "inversed"};
    static const char* SCALE[] = {"", "c", "d", "e", "f", "g", "a", "b"};
    static const char* MODE[]  = {"", "maj", "min"};
    std::map<std::string, std::string> fields;
    fields["game"] = GAME[cfg.gameIdx];
    if (cfg.scaleIdx > 0) fields["scale"] = SCALE[cfg.scaleIdx];
    if (cfg.modeIdx  > 0) fields["mode"]  = MODE[cfg.modeIdx];
    return Message("config", fields);
}

// ─── Fonctions de dessin par état ─────────────────────────────────────────────

/**
 * @brief Dessine l'en-tête avec titre et indicateur de connexion
 * @param state     État de jeu courant
 * @param connected Indique si la connexion est active
 */
void drawHeader(GameState state, bool connected) {
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, 55, BG_PANEL);
    DrawRectangle(0, 54, sw, 2, COL_ACCENT);

    DrawText("Smart",   18,  12, 30, TEXT_PRI);
    DrawText("Piano",   87,  12, 30, COL_ACCENT);
    DrawText("Trainer", 163, 18, 20, TEXT_SEC);

    const char* statusLabel = "Déconnecté";
    Color       statusColor = COL_ERR;
    if (connected) {
        switch (state) {
        case GameState::CHALLENGE_ACTIVE:
        case GameState::WAITING_FOR_CHALLENGE:
        case GameState::SHOWING_RESULT:
            statusLabel = "En jeu";
            statusColor = COL_GOLD;
            break;
        case GameState::GAME_OVER:
            statusLabel = "Partie terminée";
            statusColor = COL_GOLD;
            break;
        default:
            statusLabel = "Connecté";
            statusColor = COL_OK;
            break;
        }
    }
    int lw = MeasureText(statusLabel, 16);
    int sx = sw - lw - 30;
    DrawCircle(sx - 10, 27, 5, statusColor);
    DrawText(statusLabel, sx, 19, 16, statusColor);
}

/**
 * @brief Dessine le pied de page avec les raccourcis clavier disponibles
 */
void drawFooter() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, sh - 28, sw, 28, BG_PANEL);
    DrawRectangle(0, sh - 29, sw, 1, {40, 40, 60, 255});
    DrawText("S=Démarrer   R=Prêt   Q=Quitter   Esc=Fermer",
             12, sh - 20, 13, TEXT_SEC);
}

/**
 * @brief Dessine l'écran de déconnexion avec animation de chargement
 */
void drawDisconnected() {
    int sw  = GetScreenWidth();
    int sh  = GetScreenHeight();
    int cx  = sw / 2;
    int cy  = (sh - 160 - 28 + 56) / 2;
    auto af = static_cast<float>(GetTime()) * 180.f;

    const char* msg1 = "Connexion au moteur de jeu...";
    DrawText(msg1, cx - MeasureText(msg1, 22) / 2, cy - 42, 22, TEXT_SEC);

    DrawCircleSector({static_cast<float>(cx), static_cast<float>(cy + 18)},
                     22.f, af, af + 270.f, 36, COL_ACCENT);
    DrawCircle(cx, cy + 18, 15, BG_DARK);

    const char* msg2 = "Verifiez que le moteur est lance sur /tmp/smartpiano.sock";
    DrawText(msg2, cx - MeasureText(msg2, 13) / 2, cy + 58, 13, TEXT_SEC);
}

/**
 * @brief Dessine l'écran de configuration avec sélection du mode, gamme et tonalité
 * @param cfg    Configuration courante (modifiée en place par les boutons)
 * @param ackErr Message d'erreur de l'ack précédent (vide si aucun)
 * @return Vrai si le bouton "Demarrer" est cliqué
 */
bool drawConfigScreen(GameConfig& cfg, const std::string& ackErr) {
    int  sw  = GetScreenWidth();
    int  cw  = sw - 40;
    int  cx  = 20;
    int  sy  = 65;
    bool ret = false;

    // Carte 1 : Mode de jeu
    DrawRectangleRounded({static_cast<float>(cx), static_cast<float>(sy),
                          static_cast<float>(cw), 76.f}, 0.06f, 6, BG_CARD);
    DrawText("MODE DE JEU", cx + 14, sy + 10, 13, TEXT_SEC);
    static const char* GAME_LABELS[] = {"Notes", "Accords", "Accords + Renversements"};
    int gBtnW = (cw - 40) / 3;
    for (int i = 0; i < 3; ++i) {
        Rectangle br = {static_cast<float>(cx + 14 + i * (gBtnW + 6)),
                        static_cast<float>(sy + 30), static_cast<float>(gBtnW), 32.f};
        if (drawToggleButton(br, GAME_LABELS[i], cfg.gameIdx == i)) cfg.gameIdx = i;
    }
    sy += 84;

    // Carte 2 : Gamme
    DrawRectangleRounded({static_cast<float>(cx), static_cast<float>(sy),
                          static_cast<float>(cw), 76.f}, 0.06f, 6, BG_CARD);
    DrawText("GAMME", cx + 14, sy + 10, 13, TEXT_SEC);
    static const char* SCALE_LABELS[] = {"Alea.", "Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};
    int sBtnW = (cw - 40) / 8;
    for (int i = 0; i < 8; ++i) {
        Rectangle br = {static_cast<float>(cx + 14 + i * (sBtnW + 3)),
                        static_cast<float>(sy + 30), static_cast<float>(sBtnW), 32.f};
        if (drawToggleButton(br, SCALE_LABELS[i], cfg.scaleIdx == i, 15)) cfg.scaleIdx = i;
    }
    sy += 84;

    // Carte 3 : Mode de la gamme
    DrawRectangleRounded({static_cast<float>(cx), static_cast<float>(sy),
                          static_cast<float>(cw), 76.f}, 0.06f, 6, BG_CARD);
    DrawText("MODE DE LA GAMME", cx + 14, sy + 10, 13, TEXT_SEC);
    static const char* MODE_LABELS[] = {"Aleatoire", "Majeur", "Mineur"};
    int mBtnW = (cw - 40) / 3;
    for (int i = 0; i < 3; ++i) {
        Rectangle br = {static_cast<float>(cx + 14 + i * (mBtnW + 6)),
                        static_cast<float>(sy + 30), static_cast<float>(mBtnW), 32.f};
        if (drawToggleButton(br, MODE_LABELS[i], cfg.modeIdx == i)) cfg.modeIdx = i;
    }
    sy += 84;

    // Message d'erreur ack (si présent)
    if (!ackErr.empty()) {
        DrawRectangleRounded({static_cast<float>(cx), static_cast<float>(sy),
                              static_cast<float>(cw), 38.f}, 0.1f, 6, {55, 18, 18, 255});
        std::string errTxt = "Erreur: " + ackErr;
        DrawText(errTxt.c_str(), cx + 14, sy + 11, 15, COL_ERR);
        sy += 46;
    }

    // Bouton Démarrer
    int btnW = 230;
    Rectangle startBtn = {static_cast<float>(sw / 2 - btnW / 2),
                          static_cast<float>(sy + 4), static_cast<float>(btnW), 42.f};
    ret = drawActionButton(startBtn, "Démarrer  [S]", COL_ACCENT);
    return ret;
}

/**
 * @brief Dessine l'écran d'attente de l'accusé de réception
 */
void drawWaitingAck() {
    int sw  = GetScreenWidth();
    int sh  = GetScreenHeight();
    int cx  = sw / 2;
    int cy  = (sh - 160 - 28 + 56) / 2;
    auto af = static_cast<float>(GetTime()) * 200.f;

    DrawCircleSector({static_cast<float>(cx), static_cast<float>(cy)},
                     20.f, af, af + 270.f, 36, COL_ACCENT);
    DrawCircle(cx, cy, 14, BG_DARK);

    const char* msg = "Configuration en cours...";
    DrawText(msg, cx - MeasureText(msg, 20) / 2, cy + 32, 20, TEXT_SEC);
}

/**
 * @brief Dessine l'écran de configuration validée, prêt à jouer
 * @param cfg     Configuration active
 * @param quitBtn Mis à vrai si "Quitter" est cliqué
 * @return Vrai si "Jouer" est cliqué
 */
bool drawConfigured(const GameConfig& cfg, bool& quitBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = (sh - 160 - 28 + 56) / 2;

    const char* title = "Partie configurée !";
    DrawText(title, cx - MeasureText(title, 28) / 2, cy - 68, 28, COL_OK);

    static const char* GAME_NAMES[]  = {"Notes", "Accords", "Accords + Renversements"};
    static const char* SCALE_NAMES[] = {"Alea.", "Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};
    static const char* MODE_NAMES[]  = {"Alea.", "Majeur", "Mineur"};
    std::string summary = std::string(GAME_NAMES[cfg.gameIdx]) + "  |  Gamme: " +
                          SCALE_NAMES[cfg.scaleIdx] + "  |  Mode: " + MODE_NAMES[cfg.modeIdx];
    DrawText(summary.c_str(),
             cx - MeasureText(summary.c_str(), 17) / 2, cy - 28, 17, TEXT_SEC);

    Rectangle btnPlay = {static_cast<float>(cx - 160), static_cast<float>(cy + 8), 148.f, 44.f};
    Rectangle btnQuit = {static_cast<float>(cx + 12),  static_cast<float>(cy + 8), 148.f, 44.f};
    bool play = drawActionButton(btnPlay, "Jouer  [R]",   COL_ACCENT);
    quitBtn   = drawActionButton(btnQuit, "Quitter  [Q]", COL_ERR);
    return play;
}

/**
 * @brief Dessine l'écran d'attente du prochain défi
 */
void drawWaitingChallenge() {
    int sw  = GetScreenWidth();
    int sh  = GetScreenHeight();
    int cx  = sw / 2;
    int cy  = (sh - 160 - 28 + 56) / 2;
    auto af = static_cast<float>(GetTime()) * 200.f;

    DrawCircleSector({static_cast<float>(cx), static_cast<float>(cy)},
                     20.f, af, af + 270.f, 36, COL_GOLD);
    DrawCircle(cx, cy, 14, BG_DARK);

    const char* msg = "En attente du defi...";
    DrawText(msg, cx - MeasureText(msg, 20) / 2, cy + 32, 20, TEXT_SEC);
}

/**
 * @brief Dessine l'écran de défi actif (note isolée ou accord)
 * @param chall   Données du défi courant
 * @param quitBtn Mis à vrai si "Abandonner" est cliqué
 */
void drawChallenge(const ChallengeData& chall, bool& quitBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = (sh - 160 - 28 + 56) / 2;

    // Badge type NOTE / ACCORD
    const char* badge    = (chall.type == "note") ? "NOTE" : "ACCORD";
    Color       badgeCol = (chall.type == "note") ? COL_ACCENT : COL_GOLD;
    int         bw       = MeasureText(badge, 14) + 22;
    DrawRectangleRounded({static_cast<float>(cx - bw / 2), static_cast<float>(cy - 78),
                          static_cast<float>(bw), 26.f}, 0.4f, 6, badgeCol);
    DrawText(badge, cx - MeasureText(badge, 14) / 2, cy - 72, 14, WHITE);

    // Numéro du défi
    if (!chall.id.empty()) {
        std::string idTxt = "Defi #" + chall.id;
        DrawText(idTxt.c_str(),
                 cx - MeasureText(idTxt.c_str(), 14) / 2, cy - 46, 14, TEXT_SEC);
    }

    // Label principal (nom de la note ou de l'accord)
    DrawText(chall.label.c_str(),
             cx - MeasureText(chall.label.c_str(), 38) / 2, cy - 18, 38, TEXT_PRI);

    // Détail des notes pour les accords
    if (chall.type == "chord" && !chall.notesRaw.empty()) {
        std::string notesTxt = "Notes: " + chall.notesRaw;
        DrawText(notesTxt.c_str(),
                 cx - MeasureText(notesTxt.c_str(), 16) / 2, cy + 32, 16, TEXT_SEC);
    }

    // Instruction
    const char* instr = "Jouez la note sur votre piano MIDI";
    DrawText(instr, cx - MeasureText(instr, 15) / 2, cy + 58, 15, TEXT_SEC);

    // Bouton Abandonner
    Rectangle qBtn = {static_cast<float>(cx - 82), static_cast<float>(cy + 86), 164.f, 38.f};
    quitBtn = drawActionButton(qBtn, "Abandonner  [Q]", COL_ERR, 17);
}

/**
 * @brief Dessine l'écran de résultat après un défi
 * @param chall   Défi correspondant (pour rappeler la demande)
 * @param result  Données du résultat reçu du serveur
 * @param nextBtn Mis à vrai si "Suivant" est cliqué
 * @param quitBtn Mis à vrai si "Abandonner" est cliqué
 */
void drawResult(const ChallengeData& chall, const ResultData& result,
                bool& nextBtn, bool& quitBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = (sh - 160 - 28 + 56) / 2;

    // Rappel de la demande
    std::string asked = "Demande: " + chall.label;
    DrawText(asked.c_str(),
             cx - MeasureText(asked.c_str(), 15) / 2, cy - 90, 15, TEXT_SEC);

    // Titre (parfait ou non)
    bool        perfect  = result.incorrect.empty() && !result.correct.empty();
    const char* title    = perfect ? "Parfait !" : "Resultat";
    Color       titleCol = perfect ? COL_OK : COL_ERR;
    DrawText(title, cx - MeasureText(title, 30) / 2, cy - 66, 30, titleCol);

    // Durée du défi
    if (!result.duration.empty()) {
        std::string durTxt = result.duration + " ms";
        DrawText(durTxt.c_str(),
                 cx - MeasureText(durTxt.c_str(), 15) / 2, cy - 32, 15, TEXT_SEC);
    }

    // Notes correctes
    if (!result.correct.empty()) {
        std::string okTxt = "Correct:";
        for (const auto& n : result.correct) okTxt += "  " + noteDisplay(parseNote(n));
        DrawText(okTxt.c_str(),
                 cx - MeasureText(okTxt.c_str(), 18) / 2, cy - 4, 18, COL_OK);
    }

    // Notes incorrectes
    if (!result.incorrect.empty()) {
        std::string errTxt = "Incorrect:";
        for (const auto& n : result.incorrect) errTxt += "  " + noteDisplay(parseNote(n));
        DrawText(errTxt.c_str(),
                 cx - MeasureText(errTxt.c_str(), 18) / 2, cy + 26, 18, COL_ERR);
    }

    // Boutons d'action
    Rectangle btnNext = {static_cast<float>(cx - 162), static_cast<float>(cy + 62), 152.f, 40.f};
    Rectangle btnQuit = {static_cast<float>(cx + 10),  static_cast<float>(cy + 62), 152.f, 40.f};
    nextBtn = drawActionButton(btnNext, "Suivant  [R]",    COL_ACCENT);
    quitBtn = drawActionButton(btnQuit, "Abandonner  [Q]", COL_ERR, 17);
}

/**
 * @brief Dessine l'écran de fin de partie avec statistiques complètes
 * @param stats   Statistiques (total, parfait, partiel, durée)
 * @param playBtn Mis à vrai si "Rejouer" est cliqué
 * @param menuBtn Mis à vrai si "Menu" est cliqué
 */
void drawGameOver(const GameStats& stats, bool& playBtn, bool& menuBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = (sh - 160 - 28 + 56) / 2;

    const char* title = "Partie terminée !";
    DrawText(title, cx - MeasureText(title, 32) / 2, cy - 96, 32, COL_GOLD);

    // Carte de statistiques
    int cardW = 320;
    int cardX = cx - cardW / 2;
    int cardY = cy - 54;
    DrawRectangleRounded({static_cast<float>(cardX), static_cast<float>(cardY),
                          static_cast<float>(cardW), 134.f}, 0.07f, 6, BG_CARD);

    auto drawStat = [&](const char* lbl, const std::string& val, int yOff, Color col) {
        DrawText(lbl, cardX + 20, cardY + yOff, 17, TEXT_SEC);
        DrawText(val.c_str(),
                 cardX + cardW - 20 - MeasureText(val.c_str(), 20),
                 cardY + yOff, 20, col);
    };
    drawStat("Durée",    stats.duration.empty() ? "-" : stats.duration + " ms", 14, TEXT_PRI);
    drawStat("Total",    stats.total.empty()    ? "-" : stats.total,            42, TEXT_PRI);
    drawStat("Parfaits", stats.perfect.empty()  ? "-" : stats.perfect,          70, COL_OK);
    drawStat("Partiels", stats.partial.empty()  ? "-" : stats.partial,          98, COL_ERR);

    Rectangle btnPlay = {static_cast<float>(cx - 162), static_cast<float>(cy + 100), 152.f, 42.f};
    Rectangle btnMenu = {static_cast<float>(cx + 10),  static_cast<float>(cy + 100), 152.f, 42.f};
    playBtn = drawActionButton(btnPlay, "Rejouer  [S]", COL_ACCENT);
    menuBtn = drawActionButton(btnMenu, "Menu",          BTN_DEF);
}

/**
 * @brief Dessine la notification flottante si active et décrémente son timer
 * @param notif Notification à afficher (timer décrémenté en interne)
 */
void drawNotification(Notification& notif) {
    if (notif.timer <= 0.f) return;
    notif.timer -= GetFrameTime();

    int   sw    = GetScreenWidth();
    float alpha = std::min(1.f, notif.timer / 0.4f);
    int   nw    = std::min(sw - 40, MeasureText(notif.message.c_str(), 15) + 44);
    int   nx    = sw / 2 - nw / 2;
    int   ny    = 62;

    Color bg = notif.color;
    bg.a     = static_cast<unsigned char>(200.f * alpha);
    DrawRectangleRounded({static_cast<float>(nx), static_cast<float>(ny),
                          static_cast<float>(nw), 38.f}, 0.2f, 6, bg);

    Color tc = {255, 255, 255, static_cast<unsigned char>(255.f * alpha)};
    DrawText(notif.message.c_str(),
             nx + nw / 2 - MeasureText(notif.message.c_str(), 15) / 2,
             ny + 12, 15, tc);
}

// ─── Point d'entrée ───────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Argument --timeout <ms> : fermeture automatique (utilisé pour la couverture de code)
    int timeoutMs = 0;
    for (int i = 1; i < argc - 1; ++i)
        if (std::string(argv[i]) == "--timeout") timeoutMs = std::stoi(argv[i + 1]);

    Logger::log("[MAIN] Démarrage UI Smart Piano");

    GameState     currentState{GameState::DISCONNECTED};
    GameConfig    config;
    ChallengeData challenge;
    ResultData    result;
    GameStats     stats;
    Notification  notif;
    std::string   ackError;
    float         reconnectTimer{0.f};

    auto comm = std::make_unique<Communication>();
    if (comm->connect()) currentState = GameState::CONNECTED;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(900, 680, "Smart Piano Trainer");
    SetWindowMinSize(600, 450);
    SetTargetFPS(60);

    double startTime = GetTime();

    while (!WindowShouldClose()) {
        // Fermeture automatique après --timeout ms (utilisé pour la couverture)
        if (timeoutMs > 0 && (GetTime() - startTime) * 1000.0 > timeoutMs) break;

        int sw     = GetScreenWidth();
        int sh     = GetScreenHeight();
        int pianoY = sh - 160 - 28;
        int pianoH = 160;

        // ─── Gestion de la connexion / reconnexion ───────────────────────────
        if (!comm->isConnected()) {
            if (currentState != GameState::DISCONNECTED) {
                Logger::log("[MAIN] Connexion perdue, retour a DISCONNECTED");
                comm->disconnect();
                currentState   = GameState::DISCONNECTED;
                reconnectTimer = 2.f;
            }
            reconnectTimer -= GetFrameTime();
            if (reconnectTimer <= 0.f) {
                if (comm->connect()) {
                    currentState = GameState::CONNECTED;
                    ackError.clear();
                    Logger::log("[MAIN] Reconnecté");
                } else reconnectTimer = 2.f;
            }
        }

        // ─── Traitement des messages reçus ───────────────────────────────────
        while (comm->isConnected()) {
            auto msgOpt = comm->popMessage();
            if (!msgOpt) break;
            Message msg = *msgOpt;
            Logger::log("[MAIN] Traitement: {}", msg.getType());

            if (msg.getType() == "ack") {
                if (msg.getField("status") == "ok") {
                    currentState = GameState::CONFIGURED;
                    ackError.clear();
                } else {
                    currentState = GameState::CONNECTED;
                    ackError     = msg.getField("message");
                    if (!msg.getField("code").empty())
                        ackError = "[" + msg.getField("code") + "] " + ackError;
                }
            } else if (msg.getType() == "note") {
                challenge.id       = msg.getField("id");
                challenge.type     = "note";
                challenge.notesRaw = msg.getField("note");
                challenge.notes    = {challenge.notesRaw};
                challenge.label    = noteDisplay(parseNote(challenge.notesRaw));
                result.hasResult   = false;
                currentState       = GameState::CHALLENGE_ACTIVE;
            } else if (msg.getType() == "chord") {
                challenge.id       = msg.getField("id");
                challenge.type     = "chord";
                challenge.label    = msg.getField("name");
                challenge.notesRaw = msg.getField("notes");
                challenge.notes    = splitNotes(challenge.notesRaw);
                result.hasResult   = false;
                currentState       = GameState::CHALLENGE_ACTIVE;
            } else if (msg.getType() == "result") {
                result.id        = msg.getField("id");
                result.correct   = splitNotes(msg.getField("correct"));
                result.incorrect = splitNotes(msg.getField("incorrect"));
                result.duration  = msg.getField("duration");
                result.hasResult = true;
                currentState     = GameState::SHOWING_RESULT;
            } else if (msg.getType() == "over") {
                stats.total    = msg.getField("total");
                stats.perfect  = msg.getField("perfect");
                stats.partial  = msg.getField("partial");
                stats.duration = msg.getField("duration");
                currentState   = GameState::GAME_OVER;
            } else if (msg.getType() == "error") {
                std::string errMsg =
                    "[" + msg.getField("code") + "] " + msg.getField("message");
                Logger::err("[MAIN] Erreur moteur: {}", errMsg);
                notif = {errMsg, COL_ERR, 4.f};
                // Erreur interne : retour à l'état connecté (réinitialisation)
                if (msg.getField("code") == "internal") currentState = GameState::CONNECTED;
            }
        }

        // ─── Raccourcis clavier ──────────────────────────────────────────────
        if (IsKeyPressed(KEY_S)) {
            if (currentState == GameState::CONNECTED ||
                currentState == GameState::CONFIGURED ||
                currentState == GameState::GAME_OVER) {
                ackError.clear();
                comm->send(buildConfigMsg(config));
                currentState = GameState::WAITING_FOR_ACK;
                Logger::log("[MAIN] Config envoyée");
            }
        }
        if (IsKeyPressed(KEY_R)) {
            if (currentState == GameState::CONFIGURED ||
                currentState == GameState::SHOWING_RESULT) {
                comm->send(Message("ready"));
                currentState = GameState::WAITING_FOR_CHALLENGE;
                Logger::log("[MAIN] Ready envoyé");
            }
        }
        if (IsKeyPressed(KEY_Q)) {
            if (currentState != GameState::DISCONNECTED &&
                currentState != GameState::CONNECTED) {
                comm->send(Message("quit"));
                currentState = GameState::CONNECTED;
                Logger::log("[MAIN] Quit envoyé");
            }
        }

        // ─── Rendu ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawHeader(currentState, comm->isConnected());

        switch (currentState) {
        case GameState::DISCONNECTED:
            drawDisconnected();
            break;
        case GameState::CONNECTED: {
            bool start = drawConfigScreen(config, ackError);
            if (start) {
                ackError.clear();
                comm->send(buildConfigMsg(config));
                currentState = GameState::WAITING_FOR_ACK;
            }
            break;
        }
        case GameState::WAITING_FOR_ACK:
            drawWaitingAck();
            break;
        case GameState::CONFIGURED: {
            bool quit = false;
            bool play = drawConfigured(config, quit);
            if (play) { comm->send(Message("ready")); currentState = GameState::WAITING_FOR_CHALLENGE; }
            if (quit) { comm->send(Message("quit"));  currentState = GameState::CONNECTED; }
            break;
        }
        case GameState::WAITING_FOR_CHALLENGE:
            drawWaitingChallenge();
            break;
        case GameState::CHALLENGE_ACTIVE: {
            bool quit = false;
            drawChallenge(challenge, quit);
            if (quit) { comm->send(Message("quit")); currentState = GameState::CONNECTED; }
            break;
        }
        case GameState::SHOWING_RESULT: {
            bool next = false;
            bool quit = false;
            drawResult(challenge, result, next, quit);
            if (next) { comm->send(Message("ready")); currentState = GameState::WAITING_FOR_CHALLENGE; }
            if (quit) { comm->send(Message("quit"));  currentState = GameState::CONNECTED; }
            break;
        }
        case GameState::GAME_OVER: {
            bool play = false;
            bool menu = false;
            drawGameOver(stats, play, menu);
            if (play) {
                ackError.clear();
                comm->send(buildConfigMsg(config));
                currentState = GameState::WAITING_FOR_ACK;
            }
            if (menu) currentState = GameState::CONNECTED;
            break;
        }
        }

        // Clavier de piano en bas (3 octaves, avec mise en évidence des notes)
        std::vector<std::string> chalNotes;
        std::vector<std::string> correctNotes;
        std::vector<std::string> incorrectNotes;
        if (currentState == GameState::CHALLENGE_ACTIVE) {
            chalNotes = challenge.notes;
        } else if (currentState == GameState::SHOWING_RESULT) {
            chalNotes      = challenge.notes;
            correctNotes   = result.correct;
            incorrectNotes = result.incorrect;
        }
        drawPiano(0, pianoY, sw, pianoH, chalNotes, correctNotes, incorrectNotes);

        drawNotification(notif);
        drawFooter();
        EndDrawing();
    }

    comm->disconnect();
    CloseWindow();
    Logger::log("[MAIN] Fermeture UI");
    return 0;
}
