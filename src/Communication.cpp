#include "Communication.hpp"
#include "Logger.hpp"
#include <format>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

std::string serialize(const Message& msg) {
    std::string result = std::format("{}\n", msg.getType());
    for (const auto& field : msg.getFields())
        result += std::format("{}={}\n", field.first, field.second);
    result += "\n";
    return result;
}

Message deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string line;
    std::getline(ss, line);
    std::string type = line;
    std::map<std::string, std::string> fields;
    while (std::getline(ss, line) && !line.empty()) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            fields[key] = value;
        }
    }
    return Message(type, fields);
}

Communication::Communication(std::string path) : socketPath(std::move(path)) {}

Communication::~Communication() { this->disconnect(); }

bool Communication::connect() {
    if (this->isConnected()) {
        Logger::log("[Comm] Déjà connecté");
        return true;
    }

    this->sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sockFd < 0) {
        Logger::log("[Comm] Échec création socket");
        return false;
    }

    struct sockaddr_un serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, this->socketPath.c_str(),
            sizeof(serverAddr.sun_path) - 1);

    if (::connect(this->sockFd, (struct sockaddr*)&serverAddr,
                  sizeof(serverAddr)) < 0) {
        Logger::log("[Comm] Échec connexion socket: {}", this->socketPath);
        close(this->sockFd);
        this->sockFd = -1;
        return false;
    }

    Logger::log("[Comm] Connecté à {}", this->socketPath);
    this->running = true;
    this->listenerThread = std::thread(&Communication::listen, this);

    return true;
}

void Communication::disconnect() {
    if (!this->isConnected()) return;

    this->running = false;
    if (this->sockFd != -1) {
        shutdown(this->sockFd, SHUT_RDWR);
        close(this->sockFd);
        this->sockFd = -1;
        Logger::log("[Comm] Déconnecté");
    }

    if (this->listenerThread.joinable()) this->listenerThread.join();
}

bool Communication::isConnected() const {
    return this->sockFd != -1 && this->running;
}

void Communication::send(const Message& msg) {
    if (!this->isConnected()) {
        Logger::log("[Comm] Non connecté. Ne peut envoyer de message.");
        return;
    }
    std::string data = serialize(msg);
    if (::write(this->sockFd, data.c_str(), data.length()) < 0) {
        Logger::log("[Comm] Échec écriture socket.");
        this->disconnect();
    } else Logger::log("[Comm] Envoyé: {}", msg.getType());
}

std::optional<Message> Communication::popMessage() {
    std::lock_guard<std::mutex> lock(this->queueMutex);
    if (this->messageQueue.empty()) return std::nullopt;
    Message msg = this->messageQueue.front();
    this->messageQueue.pop();
    return msg;
}

void Communication::listen() {
    char buffer[4096];
    std::string currentMessageData;

    while (this->running) {
        ssize_t bytesRead = ::read(this->sockFd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            currentMessageData += buffer;

            size_t endOfMessagePos;
            while ((endOfMessagePos = currentMessageData.find("\n\n")) !=
                   std::string::npos) {
                std::string msgData =
                    currentMessageData.substr(0, endOfMessagePos);
                Message msg = deserialize(msgData);

                {
                    std::lock_guard<std::mutex> lock(this->queueMutex);
                    this->messageQueue.push(msg);
                }
                Logger::log("[Comm] Reçu: {}", msg.getType());

                currentMessageData.erase(0, endOfMessagePos + 2);
            }
        } else if (bytesRead == 0) {
            Logger::log("[Comm] Le serveur a fermé la connexion");
            this->running = false;
        } else {
            if (this->running) Logger::log("[Comm] Échec lecture socket");
            this->running = false;
        }
    }

    if (this->sockFd != -1) this->disconnect();
}
