#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <map>
#include <string>

/**
 * @brief Représente un message échangé via le protocole UDS
 *
 * Un message est composé d'un type et de champs optionnels key=value
 */
struct Message {
  private:
    const std::string type;                          ///< Type du message
    const std::map<std::string, std::string> fields; ///< Champs du message

  public:
    /**
     * @brief Constructeur avec type uniquement
     * @param messageType Type du message
     */
    explicit Message(std::string messageType) : type(std::move(messageType)) {}

    /**
     * @brief Constructeur avec type et champs
     * @param messageType Type du message
     * @param messageFields Champs du message
     */
    Message(std::string messageType,
            std::map<std::string, std::string> messageFields)
        : type(std::move(messageType)), fields(std::move(messageFields)) {}

    /**
     * @brief Récupère valeur d'un champ
     * @param key Clé du champ
     * @return Valeur du champ ou chaîne vide si inexistant
     */
    std::string getField(const std::string& key) const {
        auto it = this->fields.find(key);
        return (it != this->fields.end()) ? it->second : "";
    }

    /**
     * @brief Vérifie si un champ existe
     * @param key Clé du champ
     * @return true si le champ existe
     */
    bool hasField(const std::string& key) const {
        return this->fields.find(key) != this->fields.end();
    }

    std::string getType() const { return this->type; }
    std::map<std::string, std::string> getFields() const {
        return this->fields;
    }
};

#endif // MESSAGE_HPP
