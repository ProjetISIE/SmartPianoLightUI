#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include "Message.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

/**
 * @brief Sérialise un message en chaîne selon le protocole
 * @param msg Message à sérialiser
 * @return Chaîne sérialisée, prête à être envoyée via le socket
 */
[[nodiscard]] std::string serialize(const Message& msg);

/**
 * @brief Désérialise une chaîne de caractères en message
 * @param data Données brutes reçues du socket
 * @return Message parsé
 */
[[nodiscard]] Message deserialize(const std::string& data);

/**
 * @brief Gère la communication client avec le moteur de jeu via Unix Domain
 * Socket
 *
 * Encapsule la logique de connexion, d'envoi et de réception de messages
 * dans un thread dédié pour ne pas bloquer l'interface utilisateur.
 */
class Communication {
  private:
    std::string socketPath; ///< Chemin du socket Unix
    int32_t sockFd{-1};     ///< Descripteur de fichier du socket
    std::atomic<bool> running{
        false};                 ///< Indique si le thread d'écoute doit tourner
    std::thread listenerThread; ///< Thread qui écoute les messages entrants

    std::queue<Message>
        messageQueue; ///< File d'attente thread-safe pour les messages reçus
    std::mutex queueMutex; ///< Mutex pour protéger l'accès à la file

  private:
    /**
     * @brief Boucle principale du thread d'écoute
     *
     * Lit en continu sur le socket, désérialise les messages et les place
     * dans la file d'attente.
     */
    void listen();

  public:
    /**
     * @brief Construit un gestionnaire de communication
     * @param sockPath Chemin vers le fichier de socket Unix du moteur
     */
    explicit Communication(std::string sockPath = "/tmp/smartpiano.sock");

    /**
     * @brief Destructeur, assure une déconnexion propre
     */
    ~Communication();

    /**
     * @brief Tente de se connecter au socket du moteur
     * @return `true` en cas de succès, `false` sinon
     */
    [[nodiscard]] bool connect();

    /**
     * @brief Se déconnecte du socket et arrête le thread d'écoute
     */
    void disconnect();

    /**
     * @brief Vérifie l'état de la connexion
     * @return `true` si le client est connecté, `false` sinon
     */
    [[nodiscard]] bool isConnected() const noexcept;

    /**
     * @brief Envoie un message sérialisé au moteur
     * @param msg Le message à envoyer
     */
    void send(const Message& msg);

    /**
     * @brief Dépile le plus ancien message reçu de la file d'attente
     * @return Un `std::optional<Message>` contenant le message, ou
     * `std::nullopt` si la file est vide
     */
    [[nodiscard]] std::optional<Message> popMessage();
};

#endif // COMMUNICATION_HPP
