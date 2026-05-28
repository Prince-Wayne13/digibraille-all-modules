#include "POSTagger.h"
#include "pos_table.h"
#include "pos_bigram.h"

std::vector<TaggedToken> POSTagger::tag(const std::vector<Token>& tokens) {
    std::vector<TaggedToken> tagged;
    for (auto& tok : tokens) {
        if (tok.type == TokenType::WORD) {
            String lower = tok.value;
            lower.toLowerCase();
            POSConfidence conf = POSConfidence::CERTAIN;
            POS pos = classifyWord(lower, conf);
            tagged.push_back(TaggedToken(tok, pos, conf));
        } else {
            tagged.push_back(TaggedToken(tok, POS::UNKNOWN, POSConfidence::UNSURE));
        }
    }
    smoothWithBigrams(tagged);
    return tagged;
}

POS POSTagger::classifyWord(const String& word, POSConfidence& outConf) {
    char buf[32];
    for (int i = 0; i < POS_TABLE_SIZE; i++) {
        const char* ptr = (const char*)pgm_read_ptr(&POS_TABLE[i].word);
        // Fast first-char reject before full copy
        char fc = pgm_read_byte(ptr);
        if (fc != word[0]) continue;

        strncpy_P(buf, ptr, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        if (word.equals(buf)) {
            outConf = POSConfidence::CERTAIN; // table lookup = certain
            POS p;
            memcpy_P(&p, &POS_TABLE[i].pos, sizeof(POS));
            return p;
        }
    }
    // Not in table — guess by suffix
    return guessBySuffix(word, outConf);
}

POS POSTagger::guessBySuffix(const String& word, POSConfidence& outConf) {
    int len = word.length();
    if (len < 3) {
        outConf = POSConfidence::UNSURE;
        return POS::UNKNOWN;
    }

    // CONFIDENT suffixes — very reliable signals
    if (word.endsWith("ing")) { outConf = POSConfidence::CONFIDENT;   return POS::VERB; }
    if (word.endsWith("tion")){ outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("sion")){ outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ness")){ outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ment")){ outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ship")){ outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ity")) { outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ism")) { outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ist")) { outConf = POSConfidence::CONFIDENT;   return POS::NOUN; }
    if (word.endsWith("ify")) { outConf = POSConfidence::CONFIDENT;   return POS::VERB; }
    if (word.endsWith("ize")) { outConf = POSConfidence::CONFIDENT;   return POS::VERB; }
    if (word.endsWith("ise")) { outConf = POSConfidence::CONFIDENT;   return POS::VERB; }
    if (word.endsWith("ate")) { outConf = POSConfidence::CONFIDENT;   return POS::VERB; }

    // CONFIDENT adverb
    if (word.endsWith("ly"))  { outConf = POSConfidence::CONFIDENT;   return POS::ADVERB; }

    // MEDIUM confidence suffixes — reliable but have exceptions
    if (word.endsWith("ed"))  { outConf = POSConfidence::MEDIUM; return POS::VERB; }
    if (word.endsWith("ful")) { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("less")){ outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("ous")) { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("ive")) { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("able")){ outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("ible")){ outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("ous")) { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("al"))  { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("ic"))  { outConf = POSConfidence::MEDIUM; return POS::ADJECTIVE; }
    if (word.endsWith("er"))  { outConf = POSConfidence::MEDIUM; return POS::NOUN; }
    if (word.endsWith("or"))  { outConf = POSConfidence::MEDIUM; return POS::NOUN; }

    // UNSURE — — very ambiguous
    outConf = POSConfidence::UNSURE;
    return POS::UNKNOWN;
}

uint8_t POSTagger::transitionScore(POS left, POS right) {
    return pgm_read_byte(&POS_BIGRAM[(int)left][(int)right]);
}

void POSTagger::smoothWithBigrams(std::vector<TaggedToken>& tagged) {
    for (int i = 0; i < (int)tagged.size(); i++) {
        if (tagged[i].token.type != TokenType::WORD) continue;
        if (tagged[i].confidence == POSConfidence::CERTAIN) continue;

        POS left = POS::UNKNOWN;
        POS right = POS::UNKNOWN;

        for (int l = i - 1; l >= 0; l--) {
            if (tagged[l].token.type == TokenType::WORD) {
                left = tagged[l].pos;
                break;
            }
        }
        for (int r = i + 1; r < (int)tagged.size(); r++) {
            if (tagged[r].token.type == TokenType::WORD) {
                right = tagged[r].pos;
                break;
            }
        }

        POS candidates[] = {tagged[i].pos, POS::NOUN, POS::VERB, POS::ADJECTIVE, POS::ADVERB};
        POS bestPos = tagged[i].pos;
        int bestScore = -1;
        int currentScore = transitionScore(left, tagged[i].pos) + transitionScore(tagged[i].pos, right);

        for (int c = 0; c < 5; c++) {
            POS p = candidates[c];
            int score = transitionScore(left, p) + transitionScore(p, right);
            if (score > bestScore) {
                bestScore = score;
                bestPos = p;
            }
        }

        if (bestScore >= currentScore + 60) {
            tagged[i].pos = bestPos;
            tagged[i].confidence = POSConfidence::MEDIUM;
        }
    }
}
