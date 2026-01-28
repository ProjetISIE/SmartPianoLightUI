#ifndef UDSTRANSPORT_HPP
#define UDSTRANSPORT_HPP

#include "ITransport.hpp"
#include "Logger.hpp"
#include <string>

/**
 * @brief Implémentation de la communication UI/moteur via Unix Domain Socket
 * Gère la communication via socket Unix en respectant le protocole défini
 */
class UdsTransport : public ITransport {
  private:
    const std::string sockPath; ///< Chemin socket Unix
    int serverSock{-1};         ///< Descripteur socket serveur
    int clientSock{-1};         ///< Descripteur socket client

  private:
    /**
     * @brief Sérialise un message en chaîne selon le protocole
     * @param msg Message à sérialiser
     * @return Chaîne sérialisée
     */
    std::string serializeMessage(const Message& msg) const;

    /**
     * @brief Parse une chaîne en message selon le protocole
     * @param data Données à parser
     * @return Message parsé
     */
    Message parseMessage(const std::string& data) const;

  public:
    explicit UdsTransport(std::string path = "/tmp/smartpiano.sock")
        : sockPath(std::move(path)) {
        Logger::log("[UdsTransport] Instance créée sur {}", sockPath);
    }

    ~UdsTransport() {
        stop();
        Logger::log("[UdsTransport] Instance détruite");
    }

    /**
     * @brief Démarre le serveur UDS
     * @return true si démarrage réussi
     */
    bool start() override;

    /**
     * @brief Attend la connexion d'un client (bloquant)
     */
    void waitForClient() override;

    /**
     * @brief Envoie un message au client
     * @param msg Message à envoyer
     */
    void send(const Message& msg) override;

    /**
     * @brief Reçoit un message du client (bloquant)
     * @return Message reçu
     */
    Message receive() override;

    /**
     * @brief Arrête le serveur
     */
    void stop() override;

    /**
     * @brief Vérifie si un client est connecté
     * @return true si un client est connecté
     */
    bool isClientConnected() const override;

    /**
     * @brief Obtient le chemin de la socket Unix
     * @return Chemin de la socket (string)
     */
    std::string getSocketPath() const override { return this->sockPath; }
};

#endif // UDSTRANSPORT_HPP
