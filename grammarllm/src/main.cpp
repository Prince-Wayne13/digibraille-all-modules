#include <Arduino.h>
#include "Normalizer.h"
#include "Tokenizer.h"
#include "Reconstructor.h"
#include "SpellChecker.h"
#include "POSTagger.h"
#include "GrammarEngine.h"
#include "QuestionDetector.h"
#include "PunctuationEngine.h"
#include "RegressionRunner.h"

Normalizer        normalizer;
Tokenizer         tokenizer;
Reconstructor     reconstructor;
SpellChecker      spellChecker;
POSTagger         posTagger;
GrammarEngine     grammarEngine;
QuestionDetector  questionDetector;
PunctuationEngine punctuationEngine;
RegressionRunner  regressionRunner;

bool debugMode = false;
uint32_t sessionInputs = 0;
uint32_t sessionCorrections = 0;

void processInput(const String& input) {
    uint32_t t0 = millis();
    Serial.println("─────────────────────────────");
    Serial.print("Input:  ");
    Serial.println(input);

    String norm = normalizer.normalize(input);
    uint32_t tNorm = millis();
    std::vector<Token> tokens = tokenizer.tokenize(norm);
    uint32_t tTok = millis();
    spellChecker.checkAndCorrect(tokens);
    uint32_t tSpell = millis();
    std::vector<TaggedToken> tagged = posTagger.tag(tokens);
    uint32_t tPos = millis();
    grammarEngine.correct(tagged);
    uint32_t tGrammar = millis();
    bool isQ = questionDetector.isQuestion(tagged);
    punctuationEngine.correct(tagged, isQ);
    uint32_t tPunct = millis();
    std::vector<Token> corrected;
    corrected.reserve(tagged.size());
    for (auto& t : tagged) corrected.push_back(t.token);
    String output = reconstructor.reconstruct(corrected);
    uint32_t tRecon = millis();
    sessionInputs++;
    if (!output.equals(input)) sessionCorrections++;

    Serial.print("Output: ");
    Serial.println(output);
    if (debugMode) {
        Serial.print("Debug: question=");
        Serial.print(isQ ? "yes" : "no");
        Serial.print(" tokens=");
        Serial.println((int)tokens.size());
        Serial.print("Timing ms: norm=");
        Serial.print(tNorm - t0);
        Serial.print(" tok=");
        Serial.print(tTok - tNorm);
        Serial.print(" spell=");
        Serial.print(tSpell - tTok);
        Serial.print(" pos=");
        Serial.print(tPos - tSpell);
        Serial.print(" grammar=");
        Serial.print(tGrammar - tPos);
        Serial.print(" punct=");
        Serial.print(tPunct - tGrammar);
        Serial.print(" recon=");
        Serial.println(tRecon - tPunct);
        Serial.print("Stats: inputs=");
        Serial.print(sessionInputs);
        Serial.print(" changed=");
        Serial.println(sessionCorrections);
    }
    Serial.println("─────────────────────────────");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("=============================");
    Serial.println("  ESP32 Grammar Corrector");
    Serial.println("=============================");

    // Run regression tests on boot
    regressionRunner.run();

    Serial.println("Type a sentence and press Enter.");
    Serial.println("Type 'test' to re-run regression.");
    Serial.println("Type 'debug' to toggle logs, 'stats' for session counts.");
    Serial.println();
}

String inputBuffer = "";

void loop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            inputBuffer.trim();
            if (inputBuffer.length() > 0) {
                if (inputBuffer.equalsIgnoreCase("test")) {
                    regressionRunner.run();
                } else if (inputBuffer.equalsIgnoreCase("debug")) {
                    debugMode = !debugMode;
                    Serial.print("Debug mode: ");
                    Serial.println(debugMode ? "on" : "off");
                } else if (inputBuffer.equalsIgnoreCase("stats")) {
                    Serial.print("Inputs: ");
                    Serial.print(sessionInputs);
                    Serial.print(" changed: ");
                    Serial.println(sessionCorrections);
                } else {
                    processInput(inputBuffer);
                }
                inputBuffer = "";
            }
        } else if (c != '\r') {
            inputBuffer += c;
        }
    }
}
