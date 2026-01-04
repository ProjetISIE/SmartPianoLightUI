<!--toc:start-->

- [Smart Piano User Interface](#smart-piano-user-interface)
  - [Matériel](#matériel)
  - [Dépannage et résolution des problèmes](#dépannage-et-résolution-des-problèmes)
  - [Contribution et conventions de code](#contribution-et-conventions-de-code)
    - [Outils](#outils)
    - [Documentation du code](#documentation-du-code)
    - [Formatage du code (sauts de ligne, espaces)](#formatage-du-code-sauts-de-ligne-espaces)
    - [Nommage des symboles (casse, tirets)](#nommage-des-symboles-casse-tirets)
    - [Organisation](#organisation)
    - [Autre](#autre)
  - [Auteurs](#auteurs)

<!--toc:end-->

# Smart Piano User Interface

Smart Piano permet d’apprendre les bases du piano et de perfectionner sa
technique via des exercices "intelligents", s’adaptant aux faiblesses et à la
manière d’apprendre de l’utilisateur.

Ceci est la partie interface utilisateur, communicant avec le moteur. Elle est
spécifiquement développée pour être légère, avoir peu de dépendances et être
aisée à embarquer sur une cible ARM 32 bit (tel qu’une Raspberry Pi 4).

L’utilisateur interagit via un **clavier MIDI** connecté au dispositif Smart
Piano.

L'application propose plusieurs **modes de jeu** (notes, accords…) et évalue la
performance en temps réel pour proposer les exercices les plus propices à faire
progresser.

Ce projet, développé dans le cadre du cursus de Polytech Tours, vise à être une
plateforme ayant un intérêt **pédagogique** à la fois sur le piano et sur le
développement (`C++`).

## Matériel

Smart Piano a été conçu pour fonctionner avec :

- **Raspberry Pi 4**
  - Sous **Raspberry Pi OS**
  - MicroSD de **32 Go minimum**
- **Écran tactile** connecté à la Raspberry Pi (via HDMI)
- **Clavier MIDI** standard (ex. SWISSONIC EasyKeys49)

De plus, la compilation peut requérir :

- **Connexion internet** pour télécharger les dépendances
- **Accès au terminal** pour l'installation et la configuration

Il est néanmoins possible que l'application fonctionne sur d'autres systèmes
d’exploitations ou architectures, sans garantie.

## Dépannage et résolution des problèmes

| **Problème**                    | **Solution**                                                                    |
| ------------------------------- | ------------------------------------------------------------------------------- |
| **Aucune note n'est détectée**  | Vérifier que le **clavier MIDI est bien branché** et reconnu avec `aconnect -l` |
| **Connexion au MDJ impossible** | S’assurer que le **Moteur de Jeu (MDJ)** est bien lancé : `./PianoTrainerMDJV1` |
| **L'application plante**        | **Relancer l'application**, voire **redémarrer la Raspberry Pi**                |

## Contribution et conventions de code

Ce projet utilise [Nix](https://nixos.org) pour télécharger les (bonnes versions
des) dépendances, configurer l’environnement, et permettre in-fine d’effectuer
des compilations (croisées) reproductibles. L’environnement Nix est défini dans
[`flake.nix`](./flake.nix) et s’active avec la commande `nix flake develop`
(`nix` doit être installé) ou plus simplement via [`direnv`](https://direnv.net)
(qui doit aussi être installé séparément).

Pour compiler le projet, il est possible (pour tester) d’utiliser
[CMake](https://cmake.org) (`cmake` puis `cmake --build`) directement depuis un
environnement Nix activé, mais la solution préconisée (car reproductible) est
d’utiliser `nix build` ; ou `nix build .#cross` pour compiler en ciblant
l’architecture de la Raspberry Pi 4 (ARM64).

### Outils

| Fonction                     | Outil                                         |
| ---------------------------- | --------------------------------------------- |
| Compilation C++              | [Clang](https://clang.llvm.org)               |
| Système de build             | [CMake](https://cmake.org) (+ Ninja)          |
| Dépendances et environnement | [Nix](https://nixos.org)                      |
| Versionnage et collaboration | [Git](https://git-scm.com) avec GitHub        |
| Tests unitaires              | [doctest](https://github.com/doctest/doctest) |
| Assistance langage C++       | [clangd](https://clangd.llvm.org) (LSP)       |
| Documentation depuis le code | [Doxygen](https://www.doxygen.nl)             |
| Formatage du C++             | [clang-format](https://clangd.llvm.org)       |
| Contrôle qualité C++         | [clang-tidy](https://clangd.llvm.org)         |
| Débogage C++                 | [lldb](https://lldb.llvm.org)                 |

### Documentation du code

Documentation de toutes les méthodes / fonctions en français, suivant la syntaxe
[Doxygen](https://www.doxygen.nl/manual), comportant au moins un (court) premier
paragraphe expliquant rapidement la raison d’être de la fonction, ainsi qu’une
ligne `@param` par paramètre, et `@return` si son type de retour n’est pas
`void`.

```c++
/**
 * Ne fais rien, mis à part être une fonction d’exemple
 * @param firstArg Le premier argument
 * @param secondArg Le second argument
 */
void myFunc(uint32_t firstArg, uint16_t secondArg);
```

### Formatage du code (sauts de ligne, espaces)

Formatage automatique avec clang-format selon la convention de code de LLVM
(l’organisation derrière Clang) avec quelques ajustements pour rendre le code
plus compact (définis dans le fichier [`.clang-format`](./.clang-format)).

```yaml
BasedOnStyle: LLVM # Se baser sur le style « officiel » de Clang
IndentWidth: 4 # Indenter fortement pour décourager trop de sous-imbrication
---
Language: Cpp # Règles spécifiques au C++ (le projet pourrait utiliser plusieurs langages)
AllowShortBlocksOnASingleLine: Empty # Code plus compact, ex. {}
AllowShortCaseExpressionOnASingleLine: true # Code plus compact, ex. switch(a) { case 1: … }
AllowShortCaseLabelsOnASingleLine: true # Code plus compact, ex. case 1: …
AllowShortIfStatementsOnASingleLine: AllIfsAndElse # Code plus compact, ex. if (a) { … } else { … }
AllowShortLoopsOnASingleLine: true # Code plus compact, ex. for (…) { … }
DerivePointerAlignment: false # Tout le temps…
PointerAlignment: Left # afficher le marquer de pointeur * collé au type
```

### Nommage des symboles (casse, tirets)

Avertissements de non-respect de la convention par clang-tidy selon les règles
de nommage définies dans le fichier [`.clang-tidy`](./.clang-tidy).

```yaml
CheckOptions.readability-identifier-naming:
  EnumConstantCase: UPPER_CASE
  ConstexprVariableCase: UPPER_CASE
  GlobalConstantCase: UPPER_CASE
  ClassCase: CamelCase
  StructCase: CamelCase
  EnumCase: CamelCase
  FunctionCase: camelBack
  GlobalFunctionCase: camelBack
  VariableCase: camelBack
  GlobalVariableCase: camelBack
  ParameterCase: camelBack
  NamespaceCase: lower_case
```

| Symbole            | Convention                    |
| ------------------ | ----------------------------- |
| Enum constante     | `UPPER_CASE` (`MY_ENUM`)      |
| Constexpr          | `UPPER_CASE` (`MY_CONSTEXPR`) |
| Constante globale  | `UPPER_CASE` (`MY_CONST`)     |
| Classe             | `CamelCase` (`MyClass`)       |
| Struct             | `CamelCase` (`MyStruct`)      |
| Enum               | `CamelCase` (`MyEnum`)        |
| Fonction           | `camelBack` (`MyMethod`)      |
| Fonction globale   | `camelBack` (`MyFunc`)        |
| Variable / Objet   | `camelBack` (`myVar`)         |
| Variable globale   | `camelBack` (`myGlobalVar`)   |
| Paramètre          | `camelBack` (`myParam`)       |
| Espace de nommage  | `snake_case` (`my_namespace`) |
| Définition de type | `snake_case` suivi de `_t`    |

### Organisation

| Fichier      | Contenu                                              |
| ------------ | ---------------------------------------------------- |
| `src/main.c` | Initialisation, boucle principale, fin et nettoyages |
| `src/*`      | À compléter…                                         |

### Autre

Utilisation des entiers de taille strictement définie `uint8_t`, `uint16_t`,
`uint32_t` et `uint64_t` de la bibliothèque `<stdint.h>` au lieu des `char`,
`short`, `int` et `long` variant selon la plateforme ou l’environnement.

Vérification automatique de nombreuses règles de qualité de code par clang-tidy
(définies dans [`.clang-tidy`](./.clang-tidy)).

```yaml
Checks: >
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  clang-diagnostic-*,
  concurrency-*,
  modernize-*,
  performance-*,
  readability-*,
```

Norme utilisée du langage C++ la plus récente (stable),
[C++23](https://en.wikipedia.org/wiki/C%2B%2B23).

## Auteurs

Fankam Jisele, Fauré Guilhem
