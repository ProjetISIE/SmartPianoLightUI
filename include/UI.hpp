#ifndef CODE_UI_INCLUDE_UI_HPP_
#define CODE_UI_INCLUDE_UI_HPP_

#include "raylib.h"
#include <string>
#include <vector>

// Forward declaration of AppController to avoid circular dependency
class AppController;

class UI {
  public:
    UI() = delete; ///< Static-only utility rendering class

    /**
     * @brief Point d'entrée principal pour le rendu de l'interface
     */
    static void draw(AppController& app, Vector2 mouse, float screenW,
                     float screenH);

  private:
    /**
     * @brief Dessine un bouton avec effet de survol
     * @return true si le bouton est survolé
     */
    [[nodiscard]] static bool drawButton(Rectangle rec, const char* text,
                                         Color color, Vector2 mouse,
                                         int32_t fontSize = 20);

    /**
     * @brief Dessine une portée de 5 lignes avec les notes indiquées
     */
    static void drawStaff(const AppController& app, Rectangle rec,
                          const std::vector<std::string>& notes, Color color);

    static void drawProfileSelect(AppController& app, Vector2 mouse,
                                  float screenW, float screenH);
    static void drawMenu(AppController& app, Vector2 mouse, float screenW,
                         float screenH);
    static void drawPlay(AppController& app, Vector2 mouse, float screenW,
                         float screenH);
    static void drawGameOver(AppController& app, Vector2 mouse, float screenW,
                             float screenH);
    static void drawVirtualKeyboard(AppController& app, float screenW,
                                    float screenH, Vector2 mouse);
};

#endif // CODE_UI_INCLUDE_UI_HPP_
