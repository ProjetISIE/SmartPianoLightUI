#include "Logger.hpp"
#include "UdsTransport.hpp"
#include <csignal>

static UdsTransport* g_transport = nullptr;

/**
 * @brief Gestionnaire de signaux pour arrêts propres
 * @param signum Numéro du signal reçu
 */
void signalHandler(int signum) {
    Logger::log("[MAIN] Signal reçu: {}", signum);
    if (g_transport) g_transport->stop();
}

int main() { //(int argc, char* argv[]) {
    std::println("[MAIN] Hello Smart Piano");
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    g_transport = nullptr; // Nettoyage
    return 0;
}
