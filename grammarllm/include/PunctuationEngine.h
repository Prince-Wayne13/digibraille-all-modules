#pragma once
#include <Arduino.h>
#include <vector>
#include "POSTagger.h"
#include "QuestionDetector.h"

class PunctuationEngine {
public:
    // Main entry — runs all punctuation rules
    // Modifies token list in place, adds/changes punctuation tokens
    void correct(std::vector<TaggedToken>& tokens, bool isQuestion);

private:
    // Rule 1: question mark at end if isQuestion
    void ruleQuestionMark(std::vector<TaggedToken>& tokens, bool isQuestion);

    // Rule 2: comma after introductory time/place adverb at start
    // "Yesterday I went" → "Yesterday, I went"
    // "Oh I see"         → "Oh, I see"
    void ruleIntroductoryComma(std::vector<TaggedToken>& tokens);

    // Rule 3: comma before coordinating conjunction in compound sentence
    // "I went to school and she stayed home" → "..., and she stayed home"
    void ruleCompoundComma(std::vector<TaggedToken>& tokens);

    // Helpers
    bool isIntroductoryWord(const String& word);
    bool isCoordinatingConjunction(const String& word);
    bool hasSubjectVerb(const std::vector<TaggedToken>& tokens, int startIdx);
    TaggedToken makePunctToken(const String& punct);
};
