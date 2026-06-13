#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <print>
#include <string>

class Logger {
  private:
    static inline std::string logFilePath{"smartpiano.log"}; ///< Log standard
    static inline std::string errFilePath{"smartpiano.err.log"}; ///< Erreurs
    static inline std::mutex logMutex; ///< Mutex accès thread-safe
    static constexpr uint64_t MAX_LOG_SIZE{2 * 1024 * 1024}; ///< Maxi (2 Mo)

  private:
    /**
     * @brief Retourne heure formatée
     * @return Horodatage "HH:MM:SS.mmm"
     */
    [[nodiscard]] static std::string time() {
        try {
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
            return std::format("{:%T}", std::chrono::zoned_time{
                                            std::chrono::current_zone(),
                                            std::chrono::system_clock::now()});
#else
            // Robust POSIX fallback keeping local time rather than epoch
            // seconds
            auto now = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            std::tm tm_buf;
            localtime_r(&now, &tm_buf);
            return std::format("{:02}:{:02}:{:02}", tm_buf.tm_hour,
                               tm_buf.tm_min, tm_buf.tm_sec);
#endif
        } catch (...) {
            return "00:00:00";
        }
    }

    /**
     * @brief Retourne date formatée
     * @return Horodatage "YYYY-MM-DD"
     */
    [[nodiscard]] static std::string date() {
        try {
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
            return std::format("{:%F}", std::chrono::zoned_time{
                                            std::chrono::current_zone(),
                                            std::chrono::system_clock::now()});
#else
            // Robust POSIX fallback for compilers lacking C++20 timezone
            // support
            auto now = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            std::tm tm_buf;
            localtime_r(&now, &tm_buf);
            return std::format("{:04}-{:02}-{:02}", tm_buf.tm_year + 1900,
                               tm_buf.tm_mon + 1, tm_buf.tm_mday);
#endif
        } catch (...) {
            return "1970-01-01";
        }
    }

    /**
     * @brief Gère la rotation des fichiers de log
     * @param filePath Chemin du fichier à faire tourner
     */
    static void rotateLog(const std::string& filePath) {
        // Renomme fichier actuel comme sauvegarde
        std::filesystem::rename(filePath, date() + filePath);
        // Crée nouveau fichier vide
        std::ofstream newFile(filePath, std::ios::trunc);
        if (!newFile.is_open())
            std::println(stderr, "[Logger] Impossible de recréer fichier");
    }

    /**
     * @brief Écrit un message dans le fichier de log approprié
     * @param message Message à écrire
     * @param isError true pour log d'erreurs, false pour log standard
     */
    static void writeLog(const std::string& message, std::string& path) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (std::filesystem::exists(path) &&
            std::filesystem::is_regular_file(path) &&
            std::filesystem::file_size(path) > MAX_LOG_SIZE)
            rotateLog(path); // Rotation des logs si nécessaire
        // Écrit message dans fichier et dans sortie appropriés
        std::ofstream file(path, std::ios::app);
        if (file.is_open()) {
            file << std::format("[{} {}] {}\n", date(), time(), message);
            std::println(stdout, "{}", message);
        } else {
            std::println(stderr, "[Logger] Impossible d'écrire dans fichier");
            std::println(stderr, "{}", message);
        }
    }

  public:
    /**
     * @brief Initialise le mutex et vérifie que les fichiers peuvent être créés
     */
    static void init() {
        std::lock_guard<std::mutex> lock(logMutex);
        // Vérifie que les fichiers peuvent être créés
        std::ofstream basicFile(Logger::logFilePath, std::ios::app);
        std::ofstream errorFile(Logger::errFilePath, std::ios::app);
        if (!basicFile.is_open() || !errorFile.is_open())
            std::println(stderr,
                         "[Logger] Impossible de créer les fichiers de log");
    }

    /**
     * @brief Initialise avec des chemins des fichiers de log spécifiques
     * @param logPath Chemin du fichier de log standard
     * @param errPath Chemin du fichier de log d'erreurs
     */
    static void init(const std::string& logPath, const std::string& errPath) {
        Logger::logFilePath = logPath;
        Logger::errFilePath = errPath;
        Logger::init();
    }

    /**
     * @brief Écrit un message dans le fichier de log standard
     * @tparam Args Types des arguments de formatage
     * @param fmt Chaîne de formatage
     * @param args Arguments de formatage
     */
    template <typename... Args>
    static void log(std::format_string<Args...> fmt, Args&&... args) {
        writeLog(std::format(fmt, std::forward<Args>(args)...),
                 Logger::logFilePath);
    }

    /**
     * @brief Écrit un message dans le fichier de log d'erreurs
     * @tparam Args Types des arguments de formatage
     * @param fmt Chaîne de formatage
     * @param args Arguments de formatage
     */
    template <typename... Args>
    static void err(std::format_string<Args...> fmt, Args&&... args) {
        writeLog(std::format(fmt, std::forward<Args>(args)...),
                 Logger::errFilePath);
    }
};

#endif // LOGGER_H
