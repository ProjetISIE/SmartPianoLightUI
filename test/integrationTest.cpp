#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Communication.hpp"
#include "Message.hpp"
#include <doctest/doctest.h>
#include <map>
#include <string>

TEST_CASE("Message Class Structure") {
    SUBCASE("Constructor with Type Only") {
        Message msg("ready");
        CHECK(msg.getType() == "ready");
        CHECK(msg.getFields().empty() == true);
        CHECK(msg.hasField("status") == false);
        CHECK(msg.getField("status") == "");
    }

    SUBCASE("Constructor with Type and Fields") {
        std::map<std::string, std::string> fields = {
            {"game", "note"}, {"scale", "c"}, {"mode", "maj"}};
        Message msg("config", fields);
        CHECK(msg.getType() == "config");
        CHECK(msg.getFields().size() == 3);
        CHECK(msg.hasField("game") == true);
        CHECK(msg.getField("game") == "note");
        CHECK(msg.hasField("scale") == true);
        CHECK(msg.getField("scale") == "c");
        CHECK(msg.hasField("mode") == true);
        CHECK(msg.getField("mode") == "maj");
        CHECK(msg.hasField("invalid_field") == false);
        CHECK(msg.getField("invalid_field") == "");
    }
}

TEST_CASE("Message Serialization and Deserialization") {
    SUBCASE("Serialization of Type Only") {
        Message msg("ready");
        std::string serialized = serialize(msg);
        CHECK(serialized == "ready\n\n");
    }

    SUBCASE("Serialization of Type and Fields") {
        // Map elements are sorted by key: game, mode, scale
        std::map<std::string, std::string> fields = {{"game", "chord"},
                                                     {"scale", "g"}};
        Message msg("config", fields);
        std::string serialized = serialize(msg);

        // Expected format (keys sorted alphabetically):
        // config
        // game=chord
        // scale=g
        // \n
        std::string expected = "config\ngame=chord\nscale=g\n\n";
        CHECK(serialized == expected);
    }

    SUBCASE("Deserialization of Type Only") {
        std::string raw = "ready\n\n";
        Message msg = deserialize(raw);
        CHECK(msg.getType() == "ready");
        CHECK(msg.getFields().empty() == true);
    }

    SUBCASE("Deserialization of Type and Fields") {
        std::string raw = "config\ngame=chord\nscale=g\n\n";
        Message msg = deserialize(raw);
        CHECK(msg.getType() == "config");
        CHECK(msg.getFields().size() == 2);
        CHECK(msg.getField("game") == "chord");
        CHECK(msg.getField("scale") == "g");
    }

    SUBCASE("Deserialization of Malformed Content") {
        std::string raw = "config\nmalformed_line_no_equals\nscale=g\n\n";
        Message msg = deserialize(raw);
        CHECK(msg.getType() == "config");
        // The malformed line should be ignored, only scale=g should be parsed
        CHECK(msg.getFields().size() == 1);
        CHECK(msg.getField("scale") == "g");
    }
}
