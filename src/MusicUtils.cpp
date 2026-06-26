#include "MusicUtils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace MusicUtils {

std::vector<std::string> splitNotes(const std::string& s) {
    std::vector<std::string> notes;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        notes.push_back(tok);
    }
    return notes;
}

std::string noteLetterToFrench(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
    case 'c': return "DO";
    case 'd': return "RE";
    case 'e': return "MI";
    case 'f': return "FA";
    case 'g': return "SOL";
    case 'a': return "LA";
    case 'b': return "SI";
    default: return std::string(1, c);
    }
}

std::string noteDisplayLabel(const std::string& note, NotationMode mode) {
    if (note.empty()) return note;
    if (mode == NotationMode::LETTER) {
        std::string label;
        label += static_cast<char>(
            std::toupper(static_cast<unsigned char>(note[0])));
        size_t i = 1;
        if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
            label += note[i];
            ++i;
        }
        if (i < note.size()) {
            label += " " + note.substr(i);
        }
        return label;
    } else {
        std::string label = noteLetterToFrench(note[0]);
        size_t i = 1;
        if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
            label += (note[i] == '#') ? "#" : "b";
            ++i;
        }
        if (i < note.size()) {
            label += " " + note.substr(i);
        }
        return label;
    }
}

std::string chordDisplayLabel(const std::string& chordName, NotationMode mode) {
    if (chordName.empty()) return chordName;
    if (mode == NotationMode::LETTER) {
        return chordName;
    }
    size_t rootLen = 1;
    if (chordName.size() > 1 && (chordName[1] == '#' || chordName[1] == 'b')) {
        rootLen = 2;
    }
    std::string rootPart = chordName.substr(0, rootLen);
    std::string restPart = chordName.substr(rootLen);

    std::string frenchRoot = noteLetterToFrench(rootPart[0]);
    if (frenchRoot == "DO") frenchRoot = "Do";
    else if (frenchRoot == "RE") frenchRoot = "Ré";
    else if (frenchRoot == "MI") frenchRoot = "Mi";
    else if (frenchRoot == "FA") frenchRoot = "Fa";
    else if (frenchRoot == "SOL") frenchRoot = "Sol";
    else if (frenchRoot == "LA") frenchRoot = "La";
    else if (frenchRoot == "SI") frenchRoot = "Si";

    if (rootLen > 1) {
        frenchRoot += rootPart[1];
    }

    return frenchRoot + restPart;
}

std::string getNotationLabel(int32_t whiteIdx, NotationMode mode) {
    static const char* syllabic[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI"};
    static const char* letters[] = {"C", "D", "E", "F", "G", "A", "B"};
    if (mode == NotationMode::LETTER) {
        return letters[whiteIdx % 7];
    }
    return syllabic[whiteIdx % 7];
}

std::vector<std::string> getScaleNotesList(ScaleChoice scale, ModeChoice mode) {
    int s = static_cast<int>(scale);
    bool isMaj = (mode == ModeChoice::MODE_MAJ);
    if (isMaj) {
        switch (s) {
        case 0: return {"c", "d", "e", "f", "g", "a", "b"};
        case 1: return {"d", "e", "f#", "g", "a", "b", "c#"};
        case 2: return {"e", "f#", "g#", "a", "b", "c#", "d#"};
        case 3: return {"f", "g", "a", "bb", "c", "d", "e"};
        case 4: return {"g", "a", "b", "c", "d", "e", "f#"};
        case 5: return {"a", "b", "c#", "d", "e", "f#", "g#"};
        case 6: return {"b", "c#", "d#", "e", "f#", "g#", "a#"};
        default: break;
        }
    } else {
        switch (s) {
        case 0: return {"c", "d", "eb", "f", "g", "ab", "bb"};
        case 1: return {"d", "e", "f", "g", "a", "bb", "c"};
        case 2: return {"e", "f#", "g", "a", "b", "c", "d"};
        case 3: return {"f", "g", "ab", "bb", "c", "db", "eb"};
        case 4: return {"g", "a", "bb", "c", "d", "eb", "f"};
        case 5: return {"a", "b", "c", "d", "e", "f", "g"};
        case 6: return {"b", "c#", "d", "e", "f#", "g", "a"};
        default: break;
        }
    }
    return {};
}

std::string getScaleNameFormatted(ScaleChoice scale, ModeChoice mode,
                                  NotationMode notation) {
    static const char* syllabic[] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};
    static const char* letters[] = {"C", "D", "E", "F", "G", "A", "B"};
    int s = static_cast<int>(scale);
    std::string name =
        (notation == NotationMode::LETTER) ? letters[s % 7] : syllabic[s % 7];
    name += " ";
    name += (mode == ModeChoice::MODE_MAJ) ? "Majeur" : "Mineur";
    return name;
}

NoteKey resolveKey(const std::string& note, int32_t baseKeyboardOctave) {
    if (note.empty()) return {};
    char letter = note[0];
    size_t i = 1;
    std::string mod;
    if (i < note.size() && (note[i] == '#' || note[i] == 'b')) {
        mod = note[i];
        ++i;
    }

    int32_t octave = 4; // Default octave
    if (i < note.size() && std::isdigit(static_cast<unsigned char>(note[i]))) {
        octave = note[i] - '0';
    }

    static constexpr int32_t WHITE_MAP[7] = {5, 6, 0, 1, 2, 3, 4};
    if (letter < 'a' || letter > 'g') return {};
    int32_t whiteIdx = WHITE_MAP[static_cast<int>(letter - 'a')];

    // Offset based on octave difference
    int32_t baseIdx = (octave - baseKeyboardOctave) * 7;
    whiteIdx += baseIdx;

    int32_t octaveOffset = 0;
    if (whiteIdx < 0) {
        octaveOffset = (whiteIdx - 6) / 7;
        whiteIdx -= octaveOffset * 7;
    }

    if (mod.empty()) {
        return {false, whiteIdx + octaveOffset * 7, true};
    }

    int32_t bkWhiteIdx = whiteIdx;
    if (mod == "b") {
        bkWhiteIdx -= 1;
        // handle underflow for negative bkWhiteIdx if needed
        if (bkWhiteIdx < 0) {
            bkWhiteIdx += 7;
            octaveOffset -= 1;
        }
    }

    static const int32_t WHITE_TO_BLACK_PER_OCTAVE[7] = {0, 1, -1, 2, 3, 4, -1};
    int32_t localWhiteIdx = bkWhiteIdx % 7;
    int32_t bkLocal = WHITE_TO_BLACK_PER_OCTAVE[localWhiteIdx];
    if (bkLocal < 0) return {}; // e.g. E# or Cb

    int32_t bkIdx = (bkWhiteIdx / 7) * 5 + bkLocal + octaveOffset * 5;
    return {true, bkIdx, true};
}

int32_t getChallengeBaseOctave(const std::vector<std::string>& expectedNotes) {
    if (expectedNotes.empty()) return 4;
    int32_t minOctave = 9;
    for (const auto& note : expectedNotes) {
        if (note.empty()) continue;
        char lastChar = note.back();
        if (std::isdigit(static_cast<unsigned char>(lastChar))) {
            int32_t oct = lastChar - '0';
            if (oct < minOctave) {
                minOctave = oct;
            }
        }
    }
    if (minOctave == 9) return 4;
    return minOctave;
}

bool isSameNoteClass(const std::string& a, const std::string& b) {
    auto clean = [](const std::string& s) {
        std::string r;
        for (char c : s) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                r += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
        }
        return r;
    };
    return clean(a) == clean(b);
}

bool noteInList(const std::string& noteBase,
                const std::vector<std::string>& list) {
    auto strip = [](const std::string& n) {
        if (n.empty()) return n;
        std::string s;
        s += n[0];
        if (n.size() > 1 && (n[1] == '#' || n[1] == 'b')) {
            s += n[1];
        }
        return s;
    };
    std::string base = strip(noteBase);
    for (const auto& n : list) {
        if (strip(n) == base) return true;
    }
    return false;
}

} // namespace MusicUtils
