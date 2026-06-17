#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Logger.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>

TEST_CASE("Logger Verbose Mode Toggle") {
    Logger::setVerbose(true);
    CHECK(Logger::isVerbose() == true);

    Logger::setVerbose(false);
    CHECK(Logger::isVerbose() == false);
}

TEST_CASE("Logger Write and Verify File Logs") {
    const std::string testLogPath = "test_smartpiano.ui.log";
    const std::string testErrPath = "test_smartpiano.ui.err.log";

    // Ensure clean state before test
    std::filesystem::remove(testLogPath);
    std::filesystem::remove(testErrPath);

    // Initialize logger with custom test paths
    Logger::init(testLogPath, testErrPath);

    // Log typical message using formatting
    Logger::log("Test standard log message with value: {}", 999);
    Logger::err("Test error log message with detail: {}", "critical issue");

    // Validate that the files were created
    CHECK(std::filesystem::exists(testLogPath) == true);
    CHECK(std::filesystem::exists(testErrPath) == true);

    // Verify standard log contents
    {
        std::ifstream logFile(testLogPath);
        REQUIRE(logFile.is_open() == true);
        std::string line;
        bool found = false;
        while (std::getline(logFile, line)) {
            if (line.find("Test standard log message with value: 999") !=
                std::string::npos) {
                found = true;
                break;
            }
        }
        CHECK(found == true);
    }

    // Verify error log contents
    {
        std::ifstream errFile(testErrPath);
        REQUIRE(errFile.is_open() == true);
        std::string line;
        bool found = false;
        while (std::getline(errFile, line)) {
            if (line.find(
                    "Test error log message with detail: critical issue") !=
                std::string::npos) {
                found = true;
                break;
            }
        }
        CHECK(found == true);
    }

    // Clean up generated files after testing
    std::filesystem::remove(testLogPath);
    std::filesystem::remove(testErrPath);
}
