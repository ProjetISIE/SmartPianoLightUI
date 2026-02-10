#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

struct Message {
  private:
    const std::string type;
    const std::map<std::string, std::string> fields;

  public:
    explicit Message(std::string messageType) : type(std::move(messageType)) {}

    Message(std::string messageType,
            std::map<std::string, std::string> messageFields)
        : type(std::move(messageType)), fields(std::move(messageFields)) {}

    std::string getField(const std::string& key) const {
        auto it = this->fields.find(key);
        return (it != this->fields.end()) ? it->second : "";
    }

    bool hasField(const std::string& key) const {
        return this->fields.find(key) != this->fields.end();
    }

    std::string getType() const { return this->type; }
    std::map<std::string, std::string> getFields() const {
        return this->fields;
    }
};

std::string serialize(const Message& msg);
Message deserialize(const std::string& data);

class Communication {
  private:
    void listen();
    std::string socket_path;
    int sock_fd{-1};
    std::atomic<bool> running{false};
    std::thread listener_thread;
    std::queue<Message> message_queue;
    std::mutex queue_mutex;

  public:
    Communication(std::string socket_path = "/tmp/smartpiano.sock");
    ~Communication();
    bool connect();
    void disconnect();
    bool isConnected() const;
    void send(const Message& msg);
    std::optional<Message> popMessage();
};

#endif // COMMUNICATION_HPP
