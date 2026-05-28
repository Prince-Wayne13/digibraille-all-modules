#include "RegressionRunner.h"
#include "Normalizer.h"
#include "Tokenizer.h"
#include "Reconstructor.h"
#include "SpellChecker.h"
#include "POSTagger.h"
#include "GrammarEngine.h"
#include "QuestionDetector.h"
#include "PunctuationEngine.h"

// Uses same pipeline as main.cpp
// Defined as externs so we reuse the same instances
extern Normalizer        normalizer;
extern Tokenizer         tokenizer;
extern Reconstructor     reconstructor;
extern SpellChecker      spellChecker;
extern POSTagger         posTagger;
extern GrammarEngine     grammarEngine;
extern QuestionDetector  questionDetector;
extern PunctuationEngine punctuationEngine;

// Run the full pipeline on a string, return corrected output
static String runPipeline(const String& input) {
    String norm = normalizer.normalize(input);
    std::vector<Token> tokens = tokenizer.tokenize(norm);
    spellChecker.checkAndCorrect(tokens);
    std::vector<TaggedToken> tagged = posTagger.tag(tokens);
    grammarEngine.correct(tagged);
    bool isQ = questionDetector.isQuestion(tagged);
    punctuationEngine.correct(tagged, isQ);
    std::vector<Token> corrected;
    corrected.reserve(tagged.size());
    for (auto& t : tagged) corrected.push_back(t.token);
    return reconstructor.reconstruct(corrected);
}

void RegressionRunner::run() {
    Serial.println();
    Serial.println("╔═══════════════════════════════╗");
    Serial.println("║   REGRESSION TEST SUITE       ║");
    Serial.println("╚═══════════════════════════════╝");

    // Category tracking
    struct CatScore { const char* name; int pass; int total; };
    CatScore cats[] = {
        {"SPELL",     0, 0},
        {"SUBJ_VERB", 0, 0},
        {"CAPITAL_I", 0, 0},
        {"QUESTION",  0, 0},
    };
    const int NUM_CATS = 4;

    int totalPass = 0;
    int totalFail = 0;

    char inBuf[64], exBuf[64], catBuf[16];

    for (int i = 0; i < TEST_BANK_SIZE; i++) {
        // Read from PROGMEM
        const char* inPtr  = (const char*)pgm_read_ptr(&TEST_BANK[i].input);
        const char* exPtr  = (const char*)pgm_read_ptr(&TEST_BANK[i].expected);
        const char* catPtr = (const char*)pgm_read_ptr(&TEST_BANK[i].category);

        strncpy_P(inBuf,  inPtr,  sizeof(inBuf)  - 1); inBuf[sizeof(inBuf)-1]   = '\0';
        strncpy_P(exBuf,  exPtr,  sizeof(exBuf)  - 1); exBuf[sizeof(exBuf)-1]   = '\0';
        strncpy_P(catBuf, catPtr, sizeof(catBuf) - 1); catBuf[sizeof(catBuf)-1] = '\0';

        String input    = String(inBuf);
        String expected = String(exBuf);
        String category = String(catBuf);

        // Run pipeline
        String output = runPipeline(input);

        bool passed = output.equals(expected);

        // Update category score
        for (int c = 0; c < NUM_CATS; c++) {
            if (category.equals(cats[c].name)) {
                cats[c].total++;
                if (passed) cats[c].pass++;
                break;
            }
        }

        if (passed) {
            totalPass++;
        } else {
            totalFail++;
            // Print failures only — keeps output clean
            Serial.print("FAIL [");
            Serial.print(catBuf);
            Serial.print("] \"");
            Serial.print(input);
            Serial.print("\" → got \"");
            Serial.print(output);
            Serial.print("\" expected \"");
            Serial.print(expected);
            Serial.println("\"");
        }
    }

    // Summary per category
    Serial.println("───────────────────────────────");
    Serial.println("Category Results:");
    for (int c = 0; c < NUM_CATS; c++) {
        int pct = cats[c].total > 0 ?
                  (cats[c].pass * 100) / cats[c].total : 0;

        Serial.print("  ");
        Serial.print(cats[c].name);
        Serial.print(": ");
        Serial.print(cats[c].pass);
        Serial.print("/");
        Serial.print(cats[c].total);
        Serial.print(" (");
        Serial.print(pct);
        Serial.print("%)");

        // Show if target met
        int target = 85;
        if (String(cats[c].name).equals("CAPITAL_I")) target = 99;
        if (String(cats[c].name).equals("SUBJ_VERB")) target = 90;

        if (pct >= target) Serial.println(" ✓");
        else               Serial.println(" ✗ needs work");
    }

    Serial.println("───────────────────────────────");
    Serial.print("TOTAL: ");
    Serial.print(totalPass);
    Serial.print("/");
    Serial.print(TEST_BANK_SIZE);
    Serial.print(" passed (");
    Serial.print((totalPass * 100) / TEST_BANK_SIZE);
    Serial.println("%)");
    Serial.println("═══════════════════════════════");
    Serial.println();
}
