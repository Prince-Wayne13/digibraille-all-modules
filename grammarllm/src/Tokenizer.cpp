#include "Tokenizer.h"

bool Tokenizer::isPunctuation(char c) {
    return (c == '.' || c == ',' || c == '!' || c == '?' ||
            c == ';' || c == ':' || c == '"'  ||
            c == '-' || c == '(' || c == ')');
    // Note: apostrophe ' is NOT in this list — handled by makeWordToken
}

// ─────────────────────────────────────────────
// Item 7: Apostrophe protection
// Splits "let's" → value="let", suffix="'s"
// Splits "player's" → value="player", suffix="'s"
// Splits "don't" → value="don", suffix="'t"
// Splits "they're" → value="they", suffix="'re"
// Root goes to spell checker, suffix cached, reattached on output
// ─────────────────────────────────────────────
Token Tokenizer::makeWordToken(const String& raw) {
    int apostrophePos = -1;

    for (int i = 1; i < (int)raw.length(); i++) {
        if (raw[i] == '\'') {
            apostrophePos = i;
            break;
        }
    }

    if (apostrophePos == -1) {
        // No apostrophe — plain word
        return Token(raw, TokenType::WORD, "");
    }

    // Split at apostrophe
    String root   = raw.substring(0, apostrophePos);
    String suffix = raw.substring(apostrophePos); // includes the '

    // Only split if root is non-empty and suffix is a known contraction ending
    // This prevents splitting hyphenated-ish words wrongly
    bool knownSuffix = (suffix == "'s"  || suffix == "'t"  ||
                        suffix == "'re" || suffix == "'ve" ||
                        suffix == "'ll" || suffix == "'d"  ||
                        suffix == "'m"  || suffix == "'S"); // possessive

    if (root.length() > 0 && knownSuffix) {
        return Token(root, TokenType::WORD, suffix);
    }

    // Unknown suffix — keep whole word intact
    return Token(raw, TokenType::WORD, "");
}

std::vector<Token> Tokenizer::tokenize(const String& input) {
    std::vector<Token> tokens;
    String current = "";

    for (int i = 0; i < (int)input.length(); i++) {
        char c = input[i];

        if (c == ' ' || c == '\t') {
            if (current.length() > 0) {
                if ((int)tokens.size() < MAX_TOKENS) tokens.push_back(makeWordToken(current));
                current = "";
            }
        }
        else if (isPunctuation(c)) {
            if (current.length() > 0) {
                if ((int)tokens.size() < MAX_TOKENS) tokens.push_back(makeWordToken(current));
                current = "";
            }
            if ((int)tokens.size() < MAX_TOKENS) tokens.push_back(Token(String(c), TokenType::PUNCTUATION, ""));
        }
        else {
            current += c;
        }
    }

    if (current.length() > 0) {
        if ((int)tokens.size() < MAX_TOKENS) tokens.push_back(makeWordToken(current));
    }

    return tokens;
}
