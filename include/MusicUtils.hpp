#ifndef CODE_UI_INCLUDE_MUSICUTILS_HPP_
#define CODE_UI_INCLUDE_MUSICUTILS_HPP_

#include "Types.hpp"
#include <string>
#include <vector>

namespace MusicUtils {

/**
 * @brief Sépare une chaîne de notes séparées par des espaces en vecteur
 */
[[nodiscard]] std::vector<std::string> splitNotes(const std::string& s);

/**
 * @brief Traduit le nom d'une note de la notation internationale (A-G) vers le
 * français (LA-SOL)
 */
[[nodiscard]] std::string noteLetterToFrench(char c);

/**
 * @brief Formate l'affichage d'une note selon le mode de notation
 */
[[nodiscard]] std::string noteDisplayLabel(const std::string& note,
                                           NotationMode mode);

/**
 * @brief Formate l'affichage du nom d'un accord
 */
[[nodiscard]] std::string chordDisplayLabel(const std::string& chordName,
                                            NotationMode mode);

/**
 * @brief Récupère le label de notation pour une touche blanche (DO, RE...)
 */
[[nodiscard]] std::string getNotationLabel(int32_t whiteIdx, NotationMode mode);

/**
 * @brief Renvoie la liste des notes d'une gamme donnée
 */
[[nodiscard]] std::vector<std::string> getScaleNotesList(ScaleChoice scale,
                                                         ModeChoice mode);

/**
 * @brief Formate le nom complet d'une gamme
 */
[[nodiscard]] std::string getScaleNameFormatted(ScaleChoice scale,
                                                ModeChoice mode,
                                                NotationMode notation);

/**
 * @brief Résout une note textuelle en index de touche de piano (blanche ou
 * noire)
 */
[[nodiscard]] NoteKey resolveKey(const std::string& note,
                                 int32_t baseKeyboardOctave = 4);

/**
 * @brief Détermine l'octave de base d'un défi à partir des notes attendues
 */
[[nodiscard]] int32_t
getChallengeBaseOctave(const std::vector<std::string>& expectedNotes);

/**
 * @brief Indique si deux représentations de notes font référence à la même
 * classe de note (sans octave)
 */
[[nodiscard]] bool isSameNoteClass(const std::string& a, const std::string& b);

/**
 * @brief Vérifie si une note fait partie d'une liste
 */
[[nodiscard]] bool noteInList(const std::string& noteBase,
                              const std::vector<std::string>& list);

} // namespace MusicUtils

#endif // CODE_UI_INCLUDE_MUSICUTILS_HPP_
