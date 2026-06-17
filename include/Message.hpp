#ifndef CODE_UI_INCLUDE_MESSAGE_HPP_
#define CODE_UI_INCLUDE_MESSAGE_HPP_

#include <map>
#include <string>
#include <utility>

/**
 * @brief Représente un message échangé via le protocole UDS
 *
 * Un message est composé d'un type et de champs optionnels key=value
 */
class Message {
  private:
    std::string type_;                          ///< Type du message
    std::map<std::string, std::string> fields_; ///< Champs du message

  public:
    /**
     * @brief Constructeur avec type uniquement
     * @param messageType Type du message
     */
    explicit Message(std::string messageType) : type_(std::move(messageType)) {}

    /**
     * @brief Constructeur avec type et champs
     * @param messageType Type du message
     * @param messageFields Champs du message
     */
    Message(std::string messageType,
            std::map<std::string, std::string> messageFields)
        : type_(std::move(messageType)), fields_(std::move(messageFields)) {}

    /**
     * @brief Récupère la valeur d'un champ
     * @param key Clé du champ
     * @return Valeur du champ, ou chaîne vide si inexistant
     */
    [[nodiscard]] const std::string&
    getField(const std::string& key) const noexcept {
        static const std::string emptyString;
        auto it = this->fields_.find(key);
        return (it != this->fields_.end()) ? it->second : emptyString;
    }

    /**
     * @brief Vérifie si un champ existe
     * @param key Clé du champ
     * @return true si le champ existe
     */
    [[nodiscard]] bool hasField(const std::string& key) const noexcept {
        return this->fields_.find(key) != this->fields_.end();
    }

    [[nodiscard]] const std::string& getType() const noexcept {
        return this->type_;
    }

    [[nodiscard]] const std::map<std::string, std::string>&
    getFields() const noexcept {
        return this->fields_;
    }
};

#endif // CODE_UI_INCLUDE_MESSAGE_HPP_
