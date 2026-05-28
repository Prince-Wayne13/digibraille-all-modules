#include "Normalizer.h"

// ─────────────────────────────────────────────
// Known fused words → split form
// Checked at raw string level before tokenization
// ─────────────────────────────────────────────
struct FusedEntry {
    const char* fused;
    const char* split;
};

static const FusedEntry FUSED_WORDS[] = {
    // I contractions
    {"iam",         "i am"},
    {"iwas",        "i was"},
    {"iwill",       "i will"},
    {"iknow",       "i know"},
    {"ican",        "i can"},
    {"icant",       "i can not"},
    {"idont",       "i do not"},
    {"ilike",       "i like"},
    {"ilove",       "i love"},
    {"ihave",       "i have"},
    {"ido",         "i do"},
    {"igo",         "i go"},
    {"isee",        "i see"},
    {"ineed",       "i need"},
    {"iwant",       "i want"},
    {"ifeel",       "i feel"},
    {"ithink",      "i think"},
    {"iget",        "i get"},
    // negatives
    {"donot",       "do not"},
    {"doesnt",      "does not"},
    {"didnot",      "did not"},
    {"cannot",      "can not"},
    {"willnot",     "will not"},
    {"wasnot",      "was not"},
    {"werenot",     "were not"},
    {"isnot",       "is not"},
    {"arenot",      "are not"},
    {"havenot",     "have not"},
    {"hasnt",       "has not"},
    {"hadnot",      "had not"},
    {"wouldnot",    "would not"},
    {"shouldnot",   "should not"},
    {"couldnot",    "could not"},
    // common fusions
    {"alot",        "a lot"},
    {"alright",     "all right"},
    {"aswell",      "as well"},
    {"ofcourse",    "of course"},
    {"thankyou",    "thank you"},
    {"noone",       "no one"},
    {"everytime",   "every time"},
    {"infact",      "in fact"},
    {"inspite",     "in spite"},
    {"infront",     "in front"},
    {"atleast",     "at least"},
    {"alittle",     "a little"},
    {"abit",        "a bit"},
    {"eachother",   "each other"},
    {"sofar",       "so far"},
    {"upthere",     "up there"},
    {"downhere",    "down here"},
    {"inthere",     "in there"},
    {"outthere",    "out there"},
    {nullptr, nullptr}
};

// ─────────────────────────────────────────────
// Main normalize entry
// ─────────────────────────────────────────────
String Normalizer::normalize(const String& input) {
    String s = input;
    s.trim();
    s = fixApostrophes(s);
    s = collapseRepeats(s);
    s = splitFusedWords(s);
    return s;
}

// ─────────────────────────────────────────────
// Fused word splitter (item 5)
// O(N × F) where F = fused word table size (~55)
// Checks each token against the fused table
// Only splits whole tokens — never mid-word
// ─────────────────────────────────────────────
String Normalizer::splitFusedWords(const String& input) {
    // Tokenize by spaces, check each token
    String result = "";
    String token  = "";

    for (int i = 0; i <= (int)input.length(); i++) {
        char c = (i < (int)input.length()) ? input[i] : ' ';

        if (c == ' ' || c == '\t') {
            if (token.length() > 0) {
                // Check token against fused table
                String lower = token;
                lower.toLowerCase();

                bool found = false;
                for (int j = 0; FUSED_WORDS[j].fused != nullptr; j++) {
                    if (lower.equals(FUSED_WORDS[j].fused)) {
                        if (result.length() > 0) result += " ";
                        result += String(FUSED_WORDS[j].split);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (result.length() > 0) result += " ";
                    result += token;
                }
                token = "";
            }
        } else {
            token += c;
        }
    }
    return result;
}

// ─────────────────────────────────────────────
// Apostrophe normalization
// ─────────────────────────────────────────────
String Normalizer::fixApostrophes(const String& input) {
    String out = "";
    for (int i = 0; i < (int)input.length(); i++) {
        char c = input[i];
        if ((uint8_t)c == 0xe2 && i + 2 < (int)input.length()) {
            out += '\'';
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}

// ─────────────────────────────────────────────
// Collapse repeated characters
// "sooooo" → "soo", "heeey" → "hee"
// ─────────────────────────────────────────────
String Normalizer::collapseRepeats(const String& input) {
    String out = "";
    int count = 1;
    for (int i = 0; i < (int)input.length(); i++) {
        char c = input[i];
        if (i > 0 && c == input[i-1]) {
            count++;
            if (count <= 2) out += c;
        } else {
            count = 1;
            out += c;
        }
    }
    return out;
}
