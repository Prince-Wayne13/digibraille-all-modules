#pragma once
#include <Arduino.h>
#include <vector>
#include "POSTagger.h"

struct SentenceState {
    bool hasPastTimeWord;
    bool hasNegAux;
    bool isQuestion;
};

enum class SubjectNumber {
    UNKNOWN,
    SINGULAR,
    PLURAL
};

class GrammarEngine {
public:
    void correct(std::vector<TaggedToken>& tokens);

private:
    SentenceState detectState(const std::vector<TaggedToken>& tokens);

    void ruleCapitalI      (std::vector<TaggedToken>& tokens);
    void ruleWasWere       (std::vector<TaggedToken>& tokens, const SentenceState& s);
    void ruleNounBeAgreement(std::vector<TaggedToken>& tokens, const SentenceState& s);
    void ruleSubjectVerb   (std::vector<TaggedToken>& tokens, const SentenceState& s);
    void rulePluralSubjectVerb(std::vector<TaggedToken>& tokens, const SentenceState& s);
    void ruleTheirThereTheyre(std::vector<TaggedToken>& tokens);
    void rulePastTense     (std::vector<TaggedToken>& tokens);
    void ruleMissingArticle(std::vector<TaggedToken>& tokens);
    void ruleAvsAn         (std::vector<TaggedToken>& tokens);
    void ruleDoubleNegative(std::vector<TaggedToken>& tokens, const SentenceState& s);
    void ruleAdverbPlacement(std::vector<TaggedToken>& tokens, const SentenceState& s);

    // Confidence gates
    bool isCertain         (const TaggedToken& t);
    bool isConfidentOrAbove(const TaggedToken& t);  // ← was missing

    bool   isSingularThirdPerson (const String& w);
    bool   isPluralOrFirstSecond (const String& w);
    bool   isQuestionStarter     (const String& w);
    bool   isAuxiliaryWord       (const String& w);
    bool   canStartSubjectAt     (const std::vector<TaggedToken>& tokens, int idx);
    bool   isSingularProperNoun  (const String& w);
    SubjectNumber nounSubjectNumber(const String& noun, const String& determiner);
    String toThirdPersonSingular (const String& verb);
    String toBaseFromThirdPerson (const String& verb);
    String toPastTense           (const String& verb);
    bool   startsWithVowel       (const String& w);
    bool   isBaseVerb            (const String& v);
    bool   isShortBaseVerb       (const String& v);

    char _buf[32];
};
