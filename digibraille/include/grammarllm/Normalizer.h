#pragma once
#include <Arduino.h>

class Normalizer {
public:
    String normalize(const String& input);

private:
    String fixApostrophes(const String& input);
    String collapseRepeats(const String& input);
    String splitFusedWords(const String& input);  // item 5
};