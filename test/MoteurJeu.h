#ifndef MOTEUR_JEU_H
#define MOTEUR_JEU_H

#include <vector>
#include <map>
#include <chrono>
#include <string>

// États pour le retour visuel
enum class Feedback { NEUTRE, VERT, ROUGE };

// Modes de jeu définis 
enum class ModeJeu { NOTES, ACCORDS, ACCORDS_RENVERSES, PORTEE };

class MoteurJeu {
public:
    // Constructeur : initialise la session avec une partition
    MoteurJeu(std::vector<std::vector<int>> partition, ModeJeu mode = ModeJeu::NOTES)
        : partition(partition), modeActuel(mode), index(0), score(0), combo(0), 
          scoreRecord(0), estTermine(false), feedbackActuel(Feedback::NEUTRE) {
        debutJeu = std::chrono::steady_clock::now();
    }
    void validerNote(std::vector<int> notesJouees); // Vérifie si la note/accord est juste
    void passerNote(); // Saute la note actuelle
    void reset();      // Remet à zéro pour une nouvelle partie

    int getScore() const { return score; }
    int getCombo() const { return combo; }
    int getScoreRecord() const { return scoreRecord; }
    Feedback getFeedback() const { return feedbackActuel; }
    ModeJeu getModeActuel() const { return modeActuel; }
    bool isTermine() const { return estTermine; }
    void ajouterNoteALaFin(std::vector<int> nouvelleNote) {
        partition.push_back(nouvelleNote);
    }
    int getNbNotesRestantes() const {
        return (int)partition.size() - index;
    }
    // Dans MoteurJeu.h
public:
    int getIndex() const { return index; }
    
    std::vector<std::vector<int>> getPartitionEntiere() const { return partition; }

    int getNbNotesRestantes() const {
        return (int)partition.size() - index;
    }

    void ajouterNoteALaFin(std::vector<int> nouvelleNote) {
        partition.push_back(nouvelleNote);
    }
    
    // Retourne la note attendue
    std::vector<int> getNotesAttendues() const;
    
    // Calcul de la note la plus difficile/manquée
    int getNoteLaPlusEchouee() const;
    
    // Temps écoulé depuis le début de l'exo
    double getTempsEcoule() const;

private:
    std::vector<std::vector<int>> partition;
    ModeJeu modeActuel;
    int index, score, combo, scoreRecord;
    bool estTermine;
    Feedback feedbackActuel;
    std::map<int, int> erreursParNote; // Compteur d'erreurs par ID MIDI
    std::chrono::steady_clock::time_point debutJeu;

    void avancer(); // Passe à l'index suivant ou termine le jeu
};

#endif