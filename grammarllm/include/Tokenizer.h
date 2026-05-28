#pragma once
#include <Arduino.h>
#include <vector>

#define MAX_TOKENS 48

enum class TokenType {
    WORD,
    PUNCTUATION,
    WHITESPACE
};

struct Token {
    String value;
    TokenType type;
    String suffix;      // item 7: stores apostrophe suffix e.g. "'s", "'t", "'re"

    Token(const String& val, TokenType t) : value(val), type(t), suffix("") {}
    Token(const String& val, TokenType t, const String& suf)
        : value(val), type(t), suffix(suf) {}
};

class Tokenizer {
public:
    std::vector<Token> tokenize(const String& input);

private:
    bool isPunctuation(char c);

    // Item 7: split "let's" → value="let", suffix="'s"
    Token makeWordToken(const String& raw);
};
