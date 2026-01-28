---
lang: fr
---

<!--toc:start-->

- [Protocole Smart Piano](#protocole-smart-piano)
  - [Format des messages](#format-des-messages)
  - [Types de messages](#types-de-messages)
    - [1. Client (interface utilisateur) → Serveur (moteur de jeu)](#1-client-interface-utilisateur-serveur-moteur-de-jeu)
      - [1.1 Configuration de jeu `config`](#11-configuration-de-jeu-config)
      - [1.2 Prêt pour le challenge suivant `ready`](#12-prêt-pour-le-challenge-suivant-ready)
      - [1.3 Abandon `quit`](#13-abandon-quit)
    - [2. Serveur (moteur de jeu) → Client (interface utilisateur)](#2-serveur-moteur-de-jeu-client-interface-utilisateur)
      - [2.1 Accusé de réception (de configuration) `ack`](#21-accusé-de-réception-de-configuration-ack)
      - [2.2 Challenge note `note`](#22-challenge-note-note)
      - [2.3 Challenge accord `chord`](#23-challenge-accord-chord)
      - [2.4 Résultat du challenge `result`](#24-résultat-du-challenge-result)
      - [2.5 Fin de partie `over`](#25-fin-de-partie-over)
      - [2.6 Erreur `error`](#26-erreur-error)
  - [Diagramme de séquence](#diagramme-de-séquence)
    - [Session complète](#session-complète)
    - [Gestion d'erreur](#gestion-derreur)
  - [États de la connexion](#états-de-la-connexion)
  - [Transitions d'états](#transitions-détats)
  - [Règles de validation](#règles-de-validation)
  - [Gestion des erreurs de protocole](#gestion-des-erreurs-de-protocole)
  - [Exemple de Jeu de notes réussi](#exemple-de-jeu-de-notes-réussi)
  - [Exemple de Jeu d'accords avec erreur](#exemple-de-jeu-daccords-avec-erreur)

<!--toc:end-->

# Protocole Smart Piano

> Version: 0.0.0

Smart Piano utilise un protocole texte simple sur Unix Domain Socket (UDS) pour
la communication entre le moteur de jeu (serveur) et l'interface utilisateur
(client).

- **Type** : Unix Domain Socket (UDS)
- **Chemin par défaut** : `/tmp/smartpiano.sock`
- **Mode** : SOCK_STREAM (orienté connexion)
- **Encodage** : UTF-8

## Format des messages

Chaque message débute par son type, comporte d’éventuelles lignes de texte au
format `clé=valeur`, et se termine par une ligne vide (`\n\n`).

```
type du message
clé1=valeur1
clé2=valeur2
# ligne vide ici <--
```

- Le type de message est en minuscules ASCII, ne contient ni espaces, ni `\n`
- Le type du message se termine par `\n` (LF uniquement)
- Les éventuelles lignes subséquentes contiennent une (seule) paire `clé=valeur`
- Les clés contiennent uniquement des minuscules ASCII (non accentuées) et `_`
- Clé et valeur ne peuvent pas contenir `=` ou `\n`
- Clé et valeur sont séparés uniquement par `=` (qui termine la clé)
- La valeur peut contenir n’importe quel caractère UTF-8, sauf `\n`, qui est le
  seul caractère à la terminer
  - Dans les faits, les valeurs (actuellement) acceptées sont la plupart du
    temps un mot en minuscules, avec parfois des chiffres
- Les lignes se terminent par `\n` (LF uniquement)
- Le message se termine par une ligne ne contenant pas de paire `clé=valeur`
  - Généralement vide, donc `\n\n`
- Encodage UTF-8 ; Caractères accentués, spéciaux… supportés dans les valeurs
- Un message sans champ contient le type du message suivi de `\n\n`

**Exemple** :

```
config
game=note
scale=c
mode=maj
# ligne vide indiquant la fin du message <--
```

Ci-dessous, la fin d’un bloc de code est considérée comme une ligne vide.

## Types de messages

### 1. Client (interface utilisateur) → Serveur (moteur de jeu)

#### 1.1 Configuration de jeu `config`

Commence une nouvelle session de jeu en la configurant.

```
config
game=<TYPE>
scale=<GAMME>
mode=<MODE>
```

**Champs** :

- `game` : Type de jeu
  - `note` : Jeu de reconnaissance de notes
  - `chord` : Jeu d'accords sans renversement
  - `inversed` : Jeu d'accords avec renversements
- `scale` : Éventuelle gamme musicale voulue (sinon le serveur choisit)
  - Valeurs : `c`, `d`, `e`, `f`, `g`, `a`, `b`
  - Cela équivaut en français à "Do", "Ré", "Mi", "Fa", "Sol", "La", "Si"
- `mode` : Éventuel mode de la gamme voulu (sinon le serveur choisit)
  - Valeurs : `maj` pour Majeur, `min` pour mineur

**Exemple** :

```
config
game=note
scale=c
mode=maj
```

#### 1.2 Prêt pour le challenge suivant `ready`

Indique que le client est prêt à recevoir le prochain challenge, à poursuivre le
jeu en cours.

```
ready
```

Aucun **champ**, seulement le type valant `ready`, suivi d’une fin de message.

#### 1.3 Abandon `quit`

Demande l'arrêt de la session : retourne dans l’état non configuré.

```
quit
```

Aucun **champ**, uniquement le type valant `quit`, suivi d’une fin de message.

### 2. Serveur (moteur de jeu) → Client (interface utilisateur)

#### 2.1 Accusé de réception (de configuration) `ack`

Confirme la réception de la configuration.

```
ack
status=ok
```

**Exemple en cas d'erreur** :

```
ack
status=error
code=<CODE>
message=<MESSAGE>
```

**Champs** :

- `status` : `ok` ou `error`
- `code` : Code d'erreur éventuel
  - `game` : Type de jeu invalide
  - `scale` : Gamme invalide
  - `mode` : Mode invalide
  - `midi` : Périphérique MIDI non disponible
- `message` : Éventuel message d'erreur descriptif

#### 2.2 Challenge note `note`

Demande au joueur de jouer une note spécifique.

```
note
note=<NOTE>
id=<ID>
```

**Champs** :

- `note` : Note à jouer (ex : `c4`, `d#5`, `gb3`)
- `id` : Identifiant unique du challenge (entier positif)

**Exemple** :

```
note
note=c4
id=1
```

#### 2.3 Challenge accord `chord`

Demande au joueur de jouer un accord spécifique.

```
chord
name=<NOM>
notes=<NOTES>
id=<ID>
```

**Champs** :

- `name` : Nom (affiché) de l'accord (ex : `Do majeur`, `Re mineur`)
  - Commence par le nom de la fondamentale en notation syllabique
  - Suivi de l’adjectif `majeur` ou `mineur`
  - Éventuellement suivi par un chiffre arabe indiquant le renversement
- `notes` : Notes de l'accord (ex : `c4 e4 g4`)
  - Une note commence par une lettre minuscule entre `a` et `g`
  - Suivie éventuellement d’un dièse (`#`) ou bémol (`b`)
  - Terminé par un chiffre d’octave de 0 à 8 (ex : `4`)
  - Les notes sont séparées par des espaces
- `id` : Identifiant unique du challenge

**Exemple (position fondamentale)** :

```
chord
name=Do majeur
notes=c4 e4 g4
id=5
```

**Exemple (premier renversement)** :

```
chord
name=Do majeur 1
notes=e4 g4 c5
id=6
```

#### 2.4 Résultat du challenge `result`

Informe le joueur du résultat de sa tentative, clôturant le challenge.

```
result
id=<ID>
correct=<NOTES>
incorrect=<NOTES>
duration=<MS>
```

**Champs** :

- `id` : Identifiant du challenge correspondant
- `correct` : Éventuelles notes (séparées par des espaces) jouées a raison
- `incorrect` : Éventuelles notes jouées alors que non attendues
- `duration` : Éventuelle durée en millisecondes entre l’envoi du challenge et
  la réception des notes jouées

Un message de résultat contient au moins une note, qu’elle soit correcte ou
incorrecte.

**Exemple (correct)** :

```
result
id=1
correct=c4
```

**Exemple (incorrect)** :

```
result
id=2
incorrect=d4
```

#### 2.5 Fin de partie `over`

Indique la fin de la session de jeu avec un récapitulatif.

```
over
duration=<DURATION>
perfect=<CORRECT>
partial=<PARTIAL>
total=<TOTAL>
```

**Champs** :

- `duration` : Durée depuis le dernier message `config` en millisecondes
- `perfect` : Éventuel nombre de challenges sans notes `incorrect`
- `partial` : Éventuel nombre de challenges avec au moins une note `correct` et
  une note `incorrect`
- `total` : Nombre total de challenges joués

**Exemple** :

```
over
duration=45
perfect=10
total=10
```

#### 2.6 Erreur `error`

Signale une erreur détectée par le serveur.

```
error
code=<CODE>
message=<MESSAGE>
```

**Champs** :

- `code` : Code d'erreur
  - `internal` : Erreur interne du serveur
  - `protocol` : Erreur de protocole (message mal formé)
  - `state` : État invalide (ex : ready sans config)
  - `midi` : Erreur MIDI
- `message` : Message d'erreur descriptif

Smart Piano essaie d’être tolérant aux erreurs. Seule l’erreur interne engendre
un retour en état sûr. Les autres erreurs engendrent simplement un avertissement
du client (qui avertit l’utilisateur si besoin) et un maintien de l’état actuel.

Par exemple, en cas de périphérique MIDI indisponible, le serveur envoie une
erreur pour indiquer à l’utilisateur de reconnecter son piano, mais la session
de jeu peut continuer une fois le piano reconnecté.

**Exemple** :

```
error
code=protocol
message=Message mal formé: champ 'id' manquant
```

## Diagramme de séquence

### Session complète

```
Client                       Serveur
  |                             |
  |--- config ----------------->|
  |                             |
  |<-- ack (ok) ----------------|
  |                             |
  |--- ready ------------------>|
  |                             |
  |<-- note --------------------|
  |                             |
  | [Joueur joue une note]      |
  |                             |
  |<-- result ------------------|
  |                             |
  |--- ready ------------------>|
  |                             |
  |<-- note --------------------|
  |                             |
  | [Répéter pour N challenges] |
  |                             |
  |<-- over --------------------|
  |                             |
```

### Gestion d'erreur

```
Client           Serveur
  |                 |
  |--- config ----->|
  |    (invalide)   |
  |                 |
  |<-- ack (error) -|
  |                 |
  |--- config ----->|
  |   (valide)      |
  |                 |
  |<-- ack (ok) ----|
  |                 |
```

## États de la connexion

1. **DISCONNECTED** : Aucune connexion établie, état de départ
2. **CONNECTED** : Client connecté, mais pas de configuration
3. **CONFIGURED** : Configuration reçue et validée, prêt à lancer le jeu
4. **PLAYING** : Challenge émis, jeu en cours, attente d’une action du joueur
5. **PLAYED** : Note(s) jouée(s) (MIDI), prêt pour jouer à nouveau

## Transitions d'états

```
DISCONNECTED --[client connect]--> CONNECTED
CONNECTED --[config + ack]--> CONFIGURED
CONFIGURED --[ready + challenge]--> PLAYING
PLAYING --[note(s) played]--> PLAYED
PLAYED --[ready]--> PLAYING
PLAYING --[last challenge done + result]--> CONFIGURED
CONFIGURED --[quit / internal error]--> CONNECTED
PLAYING --[quit / internal error]--> CONNECTED
PLAYED --[quit / internal error]--> CONNECTED
```

## Règles de validation

1. Tous les champs obligatoires (non "éventuels") doivent être présents
2. Les valeurs des champs doivent être dans les ensembles autorisés
3. Le message doit se terminer par `\n\n`
4. Timeout de réception après 90 secondes
   - Éviter les connexions zombies, mais laisser le temps de jouer

Le serveur doit aussi s’assurer que le challenge `id` est unique et croissant,
réinitialisé à chaque nouvelle configuration.

## Gestion des erreurs de protocole

En cas d'erreur de protocole (message mal formé, champ manquant, valeur
invalide), le serveur doit :

1. Journaliser (log) l'erreur, comme n’importe quel échange (pour debug)
2. Envoyer un message `error` avec le code et message appropriés
3. Fermer la connexion si l'erreur est critique
4. Continuer la session si l'erreur est récupérable

## Exemple de Jeu de notes réussi

```
→ config
→ game=note
→ scale=c
→ mode=maj
→

← ack
← status=ok
←

→ ready
→

← note
← note=c4
← id=1
←

[Joueur joue C4]

← result
← id=1
← correct=c4
←

→ ready
→

← note
← note=e4
← id=2
←

[Joueur joue E4]

← result
← id=2
← correct=e4
←

[... après 10 challenges ...]

← over
← duration=32
← perfect=10
← total=10
←
```

## Exemple de Jeu d'accords avec erreur

```
→ config
→ game=inversed
→ scale=c
→ mode=maj
→

← ack
← status=ok
←

→ ready
→

← chord
← name=Do majeur
← notes=c4 e4 g4
← id=1
←

[Joueur joue C4 E4 F4 - erreur!]

← result
← id=1
← correct=c4 e4
← incorrect=f4
←

→ ready
→

← chord
← name=Do majeur
← notes=c4 e4 g4
← id=2
←

[Joueur joue C4 E4 G4 - correct!]

← result
← id=2
← correct=c4 e4 g4
←

...
```
