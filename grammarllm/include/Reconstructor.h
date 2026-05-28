#pragma once
#include <Arduino.h>
#include <vector>
#include "Tokenizer.h"

class Reconstructor {
public:
    String reconstruct(const std::vector<Token>& tokens);

private:
    bool   isAttachLeft(const String& punct);
    String capitalizeFirst(const String& s);
};