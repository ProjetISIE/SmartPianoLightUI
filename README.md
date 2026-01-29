---
lang: fr
---

<!--toc:start-->

- [Smart Piano User Interface](#smart-piano-user-interface)
  - [Modes de Jeu](#modes-de-jeu)
    - [Gammes Supportées](#gammes-supportées)
    - [Modes Supportés](#modes-supportés)
  - [Matériel](#matériel)
  - [Dépannage et Résolution des Problèmes](#dépannage-et-résolution-des-problèmes)
  - [Outillage](#outillage)
  - [Compilation & Exécution](#compilation-exécution)
    - [Test Manuel](#test-manuel)
    - [Tests Automatiques](#tests-automatiques)
  - [Conventions de Code](#conventions-de-code)
    - [Documentation Doxygen](#documentation-doxygen)
    - [Nommage des Symboles (casse, tirets)](#nommage-des-symboles-casse-tirets)
    - [Formatage du Code (sauts de ligne, espaces)](#formatage-du-code-sauts-de-ligne-espaces)
    - [Autres](#autres)
  - [Journalisation](#journalisation)
  - [Auteurs & Licence](#auteurs-licence)
  - [Contribution](#contribution)
    - [Ajout d’un Transport](#ajout-dun-transport)
  - [Architecture](#architecture)
    - [Utilitaires](#utilitaires)

<!--toc:end-->

# Smart Piano User Interface

Smart Piano est un système aidant à progresser au piano en s'entrainant à en
jouer d’une manière optimisant l’apprentissage grâce à des exercices
intelligents.

Ceci est la partie interface utilisateur, communicant avec le moteur, développée
avec [raylib]. Elle est spécifiquement développée pour être légère, avoir peu de
dépendances et être aisée à embarquer sur une cible ARM 32 bit (tel qu’une
Raspberry Pi 4).

L’utilisateur interagit via un **clavier MIDI** connecté au dispositif Smart
Piano, sur lequel fonctionne l’application.

L'application propose plusieurs **modes de jeu** (notes, accords…) et évalue la
performance en temps réel pour proposer les exercices les plus propices à faire
progresser.

Ce projet, développé dans le cadre du cursus de [Polytech Tours], vise à être
une plateforme ayant un intérêt **pédagogique** à la fois sur le piano et sur le
développement (`C++`).

## Modes de Jeu

1. Notes `note` : Reconnaissance de notes individuelles
   - Le joueur doit jouer la note affichée

2. Accords `chord` : Accords simples (sans renversement)
   - Le joueur doit jouer les 3 notes de l'accord dans n'importe quel ordre

3. Accords renversés `inversed` : Accords avec renversements
   - Le joueur doit jouer l'accord, mais il est renversé

### Gammes Supportées

- `c` : Do
- `d` : Ré
- `e` : Mi
- `f` : Fa
- `g` : Sol
- `a` : La
- `b` : Si

### Modes Supportés

- `maj` : Majeur
- `min` : Mineur

## Matériel

Smart Piano a été conçu pour fonctionner avec :

- **Raspberry Pi 4**
  - Sous **Raspberry Pi OS**
  - MicroSD de **32 Go**
- **Écran tactile** connecté à la Raspberry Pi (via HDMI)
- **Clavier MIDI** standard (ex. SWISSONIC EasyKeys49)

Il est néanmoins possible que l'application fonctionne sur d'autres systèmes
d’exploitations, architectures ou configurations, sans garantie.

## Dépannage et Résolution des Problèmes

| **Problème**                | **Solution**                                                                        |
| --------------------------- | ----------------------------------------------------------------------------------- |
| Aucune note n'est détectée  | Vérifier que le **clavier MIDI** est bien **branché** et reconnu avec `aconnect -l` |
| Connexion au MDJ impossible | S’assurer que le **moteur de jeu** est bien lancé : `./engine`                      |
| L'application plante        | **Relancer l'application**, voire **redémarrer la Raspberry Pi**                    |

## Outillage

| Fonction                         | Outil                    |
| -------------------------------- | ------------------------ |
| Compilation C++                  | [Clang]                  |
| Système de build                 | [CMake] (+ Ninja)        |
| Dépendances et environnement     | [Nix]                    |
| Versionnage et collaboration     | [Git] hébergé sur GitHub |
| Tests automatisés                | [doctest]                |
| Couverture de code               | [llvm-cov]               |
| Assistance langage C++           | [clangd] (LSP)           |
| Documentation depuis le code     | [Doxygen]                |
| Formatage du C++                 | [clang-format]           |
| Contrôle qualité C++             | [clang-tidy]             |
| Débogage C++                     | [lldb]                   |
| Test manuel communication socket | [socat]                  |
| Éditeur de code                  | [VS Code], [Helix]…      |

## Compilation & Exécution

Ce projet utilise [Nix] pour télécharger les (bonnes versions des) dépendances,
configurer l’environnement, et permettre in-fine d’effectuer des compilations
(croisées) reproductibles. L’environnement [Nix] est défini dans
[`flake.nix`](./flake.nix) et s’active avec la commande `nix flake develop`
(`nix` doit être installé) ou plus simplement via [`direnv`] (qui doit aussi
être installé séparément).

Pour compiler le projet, il est possible (pour tester) d’utiliser [CMake]
(`cmake --build build`) directement depuis un environnement [Nix] activé, mais
la solution préconisée (car reproductible) est d’utiliser `nix build` ; ou
`nix build .#cross` pour compiler en ciblant l’architecture de la Raspberry Pi 4
(ARM64).

L’application peut être lancée avec `./result/bin/main`, ou `./build/main` si
compilé avec [CMake] (ou automatiquement après un build avec
`cmake --build build --target run`).

### Test Manuel

Lancer l’application, visualiser.

### Tests Automatiques

Les tests unitaires et tests d’intégration peuvent être exécutés manuellement
avec la commande `cmake --build build --target tests`.

Ils sont automatiquement exécutés lors des builds avec [Nix].

De plus, il est possible de générer un rapport de couverture de code avec
`cmake --build build --target coverage`.

## Conventions de Code

Norme utilisée du langage C++ la plus récente (stable), `C++23`. Utilisation de
ses fonctionnalités modernes et respect des meilleures pratiques. Par exemple,
`std::println()` est préféré à `std::cout <<` ou `std::cerr <<`.

### Documentation Doxygen

Documentation des attributs, méthodes et fonctions en _français_ (correct),
suivant la syntaxe [Doxygen], comportant au moins un (concis) premier paragraphe
expliquant rapidement la raison d’être de la fonction, ainsi qu’une ligne
`@param` par paramètre, et `@return` si son type de retour n’est pas `void`.

```c++
const std::string message; ///< Message intéressant

/**
 * Ne fais rien, mis à part être une fonction d’exemple
 * @param firstArg Le premier argument
 * @param secondArg Le second argument
 */
void myFunc(uint32_t firstArg, uint16_t secondArg);
```

Ne pas commenter les éléments évidents tels que les getters/setters simples ou
constructeurs/destructeurs simples.

### Nommage des Symboles (casse, tirets)

Avertissements de non-respect de la convention par [clang-tidy] selon les règles
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

| Symbole           | Convention                    |
| ----------------- | ----------------------------- |
| Enum constante    | `UPPER_CASE` (`MY_ENUM`)      |
| Constexpr         | `UPPER_CASE` (`MY_CONSTEXPR`) |
| Constante globale | `UPPER_CASE` (`MY_CONST`)     |
| Classe            | `CamelCase` (`MyClass`)       |
| Struct            | `CamelCase` (`MyStruct`)      |
| Enum              | `CamelCase` (`MyEnum`)        |
| Fonction          | `camelBack` (`MyMethod`)      |
| Fonction globale  | `camelBack` (`MyFunc`)        |
| Variable / Objet  | `camelBack` (`myVar`)         |
| Variable globale  | `camelBack` (`myGlobalVar`)   |
| Paramètre         | `camelBack` (`myParam`)       |
| Espace de nommage | `snake_case` (`my_namespace`) |
| Type C (à éviter) | `snake_case` suivi de `_t`    |

### Formatage du Code (sauts de ligne, espaces)

Formatage automatique avec [clang-format] selon la convention de code de LLVM
(l’organisation derrière [Clang]) avec quelques ajustements pour rendre le code
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

### Autres

Utilisation des entiers de taille strictement définie `uint8_t`, `uint16_t`,
`uint32_t` et `uint64_t` de la bibliothèque `<stdint.h>` au lieu des `char`,
`short`, `int` et `long` variant selon la plateforme ou l’environnement.

- Structure des classes en quatre blocs
  1. Attributs privés
  2. Éventuels attributs publics
  3. Éventuelles méthodes privées
  4. Méthodes publiques
- Utilisation de `this->` pour identifier attributs
  - Pas de préfixe ou postfixe tel que `_`
- Pas d’attributs initialisés avec valeurs littérales dans le constructeur
  - Initialiser à la déclaration, ex. `type name{value};`
- Méthodes de moins de trois lignes dans le `.hpp` uniquement
  - Ex. getters/setters simples, constructeurs/destructeurs simples
- Commentaires et strings concis
  - Pas de verbe ni déterminants si compréhensible sans
  - Pas de points finaux
  - Pas d’espace avant les `:`
- Pas de blocs `{}` pour une seule instruction
- Limiter les lignes vides au sein d’une même méthode
  - Préférer la séparation en blocs logiques avec des commentaires
- Limiter l’imbrication des blocs
  - Préférer les retours anticipés (`early return`)
  - Préférer les ET `&&` aux imbrications `if` multiples
  - Privilégier les `switch case` lorsque possible
- Toujours expliciter strictement le comportement des attributs et des méthodes
  - `const`, `noexcept`, `override`, `final`, `nodiscard`, …

Vérification automatique de nombreuses règles de qualité de code par
[clang-tidy] (définies dans [`.clang-tidy`](./.clang-tidy)).

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

## Journalisation

L’application génère un fichier de journaux (« logs ») qu’il est possible
d’utiliser pour vérifier le bon fonctionnement de Smart Piano ou comprendre
l’origine des erreurs : `smartpiano.ui.log`.

## Auteurs & Licence

- Fankam Jisele
- Fauré Guilhem

> Initiateur du projet : Mahut Vivien

L’entièreté de Smart Piano est sous GNU GPL v3, voir [LICENSE](LICENSE).

## Contribution

Respecter les instructions des sections précédentes pour contribuer, en
particulier celles des [Conventions de Code](#conventions-de-code).

### Ajout d’un Transport

1. Créer une classe héritant de `ITransport`
2. Implémenter les méthodes de communication
3. Injecter dans le `main.cpp`

## Architecture

Voir [PROTOCOL](PROTOCOL.md) pour la spécification complète du protocole de
communication entre le moteur de jeu et l'interface utilisateur.

L'architecture suit le principe **d'inversion de dépendances** avec des
interfaces abstraites permettant de découpler les composants et faciliter les
tests et l'extensibilité.

[`main`](src/main.cpp): Point d'entrée de l'application.

### Utilitaires

[`Logger`](include/Logger.hpp) Système de journalisation thread-safe avec
rotation automatique de fichiers logs (standard et erreurs), formatage avec
timestamps.

[CMake]: https://cmake.org
[Clang]: https://clang.llvm.org
[clangd]: https://clangd.llvm.org
[clang-format]: https://clangd.llvm.org
[clang-tidy]: https://clangd.llvm.org
[C++23]: https://en.wikipedia.org/wiki/C%2B%2B23
[direnv]: https://direnv.net
[`direnv`]: https://direnv.net
[doctest]: https://github.com/doctest/doctest
[Doxygen]: https://www.doxygen.nl
[Doxygen Docs]: https://www.doxygen.nl/manual
[Git]: https://git-scm.com
[Helix]: https://helix-editor.com
[lcov]: https://github.com/linux-test-project/lcov
[lldb]: https://lldb.llvm.org
[llvm-cov]: https://llvm.org/docs/CommandGuide/llvm-cov.html
[Nix]: https://nixos.org
[Polytech Tours]: https://polytech.univ-tours.fr
[raylib]: https://www.raylib.com
[socat]: http://www.dest-unreach.org/socat
[tio]: https://github.com/tio/tio
[VS Code]: https://code.visualstudio.com
