#include "Reconstructor.h"

bool Reconstructor::isAttachLeft(const String& punct) {
    return (punct=="."||punct==","||punct=="!"||
            punct=="?"||punct==";"||punct==":"||
            punct==")"||punct=="'");
}

String Reconstructor::capitalizeFirst(const String& s) {
    if (!s.length()) return s;
    String r = s;
    r[0] = toupper(r[0]);
    return r;
}

String Reconstructor::reconstruct(const std::vector<Token>& tokens) {
    String output = "";
    bool firstWord = true;

    for (int i = 0; i < (int)tokens.size(); i++) {
        const Token& tok = tokens[i];

        if (tok.type == TokenType::PUNCTUATION) {
            output += tok.value;
        } else if (tok.type == TokenType::WORD) {
            String word = tok.value;

            if (firstWord) {
                word = capitalizeFirst(word);
                firstWord = false;
            }

            if (output.length() > 0) {
                char last = output[output.length()-1];
                if (last != '(') output += " ";
            }

            output += word;

            // Item 7: reattach apostrophe suffix if present
            // "let" + "'s" → "let's", "don" + "'t" → "don't"
            if (tok.suffix.length() > 0) {
                output += tok.suffix;
            }
        }
    }

    // Ensure sentence ends with punctuation
    if (output.length() > 0) {
        char last = output[output.length()-1];
        if (last != '.' && last != '!' && last != '?') output += ".";
    }

    return output;
}