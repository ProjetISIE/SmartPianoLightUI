#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Communication.hpp"
#include "Message.hpp"
#include "MusicUtils.hpp"
#include <doctest/doctest.h>
#include <map>
#include <string>
#include <vector>

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
        std::map<std::string, std::string> fields = {{"game", "chord"},
                                                     {"scale", "g"}};
        Message msg("config", fields);
        std::string serialized = serialize(msg);

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
        CHECK(msg.getFields().size() == 1);
        CHECK(msg.getField("scale") == "g");
    }
}

TEST_CASE("MusicUtils Calculations") {
    using namespace MusicUtils;

    SUBCASE("splitNotes") {
        auto notes = splitNotes("c4 d4 e4");
        REQUIRE(notes.size() == 3);
        CHECK(notes[0] == "c4");
        CHECK(notes[1] == "d4");
        CHECK(notes[2] == "e4");

        auto empty = splitNotes("");
        CHECK(empty.empty() == true);
    }

    SUBCASE("noteLetterToFrench") {
        CHECK(noteLetterToFrench('c') == "DO");
        CHECK(noteLetterToFrench('d') == "RE");
        CHECK(noteLetterToFrench('e') == "MI");
        CHECK(noteLetterToFrench('f') == "FA");
        CHECK(noteLetterToFrench('g') == "SOL");
        CHECK(noteLetterToFrench('a') == "LA");
        CHECK(noteLetterToFrench('b') == "SI");
        CHECK(noteLetterToFrench('x') == "x");
    }

    SUBCASE("noteDisplayLabel") {
        // Syllabic mode
        CHECK(noteDisplayLabel("c4", NotationMode::SYLLABIC) == "DO 4");
        CHECK(noteDisplayLabel("c#4", NotationMode::SYLLABIC) == "DO# 4");
        CHECK(noteDisplayLabel("eb4", NotationMode::SYLLABIC) == "MIb 4");

        // Letter mode
        CHECK(noteDisplayLabel("c4", NotationMode::LETTER) == "C 4");
        CHECK(noteDisplayLabel("c#4", NotationMode::LETTER) == "C# 4");
        CHECK(noteDisplayLabel("eb4", NotationMode::LETTER) == "Eb 4");
    }

    SUBCASE("chordDisplayLabel") {
        CHECK(chordDisplayLabel("c maj", NotationMode::LETTER) == "c maj");
        CHECK(chordDisplayLabel("c maj", NotationMode::SYLLABIC) == "Do maj");
        CHECK(chordDisplayLabel("f# min", NotationMode::SYLLABIC) == "Fa# min");
    }

    SUBCASE("getNotationLabel") {
        CHECK(getNotationLabel(0, NotationMode::SYLLABIC) == "DO");
        CHECK(getNotationLabel(1, NotationMode::SYLLABIC) == "RE");
        CHECK(getNotationLabel(0, NotationMode::LETTER) == "C");
        CHECK(getNotationLabel(1, NotationMode::LETTER) == "D");
    }

    SUBCASE("getScaleNotesList") {
        auto scaleC =
            getScaleNotesList(ScaleChoice::SCALE_C, ModeChoice::MODE_MAJ);
        REQUIRE(scaleC.size() == 7);
        CHECK(scaleC[0] == "c");
        CHECK(scaleC[6] == "b");

        auto scaleCmin =
            getScaleNotesList(ScaleChoice::SCALE_C, ModeChoice::MODE_MIN);
        REQUIRE(scaleCmin.size() == 7);
        CHECK(scaleCmin[2] == "eb");
    }

    SUBCASE("resolveKey") {
        // White key resolution (C4 in base octave 4 -> white index 0)
        NoteKey nk1 = resolveKey("c4", 4);
        CHECK(nk1.valid == true);
        CHECK(nk1.isBlack == false);
        CHECK(nk1.index == 0);

        // Black key resolution (C#4 in base octave 4 -> black index 0)
        NoteKey nk2 = resolveKey("c#4", 4);
        CHECK(nk2.valid == true);
        CHECK(nk2.isBlack == true);
        CHECK(nk2.index == 0);

        // White key resolution (C3 in base octave 4 -> octave difference -1 ->
        // white index -7)
        NoteKey nk3 = resolveKey("c3", 4);
        CHECK(nk3.valid == true);
        CHECK(nk3.isBlack == false);
        CHECK(nk3.index == -7);
    }

    SUBCASE("getChallengeBaseOctave") {
        CHECK(getChallengeBaseOctave({"c4", "e4", "g4"}) == 4);
        CHECK(getChallengeBaseOctave({"c3", "e3"}) == 3);
        CHECK(getChallengeBaseOctave({"a5"}) == 5);
        CHECK(getChallengeBaseOctave({}) == 4);
    }

    SUBCASE("isSameNoteClass") {
        CHECK(isSameNoteClass("c4", "c") == true);
        CHECK(isSameNoteClass("C4", "c5") == true);
        CHECK(isSameNoteClass("c#4", "c5") == false);
        CHECK(isSameNoteClass("c#4", "c#5") == true);
    }

    SUBCASE("noteInList") {
        CHECK(noteInList("c4", {"c4", "e4"}) == true);
        CHECK(noteInList("c5", {"c4", "e4"}) ==
              true); // matches same class base
        CHECK(noteInList("d4", {"c4", "e4"}) == false);
    }
}
