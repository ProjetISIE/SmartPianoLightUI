#ifndef INTERFACE_PIANO_H
#define INTERFACE_PIANO_H

#include "MoteurJeu.h"
#include <string>
#include <vector> 

class InterfacePiano {
public:
    InterfacePiano(MoteurJeu& m) : moteur(m) {}

    // Affichage du Menu 
    std::string getEcranDemarrage() {
        return "PIANO TRAINER\n1. Notes\n2. Accords\n3. Renverses\n4. Portee";
    }

    // Affiche l'instruction 
    std::string getInstruction(); 
    
    // Retourne "VERT", "ROUGE" ou "GRIS" pour l'écran
    std::string getCouleur(); 
    
    // Formate la ligne Score/Combo/Record pour l'utilisateur 
    std::string getStatsFormatees();

    // Donne un feedback sur la vitesse 
    std::string getMessageRapidite(double temps);
    
    // Message spécifique et ludique selon l'erreur
    std::string getMessageErreurLudique(bool erreurOctave, bool erreurOrdre);
    
    // Message de fin 
    std::string getEcranBonus();

    std::string getVisualisationReserve() {
        std::string res = "Suivantes: ";
        // On récupère les notes après l'index actuel via le moteur
        auto partitionComplete = moteur.getPartitionEntiere(); 
        int actuel = moteur.getIndex();
        
        for(int i = actuel + 1; i < actuel + 4 && i < (int)partitionComplete.size(); ++i) {
            res += std::to_string(partitionComplete[i][0]) + " ";
        }
        return res;
    }

private:
    MoteurJeu& moteur; 
};

#endif