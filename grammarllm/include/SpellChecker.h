#pragma once
#include <Arduino.h>
#include <pgmspace.h>
#include "Tokenizer.h"
#include <vector>
#include "dictionary.h"

struct SpellResult {
    String word;
    float  confidence;
    bool   corrected;
};

static const char* PROTECTED_WORDS[] = {
    "a","an","the","i","in","on","at","to","do","go","be","by","my",
    "he","she","we","it","me","us","no","so","up","or","if","of","as",
    "is","am","are","was","and","but","for","not","yet","nor","its",
    "him","her","his","our","you","had","has","did","got","let","put",
    "run","sat","set","cut","hit","bit","fit","fix","mix","box","tax",
    "buy","try","fly","pay","say","may","lay","eat","ask","add","ago",
    "air","arm","art","bad","bag","bar","bay","bed","big","bow","boy",
    "bus","can","cap","car","cat","cop","cow","cry","cup","day","die",
    "dig","dog","dot","dry","due","ear","egg","end","eye","fan","far",
    "fat","few","fun","gap","gas","god","gun","gut","had","ham","hat",
    "hay","hen","hip","hog","hop","hot","hug","hut","ice","ill","inn",
    "jam","jar","jaw","jet","job","jog","joy","jug","keg","key","kid",
    "kit","lab","lag","lap","law","leg","lid","lip","lit","log","lot",
    "low","mad","map","mat","mob","mop","mud","mug","nap","net","new",
    "nod","now","nun","nut","oak","odd","old","one","opt","out","own",
    "pad","pan","paw","pea","peg","pen","pet","pie","pig","pin","pit",
    "pod","pop","pot","pub","pun","pup","rag","ram","rap","rat","raw",
    "ray","red","ref","rep","rid","rig","rim","rip","rob","rod","rot",
    "row","rub","rug","rut","sad","sap","saw","sea","sew","shy","sin",
    "sip","sir","six","ski","sky","sob","son","sow","soy","spy","sub",
    "sum","sun","tab","tan","tap","tar","tea","ten","tie","tin","tip",
    "toe","ton","too","top","toy","tub","tug","two","use","van","vat",
    "via","vow","wad","war","wax","web","wet","who","why","wig","win",
    "wit","woe","won","woo","wow","yam","yes","yet","zip","zoo","oh",
    "ow","uh","um","ok","hi","hey","ouch","duh","hmm","yep","nah",
    nullptr
};

#define SPELL_CONFIDENCE_THRESHOLD 0.65f

class SpellChecker {
public:
    void        checkAndCorrect(std::vector<Token>& tokens);
    SpellResult correctWord(const String& word);
    SpellResult correctWordWithContext(const String& word, const String& leftWord);

private:
    bool        isProtected(const String& word);
    bool        inDictionary(const String& word);
    SpellResult checkHotfix(const String& word);
    SpellResult findBestCandidate(const String& word, const String& leftWord);

    // Weighted Levenshtein — float matrix
    float weightedLevenshtein(const char* a, const char* b);

    // Confidence scoring
    float computeConfidence(const String& input, const String& candidate,
                            float wdist, uint8_t tier);
    float firstLetterBonus(char a, char b);
    float prefixBonus(const String& input, const String& candidate);
    bool  firstLetterClose(char a, char b);

    // Frequency tier lookup
    uint8_t getDictionaryTier(int index);

    // Float matrix for weighted Levenshtein — 22×22 × 4 bytes = 1.9KB
    float   _fmatrix[22][22];
    char    _buf[32];
};