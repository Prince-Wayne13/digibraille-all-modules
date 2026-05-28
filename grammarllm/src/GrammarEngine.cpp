#include "GrammarEngine.h"

// ─────────────────────────────────────────────
// Confidence gates
// ─────────────────────────────────────────────
bool GrammarEngine::isCertain(const TaggedToken& t) {
    return t.confidence == POSConfidence::CERTAIN;
}

bool GrammarEngine::isConfidentOrAbove(const TaggedToken& t) {
    return (t.confidence == POSConfidence::CERTAIN ||
            t.confidence == POSConfidence::CONFIDENT);
}

// ─────────────────────────────────────────────
// Sentence-level state detection
// ─────────────────────────────────────────────
SentenceState GrammarEngine::detectState(const std::vector<TaggedToken>& tokens) {
    SentenceState s = {false, false, false};
    bool firstWordSeen = false;
    for (auto& t : tokens) {
        if (!firstWordSeen && t.token.type == TokenType::WORD) {
            String first = t.token.value; first.toLowerCase();
            if (isQuestionStarter(first) || isAuxiliaryWord(first)) s.isQuestion = true;
            firstWordSeen = true;
        }
        if (t.pos == POS::TIME_PAST) s.hasPastTimeWord = true;
        String v = t.token.value; v.toLowerCase();
        if (v == "don't"   || v == "doesn't" || v == "didn't"   ||
            v == "won't"   || v == "can't"   || v == "couldn't" ||
            v == "never"   || v == "wasn't"  || v == "weren't"  ||
            v == "hadn't"  || v == "wouldn't"|| v == "shouldn't") {
            s.hasNegAux = true;
        }
        if (t.token.type == TokenType::PUNCTUATION && t.token.value == "?") {
            s.isQuestion = true;
        }
    }
    return s;
}

// ─────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────
void GrammarEngine::correct(std::vector<TaggedToken>& tokens) {
    SentenceState state = detectState(tokens);

    // Cooperative rule order:
    // 1. local token fixes, 2. agreement, 3. movement/insertion, 4. tense/article cleanup.
    // Each later stage sees the normalized token text from earlier stages.
    ruleCapitalI(tokens);

    // Grammar rules do not fire on questions —
    // word order is inverted and agreement rules corrupt the output
    if (state.isQuestion) return;

    ruleWasWere(tokens, state);
    ruleNounBeAgreement(tokens, state);
    ruleTheirThereTheyre(tokens);
    ruleSubjectVerb(tokens, state);          // handles pronouns + nouns now
    rulePluralSubjectVerb(tokens, state);
    ruleAdverbPlacement(tokens, state);
    if (state.hasPastTimeWord) rulePastTense(tokens);
    ruleMissingArticle(tokens);
    ruleAvsAn(tokens);
    if (state.hasNegAux) ruleDoubleNegative(tokens, state);
}

// ─────────────────────────────────────────────
// Rule 1: Capitalize "i" → "I"
// ─────────────────────────────────────────────
void GrammarEngine::ruleCapitalI(std::vector<TaggedToken>& tokens) {
    for (auto& t : tokens) {
        if (t.token.type == TokenType::WORD && t.token.value == "i") {
            t.token.value = "I";
        }
    }
}

// ─────────────────────────────────────────────
// Rule 2: Was/Were
// Gate: subject must be CERTAIN
// ─────────────────────────────────────────────
void GrammarEngine::ruleWasWere(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    for (int i = 0; i < (int)tokens.size() - 1; i++) {
        if (tokens[i].pos != POS::PRONOUN) continue;
        if (!isCertain(tokens[i])) continue;

        String subj = tokens[i].token.value; subj.toLowerCase();

        for (int j = i + 1; j < (int)tokens.size(); j++) {
            if (tokens[j].pos == POS::ADVERB) continue;
            if (tokens[j].token.type == TokenType::PUNCTUATION) break;

            String v = tokens[j].token.value; v.toLowerCase();
            if (v == "was" || v == "were") {
                if (!isCertain(tokens[j])) break;
                if ((subj=="they"||subj=="we"||subj=="you") && v=="was")
                    tokens[j].token.value = "were";
                else if ((subj=="he"||subj=="she"||subj=="it") && v=="were")
                    tokens[j].token.value = "was";
                else if (subj=="i" && v=="were")
                    tokens[j].token.value = "was";
            }
            break;
        }
    }
}

// ─────────────────────────────────────────────
// Rule 3: Subject-Verb Agreement
// Now handles BOTH pronoun and noun subjects (item 2)
//
// Pronoun subjects: he/she/it + base verb → add s
// Noun subjects:    the dog go → the dog goes
//                   a cat run  → a cat runs
//
// Gates:
//   Pronoun subject → must be CERTAIN
//   Noun subject    → noun must be CERTAIN + preceded by ARTICLE/DETERMINER
//   Verb            → must be CONFIDENT or above
// ─────────────────────────────────────────────
void GrammarEngine::ruleNounBeAgreement(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    for (int i = 0; i < (int)tokens.size() - 2; i++) {
        if ((tokens[i].pos != POS::ARTICLE && tokens[i].pos != POS::DETERMINER) ||
            !isCertain(tokens[i]) || !canStartSubjectAt(tokens, i)) continue;

        int nounIdx = i + 1;
        while (nounIdx < (int)tokens.size() && tokens[nounIdx].pos == POS::ADJECTIVE) nounIdx++;
        if (nounIdx >= (int)tokens.size()) continue;
        if (tokens[nounIdx].pos != POS::NOUN && tokens[nounIdx].pos != POS::UNKNOWN) continue;

        String det = tokens[i].token.value; det.toLowerCase();
        String noun = tokens[nounIdx].token.value; noun.toLowerCase();
        SubjectNumber number = nounSubjectNumber(noun, det);
        if (number == SubjectNumber::UNKNOWN) continue;

        for (int j = nounIdx + 1; j < (int)tokens.size(); j++) {
            if (tokens[j].pos == POS::ADVERB) continue;
            if (tokens[j].token.type == TokenType::PUNCTUATION) break;
            String v = tokens[j].token.value; v.toLowerCase();
            if (number == SubjectNumber::SINGULAR) {
                if (v == "are") tokens[j].token.value = "is";
                else if (v == "were") tokens[j].token.value = "was";
                else if (v == "have") tokens[j].token.value = "has";
            } else if (number == SubjectNumber::PLURAL) {
                if (v == "is") tokens[j].token.value = "are";
                else if (v == "was") tokens[j].token.value = "were";
                else if (v == "has") tokens[j].token.value = "have";
            }
            break;
        }
    }
}

void GrammarEngine::ruleSubjectVerb(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    if (state.hasPastTimeWord) return;

    for (int i = 0; i < (int)tokens.size() - 1; i++) {
        bool subjectFound   = false;
        bool isSingSubject  = false;

        // Case A: pronoun subject (he/she/it)
        if (tokens[i].pos == POS::PRONOUN && isCertain(tokens[i])) {
            String subj = tokens[i].token.value; subj.toLowerCase();
            if (isSingularThirdPerson(subj)) {
                subjectFound  = true;
                isSingSubject = true;
            }
        }

        // Case B: noun subject — pattern is ARTICLE/DETERMINER + NOUN
        // "the dog", "a cat", "that bird"
        if (!subjectFound &&
            (tokens[i].pos == POS::ARTICLE || tokens[i].pos == POS::DETERMINER) &&
            isCertain(tokens[i]) &&
            canStartSubjectAt(tokens, i)) {

            // Next token must be a noun — accept CERTAIN noun OR UNKNOWN
            // (common nouns like "dog","cat" may not be in POS table → UNKNOWN)
            if (i + 1 < (int)tokens.size() &&
                (tokens[i+1].pos == POS::NOUN || tokens[i+1].pos == POS::UNKNOWN) &&
                (isConfidentOrAbove(tokens[i+1]) ||
                 tokens[i+1].confidence == POSConfidence::MEDIUM ||
                 tokens[i+1].pos == POS::UNKNOWN)) {

                String noun = tokens[i+1].token.value; noun.toLowerCase();
                // Only treat as singular if noun doesn't end in 's'
                // (crude but avoids "the dogs go" being wrongly fixed)
                String det = tokens[i].token.value; det.toLowerCase();
                if (nounSubjectNumber(noun, det) == SubjectNumber::SINGULAR) {
                    subjectFound  = true;
                    isSingSubject = true;
                    i++; // advance past noun so verb search starts after it
                }
            }
        }

        if (!subjectFound && canStartSubjectAt(tokens, i) &&
            (tokens[i].pos == POS::NOUN || tokens[i].pos == POS::UNKNOWN)) {
            String noun = tokens[i].token.value; noun.toLowerCase();
            if (isSingularProperNoun(noun)) {
                subjectFound = true;
                isSingSubject = true;
            }
        }

        if (!subjectFound || !isSingSubject) continue;

        // Now find the verb that follows this subject
        for (int j = i + 1; j < (int)tokens.size(); j++) {
            POS p = tokens[j].pos;
            String v = tokens[j].token.value;
            String vl = v; vl.toLowerCase();
            if (isAuxiliaryWord(vl)) break;
            if (p == POS::AUXILIARY) break;       // aux present — don't fix
            if (p == POS::ADVERB || p == POS::NEGATIVE) continue;
            if (p == POS::VERB || p == POS::UNKNOWN) {
                // Accept UNKNOWN here — common verbs like "go","run"
                // may not be in POS table after spell check
                // but isBaseVerb() will validate them
                String v = tokens[j].token.value;
                String vl = v; vl.toLowerCase();
                if (isBaseVerb(vl)) {
                    bool wasUpper = isupper(v[0]);
                    tokens[j].token.value = toThirdPersonSingular(vl);
                    if (wasUpper) tokens[j].token.value[0] = toupper(tokens[j].token.value[0]);
                }
                break;
            }
            // Stop if we hit another noun/pronoun or punctuation — different clause
            if (p == POS::NOUN || p == POS::PRONOUN ||
                tokens[j].token.type == TokenType::PUNCTUATION) break;
        }
    }
}

void GrammarEngine::rulePluralSubjectVerb(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    if (state.hasPastTimeWord) return;

    for (int i = 0; i < (int)tokens.size() - 1; i++) {
        bool pluralSubject = false;

        if (tokens[i].pos == POS::PRONOUN && isCertain(tokens[i])) {
            String subj = tokens[i].token.value; subj.toLowerCase();
            pluralSubject = isPluralOrFirstSecond(subj);
        }

        if (!pluralSubject &&
            (tokens[i].pos == POS::ARTICLE || tokens[i].pos == POS::DETERMINER) &&
            isCertain(tokens[i]) &&
            canStartSubjectAt(tokens, i) &&
            i + 1 < (int)tokens.size() &&
            (tokens[i+1].pos == POS::NOUN || tokens[i+1].pos == POS::UNKNOWN) &&
            (isConfidentOrAbove(tokens[i+1]) ||
             tokens[i+1].confidence == POSConfidence::MEDIUM ||
             tokens[i+1].pos == POS::UNKNOWN)) {
            String noun = tokens[i+1].token.value; noun.toLowerCase();
            String det = tokens[i].token.value; det.toLowerCase();
            pluralSubject = nounSubjectNumber(noun, det) == SubjectNumber::PLURAL;
            if (pluralSubject) i++;
        }

        if (!pluralSubject) continue;

        for (int j = i + 1; j < (int)tokens.size(); j++) {
            POS p = tokens[j].pos;
            String v = tokens[j].token.value;
            String vl = v; vl.toLowerCase();
            if (p == POS::AUXILIARY || isAuxiliaryWord(vl)) break;
            if (p == POS::ADVERB || p == POS::NEGATIVE) continue;
            if (p == POS::VERB || p == POS::UNKNOWN) {
                String base = toBaseFromThirdPerson(vl);
                if (!base.equals(vl)) {
                    bool wasUpper = isupper(v[0]);
                    tokens[j].token.value = base;
                    if (wasUpper) tokens[j].token.value[0] = toupper(tokens[j].token.value[0]);
                }
                break;
            }
            if (p == POS::NOUN || p == POS::PRONOUN ||
                tokens[j].token.type == TokenType::PUNCTUATION) break;
        }
    }
}

void GrammarEngine::ruleAdverbPlacement(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    if (state.hasPastTimeWord) return;

    for (int i = 0; i + 2 < (int)tokens.size(); i++) {
        bool subject = false;
        if (tokens[i].pos == POS::PRONOUN && isCertain(tokens[i])) {
            subject = true;
        } else if ((tokens[i].pos == POS::NOUN || tokens[i].pos == POS::UNKNOWN) &&
                   canStartSubjectAt(tokens, i)) {
            String noun = tokens[i].token.value; noun.toLowerCase();
            if (isSingularProperNoun(noun)) subject = true;
        }

        if (!subject) continue;
        if (tokens[i+1].pos != POS::ADVERB) continue;
        if (tokens[i+2].pos != POS::VERB && tokens[i+2].pos != POS::UNKNOWN) continue;

        String v = tokens[i+2].token.value; v.toLowerCase();
        if (!isBaseVerb(v) && toBaseFromThirdPerson(v).equals(v)) continue;

        TaggedToken adv = tokens[i+1];
        tokens.erase(tokens.begin() + i + 1);
        int insertAt = i + 2;
        if (i + 2 < (int)tokens.size() && tokens[i+2].token.type == TokenType::WORD) {
            String next = tokens[i+2].token.value; next.toLowerCase();
            if (next == "home" || next == "school" || next == "town" ||
                next == "outside" || next == "inside" || next == "there" ||
                next == "here") {
                insertAt = i + 3;
            }
        }
        if (insertAt > (int)tokens.size()) insertAt = tokens.size();
        tokens.insert(tokens.begin() + insertAt, adv);
        i = insertAt;
    }
}

void GrammarEngine::ruleTheirThereTheyre(std::vector<TaggedToken>& tokens) {
    for (int i = 0; i < (int)tokens.size(); i++) {
        if (tokens[i].token.type != TokenType::WORD) continue;

        String w = tokens[i].token.value; w.toLowerCase();
        if (w != "their" && w != "there" && w != "they're") continue;

        POS nextPos = POS::UNKNOWN;
        String nextWord = "";
        for (int j = i + 1; j < (int)tokens.size(); j++) {
            if (tokens[j].token.type == TokenType::WORD) {
                nextPos = tokens[j].pos;
                nextWord = tokens[j].token.value;
                nextWord.toLowerCase();
                break;
            }
        }

        String replacement = w;
        bool nextLooksPossessed = (nextWord=="dog" || nextWord=="cat" || nextWord=="bird" ||
                                   nextWord=="birds" || nextWord=="letter" || nextWord=="book" ||
                                   nextWord=="books" || nextWord=="name");
        if (nextLooksPossessed || nextPos == POS::NOUN || nextPos == POS::ADJECTIVE ||
            nextPos == POS::DETERMINER || nextPos == POS::UNKNOWN) {
            replacement = "their";
        } else if (nextPos == POS::VERB || nextPos == POS::AUXILIARY ||
                   nextWord == "not") {
            replacement = "they're";
        } else if (i == 0 || nextPos == POS::PREPOSITION) {
            replacement = "there";
        }

        if (!replacement.equals(w)) {
            bool wasUpper = isupper(tokens[i].token.value[0]);
            tokens[i].token.value = replacement;
            tokens[i].token.suffix = "";
            if (wasUpper) tokens[i].token.value[0] = toupper(tokens[i].token.value[0]);
        }
    }
}

// ─────────────────────────────────────────────
// Rule 4: Past Tense
// Gate: verb CONFIDENT or above
// ─────────────────────────────────────────────
void GrammarEngine::rulePastTense(std::vector<TaggedToken>& tokens) {
    for (auto& t : tokens) {
        if (t.pos != POS::VERB) continue;
        if (!isConfidentOrAbove(t)) continue;

        String v = t.token.value;
        String vl = v; vl.toLowerCase();
        if (isBaseVerb(vl)) {
            bool wasUpper = isupper(v[0]);
            t.token.value = toPastTense(vl);
            if (wasUpper) t.token.value[0] = toupper(t.token.value[0]);
        }
    }
}

// ─────────────────────────────────────────────
// Rule 5: Missing Article
// Gate: verb CERTAIN + noun CERTAIN
// ─────────────────────────────────────────────
void GrammarEngine::ruleMissingArticle(std::vector<TaggedToken>& tokens) {
    for (int i = 0; i + 1 < (int)tokens.size(); i++) {
        POS cur = tokens[i].pos;
        POS nxt = tokens[i+1].pos;

        bool afterVerb  = (cur == POS::VERB && isCertain(tokens[i]));
        bool beforeNoun = (nxt == POS::NOUN && isCertain(tokens[i+1]));

        if (afterVerb && nxt == POS::ADJECTIVE && isCertain(tokens[i+1])) {
            if (i + 2 < (int)tokens.size() &&
                tokens[i+2].pos == POS::NOUN && isCertain(tokens[i+2])) {
                beforeNoun = true;
            } else {
                beforeNoun = false;
            }
        }

        if (!afterVerb || !beforeNoun) continue;
        if (i > 0 && tokens[i-1].pos == POS::ARTICLE) continue;
        if (cur == POS::ARTICLE) continue;

        Token artTok(String("a"), TokenType::WORD);
        TaggedToken art(artTok, POS::ARTICLE, POSConfidence::CERTAIN);
        tokens.insert(tokens.begin() + i + 1, art);
        i++;
    }
}

// ─────────────────────────────────────────────
// Rule 6: a vs an
// ─────────────────────────────────────────────
void GrammarEngine::ruleAvsAn(std::vector<TaggedToken>& tokens) {
    for (int i = 0; i + 1 < (int)tokens.size(); i++) {
        if (tokens[i].pos != POS::ARTICLE) continue;
        String art = tokens[i].token.value; art.toLowerCase();
        if (art != "a" && art != "an") continue;

        String nxt = tokens[i+1].token.value;
        bool vowel = startsWithVowel(nxt);

        if (vowel  && art == "a")  tokens[i].token.value = "an";
        if (!vowel && art == "an") tokens[i].token.value = "a";
    }
}

// ─────────────────────────────────────────────
// Rule 7: Double Negative
// Gate: negative token CERTAIN
// Never touches standalone "no"
// ─────────────────────────────────────────────
void GrammarEngine::ruleDoubleNegative(std::vector<TaggedToken>& tokens, const SentenceState& state) {
    for (auto& t : tokens) {
        if (t.pos != POS::NEGATIVE) continue;
        if (!isCertain(t)) continue;

        String v = t.token.value; v.toLowerCase();
        if      (v == "nothing") t.token.value = "anything";
        else if (v == "nobody")  t.token.value = "anybody";
        else if (v == "nowhere") t.token.value = "anywhere";
        else if (v == "neither") t.token.value = "either";
    }
}

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────
bool GrammarEngine::isSingularThirdPerson(const String& w) {
    return (w=="he"||w=="she"||w=="it");
}

bool GrammarEngine::isPluralOrFirstSecond(const String& w) {
    return (w=="they"||w=="we"||w=="you");
}

bool GrammarEngine::isQuestionStarter(const String& w) {
    return (w=="who"||w=="what"||w=="where"||w=="when"||w=="why"||w=="how");
}

bool GrammarEngine::isAuxiliaryWord(const String& w) {
    return (w=="is"||w=="are"||w=="am"||w=="was"||w=="were"||
            w=="can"||w=="could"||w=="do"||w=="does"||w=="did"||
            w=="will"||w=="would"||w=="should"||w=="has"||
            w=="have"||w=="had");
}

bool GrammarEngine::canStartSubjectAt(const std::vector<TaggedToken>& tokens, int idx) {
    for (int i = idx - 1; i >= 0; i--) {
        if (tokens[i].token.type != TokenType::WORD) continue;
        POS p = tokens[i].pos;
        String w = tokens[i].token.value; w.toLowerCase();
        if (p == POS::VERB || p == POS::AUXILIARY || p == POS::PREPOSITION ||
            isAuxiliaryWord(w)) {
            return false;
        }
        return true;
    }
    return true;
}

bool GrammarEngine::isSingularProperNoun(const String& w) {
    static const char* names[] = {
        "hastings","james","charles","wales","paris",nullptr
    };
    for (int i = 0; names[i] != nullptr; i++) {
        if (w.equals(names[i])) return true;
    }
    return false;
}

SubjectNumber GrammarEngine::nounSubjectNumber(const String& noun, const String& determiner) {
    if (isSingularProperNoun(noun)) return SubjectNumber::SINGULAR;
    if (determiner == "a" || determiner == "an" || determiner == "each" || determiner == "every")
        return SubjectNumber::SINGULAR;
    if (determiner == "these" || determiner == "those")
        return SubjectNumber::PLURAL;
    if (noun.length() > 3 && noun.endsWith("s"))
        return SubjectNumber::PLURAL;
    if (!noun.endsWith("s"))
        return SubjectNumber::SINGULAR;
    return SubjectNumber::UNKNOWN;
}

bool GrammarEngine::startsWithVowel(const String& w) {
    if (!w.length()) return false;
    char c = tolower(w[0]);
    return (c=='a'||c=='e'||c=='i'||c=='o'||c=='u');
}

bool GrammarEngine::isBaseVerb(const String& v) {
    if (isAuxiliaryWord(v)) return false;
    if (v.length() <= 2) return isShortBaseVerb(v);
    if (v.endsWith("ing")) return false;
    if (v.endsWith("'t"))  return false;
    if (v.endsWith("n't")) return false;
    if (v.endsWith("ed"))  return false;
    if (v.length() > 3 && v.endsWith("s")) return false;

    static const char* notBase[] = {
        "went","ran","ate","came","took","gave","got","made","knew","saw",
        "said","thought","felt","left","sat","stood","bought","sold",
        "found","lost","told","brought","kept","wrote","built","grew",
        "met","paid","won","held","drove","slept","drank","swam","flew",
        "sent","taught","had","did","been","was","were","gone","done",
        "given","taken","read","shot","chose","broke","spoke","stole",
        "woke","wore","tore","bit","hid","rode","rose","shone","shook",
        "forgot","began","sang","rang","blew","threw","drew","fell",
        "hurt","cut","put","set","let","cost","hit","split","burst",
        nullptr
    };
    for (int i = 0; notBase[i] != nullptr; i++) {
        if (v.equals(notBase[i])) return false;
    }
    return true;
}

bool GrammarEngine::isShortBaseVerb(const String& v) {
    static const char* verbs[] = {
        "go","do","be",nullptr
    };
    for (int i = 0; verbs[i] != nullptr; i++) {
        if (v.equals(verbs[i])) return true;
    }
    return false;
}

String GrammarEngine::toThirdPersonSingular(const String& verb) {
    if (verb=="be")   return "is";
    if (verb=="have") return "has";
    if (verb=="do")   return "does";
    if (verb=="go")   return "goes";

    if (verb.endsWith("sh")||verb.endsWith("ch")||
        verb.endsWith("x") ||verb.endsWith("z") ||verb.endsWith("ss"))
        return verb + "es";

    if (verb.endsWith("y") && verb.length() > 1) {
        char before = verb[verb.length()-2];
        if (String("aeiou").indexOf(before) == -1)
            return verb.substring(0, verb.length()-1) + "ies";
    }
    return verb + "s";
}

String GrammarEngine::toBaseFromThirdPerson(const String& verb) {
    if (verb=="is" || verb=="has" || verb=="does") return verb;
    if (verb=="goes") return "go";
    if (verb.endsWith("ies") && verb.length() > 3)
        return verb.substring(0, verb.length()-3) + "y";
    if (verb.endsWith("es") && verb.length() > 3) {
        String root = verb.substring(0, verb.length()-2);
        if (root.endsWith("sh") || root.endsWith("ch") ||
            root.endsWith("x") || root.endsWith("z") || root.endsWith("ss"))
            return root;
    }
    if (verb.endsWith("s") && verb.length() > 3)
        return verb.substring(0, verb.length()-1);
    return verb;
}

String GrammarEngine::toPastTense(const String& v) {
    if (v=="go")      return "went";
    if (v=="run")     return "ran";
    if (v=="eat")     return "ate";
    if (v=="come")    return "came";
    if (v=="take")    return "took";
    if (v=="give")    return "gave";
    if (v=="get")     return "got";
    if (v=="make")    return "made";
    if (v=="know")    return "knew";
    if (v=="see")     return "saw";
    if (v=="say")     return "said";
    if (v=="think")   return "thought";
    if (v=="feel")    return "felt";
    if (v=="leave")   return "left";
    if (v=="sit")     return "sat";
    if (v=="stand")   return "stood";
    if (v=="buy")     return "bought";
    if (v=="sell")    return "sold";
    if (v=="find")    return "found";
    if (v=="lose")    return "lost";
    if (v=="tell")    return "told";
    if (v=="bring")   return "brought";
    if (v=="keep")    return "kept";
    if (v=="write")   return "wrote";
    if (v=="read")    return "read";
    if (v=="build")   return "built";
    if (v=="grow")    return "grew";
    if (v=="meet")    return "met";
    if (v=="pay")     return "paid";
    if (v=="win")     return "won";
    if (v=="hold")    return "held";
    if (v=="drive")   return "drove";
    if (v=="sleep")   return "slept";
    if (v=="drink")   return "drank";
    if (v=="swim")    return "swam";
    if (v=="fly")     return "flew";
    if (v=="send")    return "sent";
    if (v=="teach")   return "taught";
    if (v=="have")    return "had";
    if (v=="do")      return "did";
    if (v=="be")      return "was";
    if (v=="spend")   return "spent";
    if (v=="catch")   return "caught";
    if (v=="fight")   return "fought";
    if (v=="draw")    return "drew";
    if (v=="fall")    return "fell";
    if (v=="shoot")   return "shot";
    if (v=="choose")  return "chose";
    if (v=="break")   return "broke";
    if (v=="speak")   return "spoke";
    if (v=="steal")   return "stole";
    if (v=="wake")    return "woke";
    if (v=="wear")    return "wore";
    if (v=="tear")    return "tore";
    if (v=="bite")    return "bit";
    if (v=="hide")    return "hid";
    if (v=="ride")    return "rode";
    if (v=="rise")    return "rose";
    if (v=="shake")   return "shook";
    if (v=="forget")  return "forgot";
    if (v=="begin")   return "began";
    if (v=="sing")    return "sang";
    if (v=="ring")    return "rang";
    if (v=="blow")    return "blew";
    if (v=="throw")   return "threw";
    if (v=="show")    return "showed";
    if (v=="hit")     return "hit";
    if (v=="cut")     return "cut";
    if (v=="put")     return "put";
    if (v=="set")     return "set";
    if (v=="let")     return "let";
    if (v=="hurt")    return "hurt";
    if (v=="cost")    return "cost";
    if (v=="split")   return "split";
    if (v=="burst")   return "burst";

    if (v.endsWith("e")) return v + "d";

    if (v.endsWith("y") && v.length() > 1) {
        char before = v[v.length()-2];
        if (String("aeiou").indexOf(before) == -1)
            return v.substring(0, v.length()-1) + "ied";
    }

    int len = v.length();
    if (len >= 3) {
        char c1 = v[len-1], c2 = v[len-2], c3 = v[len-3];
        String vow = "aeiou", con = "bcdfghjklmnpqrstvwxyz";
        if (c1!='w'&&c1!='x'&&c1!='y'&&
            con.indexOf(c1)!=-1 && vow.indexOf(c2)!=-1 && con.indexOf(c3)!=-1)
            return v + String(c1) + "ed";
    }

    return v + "ed";
}
