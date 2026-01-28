#include "UdsTransport.hpp"
#include "Logger.hpp"
#include <cstring>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

bool UdsTransport::start() {
    unlink(this->sockPath.c_str()); // Supprimer socket existant s'il existe
    // Créer socket Unix
    this->serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->serverSock < 0) {
        Logger::err("[UdsTransport] Erreur: Impossible de créer le socket");
        return false;
    }
    // Configurer l'adresse du socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, this->sockPath.c_str(), sizeof(addr.sun_path) - 1);
    // Lier le socket
    if (bind(this->serverSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::err("[UdsTransport] Erreur: Impossible de lier le socket");
        close(this->serverSock);
        this->serverSock = -1;
        return false;
    }
    // Écouter les connexions
    if (listen(this->serverSock, 1) < 0) {
        Logger::err(
            "[UdsTransport] Erreur: Impossible de mettre le socket en écoute");
        close(this->serverSock);
        this->serverSock = -1;
        return false;
    }
    Logger::log("[UdsTransport] Serveur démarré sur {}", this->sockPath);
    return true;
}

void UdsTransport::waitForClient() {
    if (this->serverSock < 0) {
        Logger::err("[UdsTransport] Erreur: Serveur non initialisé");
        return;
    }
    this->clientSock = accept(this->serverSock, nullptr, nullptr);
    if (this->clientSock < 0) {
        Logger::err(
            "[UdsTransport] Erreur: Échec de l'acceptation de connexion");
        return;
    }
    Logger::log("[UdsTransport] Client connecté");
}

void UdsTransport::send(const Message& msg) {
    if (this->clientSock < 0) {
        Logger::err("[UdsTransport] Erreur: Aucun client connecté");
        return;
    }

    std::string data = serializeMessage(msg);
    ssize_t sent =
        ::send(this->clientSock, data.c_str(), data.length(), MSG_NOSIGNAL);

    if (sent < 0) {
        Logger::err("[UdsTransport] Erreur: Échec de l'envoi du message");
        return;
    }

    Logger::log("[UdsTransport] Message envoyé: type={}", msg.getType());
}

Message UdsTransport::receive() {
    if (this->clientSock < 0) {
        Logger::err("[UdsTransport] Erreur: Aucun client connecté");
        return Message("error");
    }
    char buffer[4096];
    std::string data;
    // Lire jusqu'à trouver double newline
    while (true) {
        ssize_t received =
            recv(this->clientSock, buffer, sizeof(buffer) - 1, 0);
        if (received < 0) {
            Logger::err("[UdsTransport] Erreur: Échec de réception");
            return Message("error");
        }
        if (received == 0) {
            Logger::err("[UdsTransport] Client déconnecté");
            close(this->clientSock);
            this->clientSock = -1;
            return Message("error");
        }
        buffer[received] = '\0';
        if (std::string(buffer).find("\n") != std::string::npos)
            Logger::log("[UdsTransport] Ligne reçue: {}", buffer);
        data += buffer;
        // Vérifier si message complet (double newline)
        if (data.find("\n\n") != std::string::npos ||
            data.find("\r\n\r\n") != std::string::npos)
            break;
    }
    Logger::log("[UdsTransport] Message reçu");
    return parseMessage(data);
}

void UdsTransport::stop() {
    if (this->clientSock >= 0) {
        shutdown(this->clientSock, SHUT_RDWR);
        close(this->clientSock);
        this->clientSock = -1;
    }
    if (this->serverSock >= 0) {
        shutdown(this->serverSock, SHUT_RDWR);
        close(this->serverSock);
        this->serverSock = -1;
    }
    Logger::log("[UdsTransport] Serveur arrêté");
}

bool UdsTransport::isClientConnected() const { return clientSock >= 0; }

std::string UdsTransport::serializeMessage(const Message& msg) const {
    std::string result = msg.getType() + "\n";
    for (const auto& [key, value] : msg.getFields())
        result += key + "=" + value + "\n";
    return result + "\n";
}

Message UdsTransport::parseMessage(const std::string& data) const {
    std::istringstream stream(data);
    std::string line;

    // Première ligne = type du message
    if (!std::getline(stream, line) || line.empty()) {
        Logger::err("[UdsTransport] Erreur: Type de message manquant");
        return Message("error");
    }

    // Enlever le \r si présent (pour compatibilité Windows)
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::string messageType = line;
    std::map<std::string, std::string> messageFields;

    // Lignes suivantes = champs clé=valeur
    while (std::getline(stream, line)) {
        // Enlever le \r si présent
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Ligne vide = fin du message
        if (line.empty()) break;

        // Parser clé=valeur
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            messageFields[key] = value;
        }
    }
    return Message(messageType, messageFields);
}
