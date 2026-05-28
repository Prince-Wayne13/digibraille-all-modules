#include "QuestionDetector.h"

// ─────────────────────────────────────────────
// Public Entry Point
// ─────────────────────────────────────────────
bool QuestionDetector::isQuestion(const std::vector<TaggedToken>& tokens) {
    // Fallback 1: Literal punctuation check
    for (const auto& t : tokens) {
        if (t.token.type == TokenType::PUNCTUATION && t.token.value == "?") {
            return true;
        }
    }

    if (tokens.empty()) return false;

    // Evaluate syntactic patterns
    if (patternQuestionWord(tokens)) return true;
    if (patternAuxPronoun(tokens))    return true;
    if (patternAuxNoun(tokens))       return true;
    if (patternAuxAdjective(tokens))  return true;

    return false;
}

// ─────────────────────────────────────────────
// Pattern Implementations
// ─────────────────────────────────────────────

bool QuestionDetector::patternQuestionWord(const std::vector<TaggedToken>& tokens) {
    const TaggedToken* first = firstWord(tokens);
    if (!first) return false;

    return isQuestionWord(first->token.value);
}

bool QuestionDetector::patternAuxPronoun(const std::vector<TaggedToken>& tokens) {
    std::vector<const TaggedToken*> words;
    for (const auto& t : tokens) {
        if (t.token.type == TokenType::WORD) words.push_back(&t);
    }

    if (words.size() < 2) return false;

    if (isAuxiliary(words[0]->token.value) && words[1]->pos == POS::PRONOUN) {
        return true;
    }
    return false;
}

bool QuestionDetector::patternAuxNoun(const std::vector<TaggedToken>& tokens) {
    std::vector<const TaggedToken*> words;
    for (const auto& t : tokens) {
        if (t.token.type == TokenType::WORD) words.push_back(&t);
    }

    if (words.size() < 2) return false;

    if (isAuxiliary(words[0]->token.value)) {
        if (words[1]->pos == POS::NOUN || words[1]->pos == POS::UNKNOWN) {
            return true;
        }
        if (words.size() >= 3 && 
            (words[1]->pos == POS::ARTICLE || words[1]->pos == POS::DETERMINER)) {
            if (words[2]->pos == POS::NOUN || words[2]->pos == POS::UNKNOWN) {
                return true;
            }
        }
    }
    return false;
}

bool QuestionDetector::patternAuxAdjective(const std::vector<TaggedToken>& tokens) {
    std::vector<const TaggedToken*> words;
    for (const auto& t : tokens) {
        if (t.token.type == TokenType::WORD) words.push_back(&t);
    }

    if (words.size() < 2) return false;

    if (isAuxiliary(words[0]->token.value)) {
        if (words[1]->pos == POS::ADJECTIVE) {
            return true;
        }
        if (words.size() >= 3 && 
            (words[1]->pos == POS::PRONOUN || words[1]->pos == POS::DETERMINER)) {
            if (words[2]->pos == POS::ADJECTIVE) {
                return true;
            }
        }
    }
    return false;
}

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

bool QuestionDetector::isQuestionWord(const String& word) {
    String w = word; w.toLowerCase();
    return (w == "who"  || w == "what" || w == "where" || 
            w == "when" || w == "why"  || w == "how");
}

bool QuestionDetector::isAuxiliary(const String& word) {
    String w = word; w.toLowerCase();
    return (w == "is"    || w == "are"   || w == "am"    || w == "was"    || 
            w == "were"  || w == "can"   || w == "could" || w == "do"     || 
            w == "does"  || w == "did"   || w == "will"  || w == "would"  || 
            w == "should"|| w == "has"   || w == "have"  || w == "had");
}

const TaggedToken* QuestionDetector::firstWord(const std::vector<TaggedToken>& tokens) {
    for (const auto& t : tokens) {
        if (t.token.type == TokenType::WORD) {
            return &t;
        }
    }
    return nullptr;
}