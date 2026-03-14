#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "MoteurJeu.h"
#include "InterfacePiano.h"
#include <thread>

// TESTS DU MOTEUR ---

TEST_CASE("test1: Note simple") {
    MoteurJeu moteur({{60}}); // Do cf lien doc annexe
    moteur.validerNote({60});//comparaison note jouée et note attendue
    CHECK(moteur.getScore() == 10);//10 points pour une note correcte
    CHECK(moteur.getFeedback() == Feedback::VERT);//retour vert pour succès
}

TEST_CASE("test2: plusieurs notes") {
    MoteurJeu moteur({{60}, {62}}); // Do, Ré
    moteur.validerNote({60}); // +10
    moteur.validerNote({62}); // +20 car encourage utilisateurs
    CHECK(moteur.getScore() == 30);//vérification du score total
    CHECK(moteur.getCombo() == 2);//vérification nombre de notes actuelles correctes
}

TEST_CASE("test3: Perte du combo") {
    MoteurJeu moteur({{60}, {62}});
    moteur.validerNote({60}); // oui
    moteur.validerNote({99}); //non
    CHECK(moteur.getScore() == 10);
    CHECK(moteur.getCombo() == 0);
    CHECK(moteur.getFeedback() == Feedback::ROUGE);
}

TEST_CASE("test4: La note la plus manquee /difficile") {
    MoteurJeu moteur({{60}});//attendue Do
    moteur.validerNote({99}); // erreur sur la note 60
    CHECK(moteur.getNoteLaPlusEchouee() == 60);//on conserve la note mais jsp comment mettre dans un tableau
}

TEST_CASE("test5: Differences des octaves sur la portée") {
    MoteurJeu moteur({{48}, {60}}); // Do2 et Do4
    moteur.validerNote({60}); 
    CHECK(moteur.getFeedback() == Feedback::ROUGE); 
    moteur.validerNote({48}); 
    CHECK(moteur.getFeedback() == Feedback::VERT); 
}

TEST_CASE("test6: Accord fondamental et renversement") {
    std::vector<int> DoMajeurFondamental = {60, 64, 67}; // Do4, Mi4, Sol4
    std::vector<int> DoMajeurRenverse    = {64, 67, 72}; // Mi4, Sol4, Do5
    MoteurJeu moteur({DoMajeurFondamental, DoMajeurRenverse});

    SUBCASE("Accord de base valide") {
        moteur.validerNote({60, 64, 67});
        CHECK(moteur.getFeedback() == Feedback::VERT);
        CHECK(moteur.getScore() == 15);
    }

    SUBCASE("Une note incorrecte dans le fondamental") {
        moteur.validerNote({60, 64, 68}); // preciser laquelle est fausse?
        CHECK(moteur.getFeedback() == Feedback::ROUGE);
        CHECK(moteur.getScore() == 5);// encouragement pour 2 notes correctes
    }

    SUBCASE("Bonnes notes mais mauvais ordre") {
        moteur.validerNote({67, 64, 60}); // 
        CHECK(moteur.getFeedback() == Feedback::ROUGE);// ordre incorrect a preciser a l'utilisateur ou compliquer de verifier si dans liste ou hors combos ?
        CHECK(moteur.getScore() == 0);
    }

    SUBCASE("Renversement") {
        moteur.passerNote(); // Accord suivant à renverser
        moteur.validerNote({64, 67, 72}); // Accord exact
        CHECK(moteur.getFeedback() == Feedback::VERT);
        CHECK(moteur.getScore() == 15);
    }

    SUBCASE("renversement sans octave montees") {
        moteur.passerNote();
        moteur.validerNote({64, 67, 60}); 
        CHECK(moteur.getFeedback() == Feedback::ROUGE);// affichage ludique ex: Do non monté d’une octave?
    }

    SUBCASE("note hors de la gamme/rate") {
        moteur.passerNote();
        moteur.validerNote({64, 67, 71}); // 71 = Si4, hors C majeur
        CHECK(moteur.getFeedback() == Feedback::ROUGE);
    }
}
TEST_CASE("Test7: Note unique avec durée") {
    int noteAttendue = 60;       
    double dureeAttendue = 1.0;  // durée cible car parfois lon ou court mais jsp si visible sur partition
    double tolerance = 0.1;      // tolérance acceptable
    double dureeJouee = 1.0;  ////fonction donnant la duree utilisateur a reflechir
    MoteurJeu moteur({{noteAttendue}});
    SUBCASE("Note correcte et durée correcte") {
        /*
        fonction donnant la duree utilisateur a mettre ici
        */
        if (dureeJouee >= dureeAttendue - tolerance && dureeJouee <= dureeAttendue + tolerance) {
            moteur.validerNote({noteAttendue});
            CHECK(moteur.getFeedback() == Feedback::VERT);
            CHECK(moteur.getScore() == 10);
        }
    }

    SUBCASE("Note correcte mais durée trop courte") {
        /*
        fonction donnant la duree utilisateur a mettre ici
        */
        if (dureeJouee >= dureeAttendue - tolerance && dureeJouee <= dureeAttendue + tolerance) {
            moteur.validerNote({noteAttendue});
        } else {
            moteur.validerNote({0}); // simule échec
        }
        CHECK(moteur.getFeedback() == Feedback::ROUGE);
        CHECK(moteur.getScore() == 0);
    }

    SUBCASE("Note correcte mais durée trop longue") {
       /*
        fonction donnant la duree utilisateur a mettre ici
        */
        if (dureeJouee >= dureeAttendue - tolerance && dureeJouee <= dureeAttendue + tolerance) {
            moteur.validerNote({noteAttendue});
        } else {
            moteur.validerNote({0}); // simule échec
        }
        CHECK(moteur.getFeedback() == Feedback::ROUGE);
        CHECK(moteur.getScore() == 0);
    }
    SUBCASE("Note incorrecte") {
        moteur.validerNote({61}); // Do#4
        CHECK(moteur.getFeedback() == Feedback::ROUGE);
        CHECK(moteur.getScore() == 0);
    }
}
TEST_CASE("test8:Record ") {
    MoteurJeu moteur({{60}}); // 
    moteur.validerNote({60});
    CHECK(moteur.getScore() == 10);
    CHECK(moteur.getScoreRecord() == 10);
//nouvelle partie
    moteur.reset();
    CHECK(moteur.getScore() == 0);       // Le score actuel est bien revenu à 0
    CHECK(moteur.getScoreRecord() == 10); // le score maximum est conservé
}

TEST_CASE("test 9; Bonus de rapidité ?") {//dans jeu de notes
    MoteurJeu moteur({{60}, {62}});

    SUBCASE("Réponse rapide") {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 
        moteur.validerNote({60});
        CHECK(moteur.getScore() >= 10); 
        CHECK(moteur.getFeedback() == Feedback::VERT);// affichage ludique ex: "Rapide !" ?
    }

    SUBCASE("Réponse lente") {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Simule 3s d'attente
        moteur.validerNote({60});
        // Coder une logique qui réduit les points si trop lent
        CHECK(moteur.getTempsEcoule() >= 3.0);
    }
}

TEST_CASE("test 10: Vérification du changement de mode de jeu") {
    std::vector<std::vector<int>> partitionAccord = {{60, 64, 67}};

    SUBCASE("Le moteur est bien en mode ACCORDS_RENVERSES") {
        MoteurJeu moteur(partitionAccord, ModeJeu::ACCORDS_RENVERSES);
        CHECK(moteur.getModeActuel() == ModeJeu::ACCORDS_RENVERSES);
    }
}

TEST_CASE("Moteur11: Saut de note") {
    MoteurJeu moteur({{60}, {62}});
    
    moteur.validerNote({60}); 
    CHECK(moteur.getScore() == 10);
    CHECK(moteur.getCombo() == 1);

    moteur.passerNote(); // On saute la note
    
    CHECK(moteur.getIndex() == 2); // Le moteur a bien avancé à la fin 
    CHECK(moteur.getScore() == 10); // Le score n'a pas bougé
    CHECK(moteur.getCombo() == 0);  // Le combo est remis à zéro proprement
    CHECK(moteur.getFeedback() == Feedback::NEUTRE); // Pas de rouge affiché
}

TEST_CASE("test 11: Chargement de notes") {
    // ex 6 notes initiales
    std::vector<std::vector<int>> notesInitiales = {{60}, {62}, {64}, {65}, {67}, {69}};
    MoteurJeu moteur(notesInitiales);
    moteur.validerNote({60});
    moteur.ajouterNoteALaFin({71}); 
    CHECK(moteur.getNbNotesRestantes() == 6); 
}

// --- TESTS DE L'INTERFACE ---

TEST_CASE("IHM1: Affichage d'unenote seule") {
    MoteurJeu moteur({{60}}); 
    InterfacePiano ihm(moteur);
    CHECK(ihm.getInstruction() == "JOUEZ LA NOTE: 60");
}

TEST_CASE("IHM2: Affichage d'accord") {
    MoteurJeu moteur({{60, 64, 67}}); 
    InterfacePiano ihm(moteur);
    CHECK(ihm.getInstruction().find("ACCORD") != std::string::npos);// Vérifie que l'IHM détecte qu'il s'agit d'un accord
}

TEST_CASE("IHM3: Feedback de réussite") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    moteur.validerNote({60});
    CHECK(ihm.getCouleur() == "VERT");
}

TEST_CASE("IHM4: Feedback de rapidité") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    CHECK(ihm.getMessageRapidite(0.5) == "Bravo ! Tu es super rapide !");
}

TEST_CASE("IHM5: Feedback 'Dans les temps'") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    CHECK(ihm.getMessageRapidite(1.5) == "Bien joué, tu es dans les temps.");
}

TEST_CASE("IHM6: Erreur d'octave") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    CHECK(ihm.getMessageErreurLudique(true, false) == "C'est la bonne note, mais pas la bonne octave !");
}

TEST_CASE("IHM7: Alerte ludique sur erreur de renversement") {
    MoteurJeu moteur({{60, 64, 67}});
    InterfacePiano ihm(moteur);
    CHECK(ihm.getMessageErreurLudique(false, true) == "Les notes sont bonnes, mais vérifie le renversement !");
}

TEST_CASE("IHM8: Affichage du Score et du Combo en temps réel") {
    MoteurJeu moteur({{60}, {62}});
    InterfacePiano ihm(moteur);
    moteur.validerNote({60}); 
    std::string stats = ihm.getStatsFormatees();
    CHECK(stats.find("Score: 10") != std::string::npos);
    CHECK(stats.find("Combo: x1") != std::string::npos);
}

TEST_CASE("IHM9: Affichage du Record") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    
    moteur.validerNote({60});
    moteur.reset();
    std::string stats = ihm.getStatsFormatees();
    CHECK(stats.find("Record: 10") != std::string::npos);
}

TEST_CASE("IHM10: Suggestion d'aide pédagogique") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    moteur.validerNote({99});
    std::string stats = ihm.getStatsFormatees();
    CHECK(stats.find("Aide: Révisez la note 60") != std::string::npos);
}

TEST_CASE("IHM11: Message de clôture de session") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    moteur.passerNote(); // On termine l'exercice
    CHECK(ihm.getInstruction() == "SESSION TERMINEE");
}

TEST_CASE("IHM: Détection d'un nouveau record") {
    MoteurJeu moteur({{60}, {62}});
    InterfacePiano ihm(moteur);
    // Partie 1 : record initial
    moteur.validerNote({60}); 
    moteur.reset();

    // Partie 2 : nvx record
    moteur.validerNote({60}); 
    moteur.validerNote({62}); 
    CHECK(ihm.getEcranBonus() == "NOUVEAU RECORD ! BRAVO !");
}

TEST_CASE("IHM12:Message de combo élevé") {
    // test pour un gros combo
    MoteurJeu moteur({{60}, {62}, {64}, {65}, {67}, {69}}); 
    InterfacePiano ihm(moteur);
    for(int i=0; i<6; i++) moteur.validerNote(moteur.getNotesAttendues());
    CHECK(ihm.getEcranBonus().find("Machiiiine") != std::string::npos);
}
TEST_CASE("IHM13: Affichage de l'écran de démarrage") {
    MoteurJeu moteur({{60}});
    InterfacePiano ihm(moteur);
    std::string menu = ihm.getEcranDemarrage();
    CHECK(menu.find("Notes") != std::string::npos);
    CHECK(menu.find("Accords") != std::string::npos);
    CHECK(menu.find("Renversés") != std::string::npos);
    CHECK(menu.find("Portée") != std::string::npos);
}
TEST_CASE("IHM14: Saut de note") {
    MoteurJeu moteur({{60}, {62}});
    InterfacePiano ihm(moteur);
    moteur.passerNote(); 
    CHECK(ihm.getInstruction() == "JOUEZ LA NOTE: 62");
    CHECK(ihm.getCouleur() == "GRIS");
}
TEST_CASE("IHM15: Affichage de la file d'attente") {
    MoteurJeu moteur({{60}, {62}, {64}, {65}, {67}, {69}});
    InterfacePiano ihm(moteur);
    std::string prochaines = ihm.getVisualisationReserve();
    CHECK(prochaines.find("62") != std::string::npos);
    CHECK(prochaines.find("64") != std::string::npos);
}