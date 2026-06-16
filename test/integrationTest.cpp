#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Communication.hpp"
#include "Mocks.hpp"
#include <doctest/doctest.h>

TEST_CASE("serialize and deserialize preserve protocol content") {
    Message original("chord", {{"name", "C maj"}, {"notes", "c4 e4 g4"}});

    const std::string payload = serialize(original);
    Message decoded = deserialize(payload);

    CHECK(decoded.getType() == "chord");
    CHECK(decoded.getField("name") == "C maj");
    CHECK(decoded.getField("notes") == "c4 e4 g4");
}
