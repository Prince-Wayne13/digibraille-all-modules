#pragma once
#include <Arduino.h>
#include <vector>
#include "POSTagger.h" // Ensure this defines TaggedToken, POS, and TokenType

class QuestionDetector {
public:
    // Returns true if sentence is a question based on punctuation or syntax
    bool isQuestion(const std::vector<TaggedToken>& tokens);

private:
    // Pattern A: starts with question word (who/what/where/when/why/how)
    bool patternQuestionWord(const std::vector<TaggedToken>& tokens);

    // Pattern B: AUX + PRONOUN + ... (is she coming? / can he run?)
    bool patternAuxPronoun(const std::vector<TaggedToken>& tokens);

    // Pattern C: AUX + NOUN + ... (does the dog eat? / is the car fast?)
    bool patternAuxNoun(const std::vector<TaggedToken>& tokens);

    // Pattern D: AUX + ADJECTIVE (is that right? / are you sure?)
    bool patternAuxAdjective(const std::vector<TaggedToken>& tokens);

    bool isQuestionWord(const String& word);
    bool isAuxiliary(const String& word);

    // First real word (skip punctuation)
    const TaggedToken* firstWord(const std::vector<TaggedToken>& tokens);
};