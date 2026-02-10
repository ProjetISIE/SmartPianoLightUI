#include "Communication.hpp"
#include "Logger.hpp"
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

std::string serialize(const Message& msg) {
    std::stringstream ss;
    ss << msg.getType() << "\n";
    for (const auto& field : msg.getFields())
        ss << field.first << "=" << field.second << "\n";
    ss << "\n";
    return ss.str();
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

Communication::Communication(std::string path) : socket_path(std::move(path)) {}

Communication::~Communication() { disconnect(); }

bool Communication::connect() {
    if (isConnected()) {
        Logger::log("[Comm] Already connected.");
        return true;
    }
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        Logger::log("[Comm] Failed to create socket.");
        return false;
    }
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path.c_str(),
            sizeof(server_addr.sun_path) - 1);
    if (::connect(sock_fd, (struct sockaddr*)&server_addr,
                  sizeof(server_addr)) < 0) {
        Logger::log("[Comm] Failed to connect to socket: {}", socket_path);
        close(sock_fd);
        sock_fd = -1;
        return false;
    }
    Logger::log("[Comm] Connected to {}", socket_path);
    running = true;
    listener_thread = std::thread(&Communication::listen, this);
    return true;
}

void Communication::disconnect() {
    if (!isConnected()) return;
    running = false;
    if (sock_fd != -1) {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        sock_fd = -1;
        Logger::log("[Comm] Disconnected.");
    }
    if (listener_thread.joinable()) listener_thread.join();
}

bool Communication::isConnected() const { return sock_fd != -1 && running; }

void Communication::send(const Message& msg) {
    if (!isConnected()) {
        Logger::log("[Comm] Not connected. Cannot send message.");
        return;
    }
    std::string data = serialize(msg);
    if (::write(sock_fd, data.c_str(), data.length()) < 0) {
        Logger::log("[Comm] Failed to write to socket.");
        disconnect();
    } else Logger::log("[Comm] Sent: {}", msg.getType());
}

std::optional<Message> Communication::popMessage() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (message_queue.empty()) return std::nullopt;
    Message msg = message_queue.front();
    message_queue.pop();
    return msg;
}

void Communication::listen() {
    char buffer[4096];
    std::string current_message_data;
    while (running) {
        ssize_t bytes_read = ::read(sock_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            current_message_data += buffer;
            size_t end_of_message_pos;
            while ((end_of_message_pos = current_message_data.find("\n\n")) !=
                   std::string::npos) {
                std::string msg_data =
                    current_message_data.substr(0, end_of_message_pos);
                Message msg = deserialize(msg_data);
                std::lock_guard<std::mutex> lock(queue_mutex);
                message_queue.push(msg);
                Logger::log("[Comm] Received: {}", msg.getType());
                current_message_data.erase(0, end_of_message_pos + 2);
            }
        } else if (bytes_read == 0) {
            Logger::log("[Comm] Server closed connection.");
            running = false; // Stop
        } else {
            if (running) // Avoid error message on intentional disconnect
                Logger::log("[Comm] Failed to read from socket.");
            running = false; // Stop
        }
    }
    // Déconnecte si erreur ou fermeteur connection serveur
    if (sock_fd != -1) disconnect();
}
