#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Logger.hpp"
#include "Message.hpp"
#include <doctest/doctest.h>

TEST_CASE("Message stores and returns fields") {
    Message msg("config", {{"game", "note"}, {"mode", "maj"}});

    CHECK(msg.getType() == "config");
    CHECK(msg.hasField("game"));
    CHECK(msg.getField("game") == "note");
    CHECK(msg.getField("mode") == "maj");
    CHECK_FALSE(msg.hasField("unknown"));
    CHECK(msg.getField("unknown").empty());
}
