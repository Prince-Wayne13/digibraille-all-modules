#pragma once
#include <Arduino.h>

struct TestEntry {
    const char* input;
    const char* expected;
    const char* category;
};

// SPELL tests — full sentence input so pipeline output matches
const char t_in000[] PROGMEM = "the flowrs are nice";        const char t_ex000[] PROGMEM = "The flowers are nice.";       const char t_cat000[] PROGMEM = "SPELL";
const char t_in001[] PROGMEM = "i recieve a letter";         const char t_ex001[] PROGMEM = "I receive a letter.";          const char t_cat001[] PROGMEM = "SPELL";
const char t_in002[] PROGMEM = "tomorow is a good day";      const char t_ex002[] PROGMEM = "Tomorrow is a good day.";      const char t_cat002[] PROGMEM = "SPELL";
const char t_in003[] PROGMEM = "she is my freind";           const char t_ex003[] PROGMEM = "She is my friend.";            const char t_cat003[] PROGMEM = "SPELL";
const char t_in004[] PROGMEM = "i beleive you";              const char t_ex004[] PROGMEM = "I believe you.";               const char t_cat004[] PROGMEM = "SPELL";
const char t_in005[] PROGMEM = "she is definately right";    const char t_ex005[] PROGMEM = "She is definitely right.";     const char t_cat005[] PROGMEM = "SPELL";
const char t_in006[] PROGMEM = "keep them seperate";         const char t_ex006[] PROGMEM = "Keep them separate.";          const char t_cat006[] PROGMEM = "SPELL";
const char t_in007[] PROGMEM = "it occured yesterday";       const char t_ex007[] PROGMEM = "It occurred yesterday.";       const char t_cat007[] PROGMEM = "SPELL";
const char t_in008[] PROGMEM = "that is wierd";              const char t_ex008[] PROGMEM = "That is weird.";               const char t_cat008[] PROGMEM = "SPELL";
const char t_in009[] PROGMEM = "fix your grammer";           const char t_ex009[] PROGMEM = "Fix your grammar.";            const char t_cat009[] PROGMEM = "SPELL";

// SUBJ_VERB tests
const char t_in010[] PROGMEM = "he go to school";            const char t_ex010[] PROGMEM = "He goes to school.";           const char t_cat010[] PROGMEM = "SUBJ_VERB";
const char t_in011[] PROGMEM = "she run fast";               const char t_ex011[] PROGMEM = "She runs fast.";               const char t_cat011[] PROGMEM = "SUBJ_VERB";
const char t_in012[] PROGMEM = "it work well";               const char t_ex012[] PROGMEM = "It works well.";               const char t_cat012[] PROGMEM = "SUBJ_VERB";
const char t_in013[] PROGMEM = "they was happy";             const char t_ex013[] PROGMEM = "They were happy.";             const char t_cat013[] PROGMEM = "SUBJ_VERB";
const char t_in014[] PROGMEM = "he were late";               const char t_ex014[] PROGMEM = "He was late.";                 const char t_cat014[] PROGMEM = "SUBJ_VERB";
const char t_in015[] PROGMEM = "she eat lunch";              const char t_ex015[] PROGMEM = "She eats lunch.";              const char t_cat015[] PROGMEM = "SUBJ_VERB";
const char t_in016[] PROGMEM = "the dog go outside";         const char t_ex016[] PROGMEM = "The dog goes outside.";        const char t_cat016[] PROGMEM = "SUBJ_VERB";
const char t_in017[] PROGMEM = "a cat run away";             const char t_ex017[] PROGMEM = "A cat runs away.";             const char t_cat017[] PROGMEM = "SUBJ_VERB";

// CAPITAL_I tests
const char t_in018[] PROGMEM = "i went to school";           const char t_ex018[] PROGMEM = "I went to school.";            const char t_cat018[] PROGMEM = "CAPITAL_I";
const char t_in019[] PROGMEM = "i like food";                const char t_ex019[] PROGMEM = "I like food.";                 const char t_cat019[] PROGMEM = "CAPITAL_I";
const char t_in020[] PROGMEM = "yesterday i went";           const char t_ex020[] PROGMEM = "Yesterday, I went.";           const char t_cat020[] PROGMEM = "CAPITAL_I";
const char t_in021[] PROGMEM = "i am happy";                 const char t_ex021[] PROGMEM = "I am happy.";                  const char t_cat021[] PROGMEM = "CAPITAL_I";
const char t_in022[] PROGMEM = "i know the answer";          const char t_ex022[] PROGMEM = "I know the answer.";           const char t_cat022[] PROGMEM = "CAPITAL_I";

// QUESTION tests
const char t_in023[] PROGMEM = "are you coming";             const char t_ex023[] PROGMEM = "Are you coming?";              const char t_cat023[] PROGMEM = "QUESTION";
const char t_in024[] PROGMEM = "where did she go";           const char t_ex024[] PROGMEM = "Where did she go?";            const char t_cat024[] PROGMEM = "QUESTION";
const char t_in025[] PROGMEM = "what is your name";          const char t_ex025[] PROGMEM = "What is your name?";           const char t_cat025[] PROGMEM = "QUESTION";
const char t_in026[] PROGMEM = "is she happy";               const char t_ex026[] PROGMEM = "Is she happy?";                const char t_cat026[] PROGMEM = "QUESTION";
const char t_in027[] PROGMEM = "can he run";                 const char t_ex027[] PROGMEM = "Can he run?";                  const char t_cat027[] PROGMEM = "QUESTION";
const char t_in028[] PROGMEM = "did you eat";                const char t_ex028[] PROGMEM = "Did you eat?";                 const char t_cat028[] PROGMEM = "QUESTION";
const char t_in029[] PROGMEM = "does the dog eat";           const char t_ex029[] PROGMEM = "Does the dog eat?";            const char t_cat029[] PROGMEM = "QUESTION";
const char t_in030[] PROGMEM = "how are you";                const char t_ex030[] PROGMEM = "How are you?";                 const char t_cat030[] PROGMEM = "QUESTION";
const char t_in031[] PROGMEM = "he go home";                 const char t_ex031[] PROGMEM = "He goes home.";                const char t_cat031[] PROGMEM = "SUBJ_VERB";
const char t_in032[] PROGMEM = "the dog go home";            const char t_ex032[] PROGMEM = "The dog goes home.";           const char t_cat032[] PROGMEM = "SUBJ_VERB";
const char t_in033[] PROGMEM = "he quickly ru home";         const char t_ex033[] PROGMEM = "He runs home quickly.";        const char t_cat033[] PROGMEM = "SUBJ_VERB";
const char t_in034[] PROGMEM = "he quickly run";             const char t_ex034[] PROGMEM = "He runs quickly.";             const char t_cat034[] PROGMEM = "SUBJ_VERB";
const char t_in035[] PROGMEM = "every student have a book";  const char t_ex035[] PROGMEM = "Every student has a book.";    const char t_cat035[] PROGMEM = "SUBJ_VERB";

const TestEntry TEST_BANK[] PROGMEM = {
    {t_in000,t_ex000,t_cat000},{t_in001,t_ex001,t_cat001},
    {t_in002,t_ex002,t_cat002},{t_in003,t_ex003,t_cat003},
    {t_in004,t_ex004,t_cat004},{t_in005,t_ex005,t_cat005},
    {t_in006,t_ex006,t_cat006},{t_in007,t_ex007,t_cat007},
    {t_in008,t_ex008,t_cat008},{t_in009,t_ex009,t_cat009},
    {t_in010,t_ex010,t_cat010},{t_in011,t_ex011,t_cat011},
    {t_in012,t_ex012,t_cat012},{t_in013,t_ex013,t_cat013},
    {t_in014,t_ex014,t_cat014},{t_in015,t_ex015,t_cat015},
    {t_in016,t_ex016,t_cat016},{t_in017,t_ex017,t_cat017},
    {t_in018,t_ex018,t_cat018},{t_in019,t_ex019,t_cat019},
    {t_in020,t_ex020,t_cat020},{t_in021,t_ex021,t_cat021},
    {t_in022,t_ex022,t_cat022},{t_in023,t_ex023,t_cat023},
    {t_in024,t_ex024,t_cat024},{t_in025,t_ex025,t_cat025},
    {t_in026,t_ex026,t_cat026},{t_in027,t_ex027,t_cat027},
    {t_in028,t_ex028,t_cat028},{t_in029,t_ex029,t_cat029},
    {t_in030,t_ex030,t_cat030},{t_in031,t_ex031,t_cat031},
    {t_in032,t_ex032,t_cat032},{t_in033,t_ex033,t_cat033},
    {t_in034,t_ex034,t_cat034},{t_in035,t_ex035,t_cat035},
};

const int TEST_BANK_SIZE = 36;
