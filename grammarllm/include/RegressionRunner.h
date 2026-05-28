#pragma once
#include <Arduino.h>
#include "RegressionTest.h"

class RegressionRunner {
public:
    void run(); // runs all tests, prints report to Serial

private:
    // Score per category
    struct CategoryScore {
        const char* name;
        int passed;
        int total;
    };
};
