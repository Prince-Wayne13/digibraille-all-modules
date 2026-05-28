#include "SpellChecker.h"
#include "dictionary.h"
#include "hotfixes.h"

static bool leftSuggestsNoun(const String& leftWord);

// ─────────────────────────────────────────────
// Weighted Levenshtein costs
// Human errors are not equal — deletions most common
// ─────────────────────────────────────────────
#define COST_DELETE    0.8f   // missing letter — most common typo
#define COST_INSERT    1.2f   // extra letter — less common
#define COST_TRANSPOSE 0.9f   // swapped letters e.g. ei→ie
#define COST_SUBSTITUTE 1.0f  // wrong letter

// ─────────────────────────────────────────────
// Public: correct all WORD tokens in place
// ─────────────────────────────────────────────
void SpellChecker::checkAndCorrect(std::vector<Token>& tokens) {
    for (int i = 0; i < (int)tokens.size(); i++) {
        if (tokens[i].type != TokenType::WORD) continue;

        String lower = tokens[i].value;
        lower.toLowerCase();

        if (isProtected(lower)) continue;

        // Pass left neighbor POS hint for context reranking (item 4)
        // We pass the raw token — POSTagger not available here so we
        // use a simple left-word check
        String leftWord = "";
        for (int j = i - 1; j >= 0; j--) {
            if (tokens[j].type == TokenType::WORD) {
                leftWord = tokens[j].value;
                leftWord.toLowerCase();
                break;
            }
        }

        SpellResult result = correctWordWithContext(lower, leftWord);

        if (result.corrected && result.confidence >= SPELL_CONFIDENCE_THRESHOLD) {
            bool wasCapital = isupper(tokens[i].value[0]);
            tokens[i].value = result.word;
            if (wasCapital && tokens[i].value.length() > 0) {
                tokens[i].value[0] = toupper(tokens[i].value[0]);
            }
        }
    }
}

// ─────────────────────────────────────────────
// Public: correct with left-word context
// ─────────────────────────────────────────────
SpellResult SpellChecker::correctWordWithContext(const String& word, const String& leftWord) {
    SpellResult noChange = {word, 1.0f, false};

    if (isProtected(word))   return noChange;
    if (inDictionary(word))  return noChange;

    // Stage 2: hotfix — known pairs, highest confidence
    SpellResult hf = checkHotfix(word);
    if (hf.corrected) return hf;

    if (word.length() <= 2)  return noChange;

    if (word.length() > 3 && word.endsWith("s") && leftSuggestsNoun(leftWord)) {
        return noChange;
    }

    // Stage 3: candidate search with context
    if (word.length() >= 3) {
        return findBestCandidate(word, leftWord);
    }

    return noChange;
}

// Legacy wrapper
SpellResult SpellChecker::correctWord(const String& word) {
    return correctWordWithContext(word, "");
}

// ─────────────────────────────────────────────
// Protected word check
// ─────────────────────────────────────────────
bool SpellChecker::isProtected(const String& word) {
    for (int i = 0; PROTECTED_WORDS[i] != nullptr; i++) {
        if (word.equals(PROTECTED_WORDS[i])) return true;
    }
    return false;
}

// ─────────────────────────────────────────────
// Dictionary exact lookup via LETTER_INDEX jump
// ─────────────────────────────────────────────
bool SpellChecker::inDictionary(const String& word) {
    if (!word.length()) return false;
    char first = tolower(word[0]);
    if (first < 'a' || first > 'z') return false;

    int idx   = first - 'a';
    int lo    = pgm_read_word(&LETTER_INDEX[idx]);
    int hi    = pgm_read_word(&LETTER_INDEX[idx + 1]) - 1;

    // Binary search within letter section
    // Dictionary is sorted A→Z so binary search is valid
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const char* ptr = (const char*)pgm_read_ptr(&DICTIONARY[mid]);
        strncpy_P(_buf, ptr, sizeof(_buf) - 1);
        _buf[sizeof(_buf) - 1] = '\0';

        int cmp = word.compareTo(_buf);
        if (cmp == 0) return true;
        else if (cmp < 0) hi = mid - 1;
        else              lo = mid + 1;
    }
    return false;
}

// ─────────────────────────────────────────────
// Hotfix lookup
// ─────────────────────────────────────────────
SpellResult SpellChecker::checkHotfix(const String& word) {
    static const char* validWords[] = {
        "their","there","your","were","its","alright","anyway",
        "everyday","anymore","heap","bunch","kind","sort","type",
        "variety","number","amount","piece","bit","part","section",
        "portion","fraction","percentage","ratio","rate","speed",
        "velocity","acceleration","force","energy","power","work",
        "heat","light","sound","wave","particle","atom","molecule",
        "element","compound","solution","mixture","reaction","process",
        "system","structure","function","method","technique","approach",
        "strategy","plan","design","pattern","model","theory","hypothesis",
        "experiment","observation","measurement","calculation","analysis",
        "synthesis","evaluation","assessment","judgment","decision","choice",
        "option","alternative","possibility","probability","chance","risk",
        "danger","threat","opportunity","advantage","disadvantage","benefit",
        "cost","price","value","worth","quality","quantity","size","shape",
        "color","texture","weight","height","length","width","depth","volume",
        "area","space","distance","direction","position","location","place",
        "spot","point","line","curve","angle","circle","square","rectangle",
        "triangle","polygon","sphere","cube","cylinder","cone","pyramid",
        "prism","ellipsoid","paraboloid","hyperboloid","torus","surface",
        "edge","vertex","face","side","corner","base","top","bottom",
        nullptr
    };
    for (int i = 0; validWords[i] != nullptr; i++) {
        if (word.equals(validWords[i])) return {word, 1.0f, false};
    }

    char firstChar = word[0];
    for (int i = 0; i < HOTFIX_SIZE; i++) {
        const char* wPtr = (const char*)pgm_read_ptr(&HF_WRONG[i]);
        char fc = pgm_read_byte(wPtr);
        if (fc != firstChar) continue;

        strncpy_P(_buf, wPtr, sizeof(_buf) - 1);
        _buf[sizeof(_buf) - 1] = '\0';

        if (word.equals(_buf)) {
            const char* rPtr = (const char*)pgm_read_ptr(&HF_RIGHT[i]);
            char rightBuf[32];
            strncpy_P(rightBuf, rPtr, sizeof(rightBuf) - 1);
            rightBuf[sizeof(rightBuf) - 1] = '\0';
            return {String(rightBuf), 0.97f, true};
        }
    }
    return {word, 0.0f, false};
}

// ─────────────────────────────────────────────
// Weighted Levenshtein (item 1)
// Different costs for different error types
// Uses float matrix — still static, no heap
// ─────────────────────────────────────────────
float SpellChecker::weightedLevenshtein(const char* a, const char* b) {
    int la = strlen(a); if (la > 21) la = 21;
    int lb = strlen(b); if (lb > 21) lb = 21;

    for (int i = 0; i <= la; i++) _fmatrix[i][0] = (float)i;
    for (int j = 0; j <= lb; j++) _fmatrix[0][j] = (float)j;

    for (int i = 1; i <= la; i++) {
        for (int j = 1; j <= lb; j++) {
            char ca = tolower(a[i-1]);
            char cb = tolower(b[j-1]);

            if (ca == cb) {
                // Exact match — no cost
                _fmatrix[i][j] = _fmatrix[i-1][j-1];
            } else {
                // Deletion: a has extra char (flowrs vs flowers — 'e' missing in a)
                float del = _fmatrix[i-1][j] + COST_DELETE;
                // Insertion: b has extra char
                float ins = _fmatrix[i][j-1] + COST_INSERT;
                // Substitution: wrong letter
                float sub = _fmatrix[i-1][j-1] + COST_SUBSTITUTE;

                // Transposition: check if adjacent chars are swapped (ei→ie)
                float best = del < ins ? del : ins;
                best = best < sub ? best : sub;

                if (i > 1 && j > 1 &&
                    tolower(a[i-1]) == tolower(b[j-2]) &&
                    tolower(a[i-2]) == tolower(b[j-1])) {
                    float trans = _fmatrix[i-2][j-2] + COST_TRANSPOSE;
                    best = best < trans ? best : trans;
                }

                _fmatrix[i][j] = best;
            }
        }
    }
    return _fmatrix[la][lb];
}

// ─────────────────────────────────────────────
// Context hint: what POS does left word suggest
// for the word we are correcting
// ─────────────────────────────────────────────
static bool leftSuggestsNoun(const String& leftWord) {
    // After these words, next word is almost certainly a noun
    static const char* articleWords[] = {
        "a","an","the","my","his","her","its","our","their","your",
        "this","that","these","those","some","any","each","every",
        nullptr
    };
    for (int i = 0; articleWords[i] != nullptr; i++) {
        if (leftWord.equals(articleWords[i])) return true;
    }
    return false;
}

static bool leftSuggestsVerb(const String& leftWord) {
    static const char* pronouns[] = {
        "i","he","she","they","we","you","it",nullptr
    };
    for (int i = 0; pronouns[i] != nullptr; i++) {
        if (leftWord.equals(pronouns[i])) return true;
    }
    return false;
}

// ─────────────────────────────────────────────
// Find best candidate — A→B→C→D pipeline
// Now with weighted Levenshtein + frequency tiers
// + context reranking (items 1, 2, 3, 4)
// ─────────────────────────────────────────────
SpellResult SpellChecker::findBestCandidate(const String& word, const String& leftWord) {
    float  bestConf = SPELL_CONFIDENCE_THRESHOLD;
    String bestWord = "";
    int    wlen     = word.length();
    char   wFirst   = tolower(word[0]);

    bool wantNoun = leftSuggestsNoun(leftWord);
    bool wantVerb = leftSuggestsVerb(leftWord);

    // Scan current letter + adjacent close letters
    for (int letterOff = -1; letterOff <= 1; letterOff++) {
        char targetLetter = wFirst + letterOff;
        if (targetLetter < 'a' || targetLetter > 'z') continue;
        if (letterOff != 0 && !firstLetterClose(wFirst, targetLetter)) continue;

        int idx   = targetLetter - 'a';
        int start = pgm_read_word(&LETTER_INDEX[idx]);
        int end   = pgm_read_word(&LETTER_INDEX[idx + 1]);

        for (int i = start; i < end; i++) {
            const char* ptr = (const char*)pgm_read_ptr(&DICTIONARY[i]);

            // Stage B filter 1: length proximity
            int dlen = 0;
            const char* p = ptr;
            while (pgm_read_byte(p++) && dlen < 25) dlen++;
            if (abs(dlen - wlen) > 2) continue;

            // Stage B filter 2: first letter close
            char d0 = pgm_read_byte(ptr);
            if (!firstLetterClose(wFirst, d0)) continue;

            // Full copy for scoring
            strncpy_P(_buf, ptr, sizeof(_buf) - 1);
            _buf[sizeof(_buf) - 1] = '\0';

            // Stage C: weighted Levenshtein
            float wdist = weightedLevenshtein(word.c_str(), _buf);

            // Max weighted distance threshold
            float maxWDist = (wlen >= 6) ? 2.0f : 1.2f;
            if (wdist > maxWDist) continue;

            // Read frequency tier from dictionary (item 2)
            uint8_t tier = getDictionaryTier(i);

            // Stage D: confidence scoring
            float conf = computeConfidence(word, String(_buf), wdist, tier);

            // Item 4: context reranking bonus
            // If left word suggests noun and this candidate looks like noun → boost
            // We approximate by checking if it's a known common noun ending
            if (wantNoun) {
                String cand = String(_buf);
                if (cand.endsWith("er") || cand.endsWith("ion") ||
                    cand.endsWith("ness") || cand.endsWith("ment") ||
                    cand.endsWith("s"))  {
                    conf *= 1.10f;
                }
            }
            if (wantVerb) {
                String cand = String(_buf);
                if (cand.endsWith("ing") || cand.endsWith("ed") ||
                    cand.endsWith("ize") || cand.endsWith("ify")) {
                    conf *= 1.08f;
                }
            }

            if (conf > 1.0f) conf = 1.0f;

            if (conf > bestConf) {
                bestConf = conf;
                bestWord = String(_buf);
                if (wdist <= 0.9f && conf > 0.92f) goto done;
            }
        }
    }

done:
    if (bestWord.length() > 0) return {bestWord, bestConf, true};
    return {word, 0.0f, false};
}

// ─────────────────────────────────────────────
// Frequency tier lookup (item 2)
// Reads tier from DICTIONARY_TIERS array in Flash
// Returns 1 (most common) to 3 (least common)
// ─────────────────────────────────────────────
uint8_t SpellChecker::getDictionaryTier(int index) {
    if (index < 0 || index >= DICTIONARY_SIZE) return 3;
    return pgm_read_byte(&DICTIONARY_TIERS[index]);
}

// ─────────────────────────────────────────────
// Confidence formula (item 3 — improved)
//
// Base: weighted distance normalized by word length
//       deletion (0.8) gives higher base than substitution (1.0)
// × firstLetterBonus
// × prefixBonus        — rewards matching prefix heavily
// × tierBonus          — common words score higher at equal distance
// ─────────────────────────────────────────────
float SpellChecker::computeConfidence(const String& input,
                                       const String& candidate,
                                       float wdist,
                                       uint8_t tier) {
    // Item 6: LD frequency tie-breaker formula
    // FinalScore = LD - (frequency/255)
    // Lower score = better candidate
    // Convert tier to approximate frequency:
    //   tier 1 → freq ~220 (very common)
    //   tier 2 → freq ~128 (common)
    //   tier 3 → freq ~40  (rare)
    float freq = (tier == 1) ? 220.0f : (tier == 2) ? 128.0f : 40.0f;
    float freqNorm = freq / 255.0f;            // 0.0 - 1.0
    float ldScore  = wdist - freqNorm;          // lower = better candidate

    // Convert to confidence (0-1, higher = better)
    // Normalize against word length so short words aren't penalised
    float base = 1.0f - (ldScore / (float)input.length());
    if (base < 0.0f) base = 0.0f;

    // Apply structural bonuses on top
    float flb  = firstLetterBonus(input[0], candidate[0]);
    float pb   = prefixBonus(input, candidate);

    float conf = base * flb * pb;
    if (conf > 1.0f) conf = 1.0f;
    if (conf < 0.0f) conf = 0.0f;
    return conf;
}

float SpellChecker::firstLetterBonus(char a, char b) {
    a = tolower(a); b = tolower(b);
    if (a == b) return 1.00f;
    const char* groups[] = {
        "aeiou","bp","dt","cgkq","mn","sz","fv","lr","wy",nullptr
    };
    for (int i = 0; groups[i] != nullptr; i++) {
        if (strchr(groups[i], a) && strchr(groups[i], b)) return 0.75f;
    }
    return 0.50f;
}

float SpellChecker::prefixBonus(const String& input, const String& candidate) {
    int matches = 0;
    int checkLen = min((int)input.length(), (int)candidate.length());
    checkLen = min(checkLen, 4); // check up to 4 chars for better discrimination

    for (int i = 0; i < checkLen; i++) {
        if (tolower(input[i]) == tolower(candidate[i])) matches++;
        else break;
    }

    // Suffix check — last 2 chars
    int il = input.length(), cl = candidate.length();
    bool suffixMatch = (il >= 2 && cl >= 2 &&
                        tolower(input[il-1]) == tolower(candidate[cl-1]) &&
                        tolower(input[il-2]) == tolower(candidate[cl-2]));

    // Item 3: reward prefix match more heavily than before
    switch (matches) {
        case 4: return suffixMatch ? 1.20f : 1.12f;
        case 3: return suffixMatch ? 1.10f : 1.05f;
        case 2: return suffixMatch ? 1.00f : 0.92f;
        case 1: return suffixMatch ? 0.88f : 0.78f;
        default: return suffixMatch ? 0.75f : 0.60f;
    }
}

bool SpellChecker::firstLetterClose(char a, char b) {
    a = tolower(a); b = tolower(b);
    if (a == b) return true;
    const char* groups[] = {
        "aeiou","bp","dt","cgkq","mn","sz","fv","lr","wy",nullptr
    };
    for (int i = 0; groups[i] != nullptr; i++) {
        if (strchr(groups[i], a) && strchr(groups[i], b)) return true;
    }
    return false;
}
