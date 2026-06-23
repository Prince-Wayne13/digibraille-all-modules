#include "offline_grammar.h"
#include "Normalizer.h"
#include "Tokenizer.h"
#include "Reconstructor.h"
#include "SpellChecker.h"
#include "POSTagger.h"
#include "GrammarEngine.h"
#include "QuestionDetector.h"
#include "PunctuationEngine.h"

static Normalizer        _normalizer;
static Tokenizer         _tokenizer;
static Reconstructor     _reconstructor;
static SpellChecker      _spellChecker;
static POSTagger         _posTagger;
static GrammarEngine     _grammarEngine;
static QuestionDetector  _questionDetector;
static PunctuationEngine _punctuationEngine;
static bool              _grammarReady = false;

void offlineGrammarSetup() {
  _grammarReady = true;
}

static String correctSentence(const String& input) {
  String trimmed = input;
  trimmed.trim();
  if (trimmed.length() == 0) return "";

  String norm = _normalizer.normalize(trimmed);
  std::vector<Token> tokens = _tokenizer.tokenize(norm);
  _spellChecker.checkAndCorrect(tokens);
  std::vector<TaggedToken> tagged = _posTagger.tag(tokens);
  _grammarEngine.correct(tagged);
  bool isQuestion = _questionDetector.isQuestion(tagged);
  _punctuationEngine.correct(tagged, isQuestion);

  std::vector<Token> corrected;
  corrected.reserve(tagged.size());
  for (auto& t : tagged) corrected.push_back(t.token);
  return _reconstructor.reconstruct(corrected);
}

String improveNoteOffline(const String& noteText) {
  if (!_grammarReady) offlineGrammarSetup();

  String output = "";
  String sentence = "";
  int sentenceCount = 0;

  for (int i = 0; i < (int)noteText.length(); i++) {
    char c = noteText[i];
    sentence += c;
    if (c == '.' || c == '!' || c == '?' || c == '\n') {
      String fixed = correctSentence(sentence);
      if (fixed.length() > 0) {
        if (output.length() > 0) output += " ";
        output += fixed;
        sentenceCount++;
      }
      sentence = "";
    }
  }

  sentence.trim();
  if (sentence.length() > 0) {
    String fixed = correctSentence(sentence);
    if (fixed.length() > 0) {
      if (output.length() > 0) output += " ";
      output += fixed;
      sentenceCount++;
    }
  }

  output.trim();
  return output.length() > 0 ? output : noteText;
}
