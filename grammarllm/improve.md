1.  DICTIONARY_TIERS[688]       ✅ done — all 2s for now
2.  Noun subject-verb           ✅ done — GrammarEngine.cpp
3.  Binary search               ✅ done — SpellChecker.cpp
4.  Regression test suite       ✅ done — RegressionRunner

5.  Fused word splitter         ✅ done — Normalizer.cpp
                                "iam"→"i am", "iknow"→"i know"
                                "donot"→"do not", "cannot"→"can not"
                                O(N) scan, zero risk

6.  LD frequency tie-breaker    ✅ done — SpellChecker.cpp
                                LD - (freq/255) replaces tier bonus
                                "stdy"→"study" beats "stay" cleanly

7.  Apostrophe protection       ✅ done — Tokenizer.cpp + Reconstructor.cpp
                                split "let's" → root "let" + suffix "'s"
                                spell check root only, reattach on output

8.  Their/There/They're rule    ✅ done — GrammarEngine.cpp
                                real-word error — Levenshtein blind to it
                                context rule: next word POS decides

9.  Plural subject-verb         ✅ done — GrammarEngine.cpp
                                "the dogs helps" → "the dogs help"
                                extension of existing noun rule

10. Bigram POS transitions      ✅ done — pos_bigram.h + POSTagger.cpp
                                improves POS accuracy for everything downstream

11. Context-aware spell         ✅ done — SpellChecker.cpp
                                neighbor POS reranks candidates
                                ARTICLE left → prefer NOUN correction

12. Memory limits               ✅ done — Tokenizer.cpp + main.cpp
                                max token count, fixed buffers

13. Correction logs             ✅ done — main.cpp
                                debug mode — which rule fired and why

14. Benchmark timing            ✅ done — main.cpp
                                ms per stage printed after each correction

15. Correction statistics       ✅ done — main.cpp
                                pass/fail counts tracked over session

16. Frequency tiers (real)      ✅ done — dictionary.h
                                fill actual 1/2/3 values properly

17. LittleFS dictionary         reserve — replace PROGMEM
                                larger word list becomes possible

18. Plurality detection         ✅ done — GrammarEngine.cpp
                                "the dogs goes" → "the dogs go"
                                separate from item 9

19. Question tag sentences      reserve — QuestionDetector.cpp
                                "she is nice, isn't she?"

20. Adverb placement            ✅ done — GrammarEngine.cpp
                                "he quickly run" → "he runs quickly"
