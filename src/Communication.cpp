#include "Communication.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <format>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

std::string serialize(const Message& msg) {
    std::string result = std::format("{}\n", msg.getType());
    for (const auto& [key, value] : msg.getFields()) {
        result += std::format("{}={}\n", key, value);
    }
    result += "\n";
    return result;
}

Message deserialize(std::string_view data) {
    size_t pos = data.find('\n');
    if (pos == std::string_view::npos) {
        return Message(std::string(data));
    }
    std::string type(data.substr(0, pos));
    data.remove_prefix(pos + 1);

    std::map<std::string, std::string> fields;
    while (!data.empty()) {
        pos = data.find('\n');
        std::string_view line =
            (pos == std::string_view::npos) ? data : data.substr(0, pos);
        if (line.empty()) {
            break;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string_view::npos) {
            std::string_view key = line.substr(0, eqPos);
            std::string_view value = line.substr(eqPos + 1);
            fields.emplace(key, value);
        }

        if (pos == std::string_view::npos) {
            break;
        }
        data.remove_prefix(pos + 1);
    }
    return Message(std::move(type), std::move(fields));
}

Communication::Communication(std::string path) : socketPath_(std::move(path)) {}

Communication::~Communication() { this->disconnect(); }

bool Communication::connect() {
    if (this->isConnected()) {
        Logger::log("[Comm] Déjà connecté");
        return true;
    }

    this->sockFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sockFd_ < 0) {
        Logger::log("[Comm] Échec création socket");
        return false;
    }

    struct sockaddr_un serverAddr{};
    serverAddr.sun_family = AF_UNIX;

    if (this->socketPath_.length() >= sizeof(serverAddr.sun_path)) {
        Logger::err("[Comm] Chemin du socket trop long : {}",
                    this->socketPath_);
        close(this->sockFd_);
        this->sockFd_ = -1;
        return false;
    }

    std::copy_n(this->socketPath_.begin(), this->socketPath_.length(),
                serverAddr.sun_path);
    serverAddr.sun_path[this->socketPath_.length()] = '\0';

    if (::connect(this->sockFd_, (struct sockaddr*)&serverAddr,
                  sizeof(serverAddr)) < 0) {
        Logger::log("[Comm] Échec connexion socket: {}", this->socketPath_);
        close(this->sockFd_);
        this->sockFd_ = -1;
        return false;
    }

    Logger::log("[Comm] Connecté à {}", this->socketPath_);
    this->running_ = true;
    this->listenerThread_ = std::thread(&Communication::listen, this);

    return true;
}

void Communication::disconnect() {
    if (this->sockFd_ == -1 && !this->listenerThread_.joinable()) return;

    this->running_ = false;
    if (this->sockFd_ != -1) {
        shutdown(this->sockFd_, SHUT_RDWR);
        close(this->sockFd_);
        this->sockFd_ = -1;
        Logger::log("[Comm] Déconnecté");
    }

    if (this->listenerThread_.joinable() &&
        this->listenerThread_.get_id() != std::this_thread::get_id()) {
        this->listenerThread_.join();
    }
}

bool Communication::isConnected() const noexcept {
    return this->sockFd_ != -1 && this->running_;
}

void Communication::send(const Message& msg) {
    if (!this->isConnected()) {
        Logger::log("[Comm] Non connecté. Ne peut envoyer de message.");
        return;
    }
    std::string data = serialize(msg);
    if (::write(this->sockFd_, data.c_str(), data.length()) < 0) {
        Logger::log("[Comm] Échec écriture socket.");
        this->disconnect();
    } else {
        Logger::debug("[Comm] Envoyé: {}", msg.getType());
    }
}

std::optional<Message> Communication::popMessage() {
    std::lock_guard<std::mutex> lock(this->queueMutex_);
    if (this->messageQueue_.empty()) return std::nullopt;
    Message msg = std::move(this->messageQueue_.front());
    this->messageQueue_.pop();
    return msg;
}

void Communication::clearQueue() {
    std::lock_guard<std::mutex> lock(this->queueMutex_);
    while (!this->messageQueue_.empty()) {
        this->messageQueue_.pop();
    }
}

void Communication::listen() {
    char buffer[4096];
    std::string currentMessageData;

    while (this->running_) {
        ssize_t bytesRead = ::read(this->sockFd_, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            currentMessageData.append(buffer, static_cast<size_t>(bytesRead));

            size_t endOfMessagePos;
            while ((endOfMessagePos = currentMessageData.find("\n\n")) !=
                   std::string::npos) {
                std::string_view msgData = std::string_view(currentMessageData)
                                               .substr(0, endOfMessagePos);
                Message msg = deserialize(msgData);

                {
                    std::lock_guard<std::mutex> lock(this->queueMutex_);
                    this->messageQueue_.push(std::move(msg));
                }
                Logger::debug("[Comm] Reçu: {}", msg.getType());

                currentMessageData.erase(0, endOfMessagePos + 2);
            }
        } else if (bytesRead == 0) {
            Logger::log("[Comm] Le serveur a fermé la connexion");
            this->running_ = false;
        } else {
            if (this->running_) Logger::log("[Comm] Échec lecture socket");
            this->running_ = false;
        }
    }
}
