#include "PunctuationEngine.h"

// ─────────────────────────────────────────────
// Helper: make a punctuation TaggedToken
// ─────────────────────────────────────────────
TaggedToken PunctuationEngine::makePunctToken(const String& punct) {
    Token t(punct, TokenType::PUNCTUATION);
    return TaggedToken(t, POS::UNKNOWN, POSConfidence::CERTAIN);
}

// ─────────────────────────────────────────────
// Words that introduce a sentence and need
// a comma after them
// ─────────────────────────────────────────────
bool PunctuationEngine::isIntroductoryWord(const String& word) {
    static const char* introWords[] = {
        // Time adverbs
        "yesterday","tomorrow","today","recently","finally","suddenly",
        "previously","afterwards","meanwhile","later","earlier","soon",
        "once","then","next","first","last","initially","eventually",
        // Interjections
        "oh","wow","well","yes","no","hey","ouch","hmm","okay","ok",
        "indeed","however","therefore","moreover","furthermore",
        "nevertheless","otherwise","consequently","unfortunately",
        "honestly","basically","actually","clearly","obviously",
        nullptr
    };
    for (int i = 0; introWords[i] != nullptr; i++) {
        if (word.equals(introWords[i])) return true;
    }
    return false;
}

// ─────────────────────────────────────────────
// Coordinating conjunctions (FANBOYS)
// ─────────────────────────────────────────────
bool PunctuationEngine::isCoordinatingConjunction(const String& word) {
    return (word == "for"  || word == "and" || word == "nor" ||
            word == "but"  || word == "or"  || word == "yet" ||
            word == "so");
}

// ─────────────────────────────────────────────
// Check if tokens from startIdx form a clause
// with at least a subject + verb
// ─────────────────────────────────────────────
bool PunctuationEngine::hasSubjectVerb(const std::vector<TaggedToken>& tokens, int startIdx) {
    bool hasSubject = false;
    bool hasVerb    = false;

    for (int i = startIdx; i < (int)tokens.size(); i++) {
        POS p = tokens[i].pos;
        if (p == POS::PRONOUN || p == POS::NOUN) hasSubject = true;
        if (p == POS::VERB || p == POS::AUXILIARY) hasVerb = true;
        if (hasSubject && hasVerb) return true;
    }
    return false;
}

// ─────────────────────────────────────────────
// Rule 1: Question mark
// If isQuestion → replace final period with ?
// If NOT question → ensure ends with period
// ─────────────────────────────────────────────
void PunctuationEngine::ruleQuestionMark(std::vector<TaggedToken>& tokens, bool isQuestion) {
    if (tokens.empty()) return;

    // Find last token
    int last = (int)tokens.size() - 1;

    // Remove any trailing punctuation first
    while (last >= 0 && tokens[last].token.type == TokenType::PUNCTUATION) {
        tokens.erase(tokens.begin() + last);
        last--;
    }

    // Add correct ending punctuation
    if (isQuestion) {
        tokens.push_back(makePunctToken("?"));
    } else {
        tokens.push_back(makePunctToken("."));
    }
}

// ─────────────────────────────────────────────
// Rule 2: Introductory comma
// First word is introductory → insert comma after it
// BUT only if second word is not already a comma
// and sentence has more than 2 words
// ─────────────────────────────────────────────
void PunctuationEngine::ruleIntroductoryComma(std::vector<TaggedToken>& tokens) {
    if (tokens.size() < 3) return;

    // Find first WORD token
    int firstIdx = -1;
    for (int i = 0; i < (int)tokens.size(); i++) {
        if (tokens[i].token.type == TokenType::WORD) { firstIdx = i; break; }
    }
    if (firstIdx == -1) return;

    String first = tokens[firstIdx].token.value;
    first.toLowerCase();

    if (!isIntroductoryWord(first)) return;

    // Check next token — if already a comma, skip
    int nextIdx = firstIdx + 1;
    if (nextIdx < (int)tokens.size() &&
        tokens[nextIdx].token.type == TokenType::PUNCTUATION &&
        tokens[nextIdx].token.value == ",") return;

    if (nextIdx < (int)tokens.size()) {
        String next = tokens[nextIdx].token.value; next.toLowerCase();
        if (next == "is" || next == "are" || next == "was" || next == "were" ||
            next == "will" || next == "can" || next == "could" || next == "should") {
            return;
        }
    }

    // Insert comma after first word
    tokens.insert(tokens.begin() + firstIdx + 1, makePunctToken(","));
}

// ─────────────────────────────────────────────
// Rule 3: Compound sentence comma
// Pattern: [clause1] + CONJUNCTION + [clause2]
// where both clause1 and clause2 have subject+verb
// "I went to school and she stayed home"
//  → "I went to school, and she stayed home"
// ─────────────────────────────────────────────
void PunctuationEngine::ruleCompoundComma(std::vector<TaggedToken>& tokens) {
    for (int i = 1; i < (int)tokens.size() - 1; i++) {
        if (tokens[i].token.type != TokenType::WORD) continue;

        String w = tokens[i].token.value;
        w.toLowerCase();

        if (!isCoordinatingConjunction(w)) continue;

        // Check token before conjunction is not already a comma
        if (tokens[i-1].token.type == TokenType::PUNCTUATION &&
            tokens[i-1].token.value == ",") continue;

        // Check clause after conjunction has subject+verb
        if (!hasSubjectVerb(tokens, i + 1)) continue;

        // Check clause before conjunction also has subject+verb
        // (search from start up to i)
        bool firstClauseOk = false;
        bool foundSubj = false, foundVerb = false;
        for (int j = 0; j < i; j++) {
            POS p = tokens[j].pos;
            if (p == POS::PRONOUN || p == POS::NOUN) foundSubj = true;
            if (p == POS::VERB || p == POS::AUXILIARY) foundVerb = true;
        }
        firstClauseOk = foundSubj && foundVerb;

        if (!firstClauseOk) continue;

        // Insert comma before conjunction
        tokens.insert(tokens.begin() + i, makePunctToken(","));
        i++; // skip past inserted comma
    }
}

// ─────────────────────────────────────────────
// Main entry
// ─────────────────────────────────────────────
void PunctuationEngine::correct(std::vector<TaggedToken>& tokens, bool isQuestion) {
    ruleIntroductoryComma(tokens);   // commas first — before end punctuation changes
    ruleCompoundComma(tokens);
    ruleQuestionMark(tokens, isQuestion); // end punctuation last
}
