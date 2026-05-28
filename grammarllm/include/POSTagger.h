#pragma once
#include <Arduino.h>
#include <vector>
#include "Tokenizer.h"

enum class POS {
    PRONOUN,
    VERB,
    NOUN,
    ARTICLE,
    ADJECTIVE,
    ADVERB,
    PREPOSITION,
    CONJUNCTION,
    AUXILIARY,
    NEGATIVE,
    TIME_PAST,
    DETERMINER,
    INTERJECTION,
    UNKNOWN
};

enum class POSConfidence {
    CERTAIN,      // lookup table — exact match
    CONFIDENT,    // suffix heuristic — strong signal
    MEDIUM,       // suffix heuristic — ambiguous
    UNSURE        // pure guess — grammar rules must not fire
};

struct TaggedToken {
    Token         token;
    POS           pos;
    POSConfidence confidence;

    TaggedToken(const Token& t, POS p, POSConfidence c = POSConfidence::CERTAIN)
        : token(t), pos(p), confidence(c) {}
};

class POSTagger {
public:
    std::vector<TaggedToken> tag(const std::vector<Token>& tokens);

private:
    POS classifyWord(const String& word, POSConfidence& outConf);
    POS guessBySuffix(const String& word, POSConfidence& outConf);
    void smoothWithBigrams(std::vector<TaggedToken>& tagged);
    uint8_t transitionScore(POS left, POS right);
};
