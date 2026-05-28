#pragma once
#include <Arduino.h>
#include <pgmspace.h>
#include "POSTagger.h"

struct POSEntry {
    const char* word;
    POS pos;
};

// Stored in Flash
const char pos_w000[] PROGMEM = "i";           const char pos_w001[] PROGMEM = "he";
const char pos_w002[] PROGMEM = "she";         const char pos_w003[] PROGMEM = "they";
const char pos_w004[] PROGMEM = "we";          const char pos_w005[] PROGMEM = "you";
const char pos_w006[] PROGMEM = "it";          const char pos_w007[] PROGMEM = "me";
const char pos_w008[] PROGMEM = "him";         const char pos_w009[] PROGMEM = "her";
const char pos_w010[] PROGMEM = "us";          const char pos_w011[] PROGMEM = "them";
const char pos_w012[] PROGMEM = "who";

const char pos_w013[] PROGMEM = "is";          const char pos_w014[] PROGMEM = "are";
const char pos_w015[] PROGMEM = "was";         const char pos_w016[] PROGMEM = "were";
const char pos_w017[] PROGMEM = "be";          const char pos_w018[] PROGMEM = "been";
const char pos_w019[] PROGMEM = "go";          const char pos_w020[] PROGMEM = "goes";
const char pos_w021[] PROGMEM = "went";        const char pos_w022[] PROGMEM = "run";
const char pos_w023[] PROGMEM = "runs";        const char pos_w024[] PROGMEM = "ran";
const char pos_w025[] PROGMEM = "eat";         const char pos_w026[] PROGMEM = "eats";
const char pos_w027[] PROGMEM = "ate";         const char pos_w028[] PROGMEM = "come";
const char pos_w029[] PROGMEM = "comes";       const char pos_w030[] PROGMEM = "came";
const char pos_w031[] PROGMEM = "take";        const char pos_w032[] PROGMEM = "takes";
const char pos_w033[] PROGMEM = "took";        const char pos_w034[] PROGMEM = "give";
const char pos_w035[] PROGMEM = "gives";       const char pos_w036[] PROGMEM = "gave";
const char pos_w037[] PROGMEM = "get";         const char pos_w038[] PROGMEM = "gets";
const char pos_w039[] PROGMEM = "got";         const char pos_w040[] PROGMEM = "make";
const char pos_w041[] PROGMEM = "makes";       const char pos_w042[] PROGMEM = "made";
const char pos_w043[] PROGMEM = "know";        const char pos_w044[] PROGMEM = "knows";
const char pos_w045[] PROGMEM = "knew";        const char pos_w046[] PROGMEM = "see";
const char pos_w047[] PROGMEM = "sees";        const char pos_w048[] PROGMEM = "saw";
const char pos_w049[] PROGMEM = "say";         const char pos_w050[] PROGMEM = "says";
const char pos_w051[] PROGMEM = "said";        const char pos_w052[] PROGMEM = "think";
const char pos_w053[] PROGMEM = "thinks";      const char pos_w054[] PROGMEM = "thought";
const char pos_w055[] PROGMEM = "want";        const char pos_w056[] PROGMEM = "wants";
const char pos_w057[] PROGMEM = "wanted";      const char pos_w058[] PROGMEM = "need";
const char pos_w059[] PROGMEM = "needs";       const char pos_w060[] PROGMEM = "needed";
const char pos_w061[] PROGMEM = "feel";        const char pos_w062[] PROGMEM = "feels";
const char pos_w063[] PROGMEM = "felt";        const char pos_w064[] PROGMEM = "live";
const char pos_w065[] PROGMEM = "lives";       const char pos_w066[] PROGMEM = "lived";
const char pos_w067[] PROGMEM = "play";        const char pos_w068[] PROGMEM = "plays";
const char pos_w069[] PROGMEM = "played";      const char pos_w070[] PROGMEM = "work";
const char pos_w071[] PROGMEM = "works";       const char pos_w072[] PROGMEM = "worked";
const char pos_w073[] PROGMEM = "love";        const char pos_w074[] PROGMEM = "loves";
const char pos_w075[] PROGMEM = "loved";       const char pos_w076[] PROGMEM = "like";
const char pos_w077[] PROGMEM = "likes";       const char pos_w078[] PROGMEM = "liked";
const char pos_w079[] PROGMEM = "help";        const char pos_w080[] PROGMEM = "helps";
const char pos_w081[] PROGMEM = "helped";      const char pos_w082[] PROGMEM = "write";
const char pos_w083[] PROGMEM = "writes";      const char pos_w084[] PROGMEM = "wrote";
const char pos_w085[] PROGMEM = "read";        const char pos_w086[] PROGMEM = "reads";
const char pos_w087[] PROGMEM = "bring";       const char pos_w088[] PROGMEM = "brings";
const char pos_w089[] PROGMEM = "brought";     const char pos_w090[] PROGMEM = "keep";
const char pos_w091[] PROGMEM = "keeps";       const char pos_w092[] PROGMEM = "kept";
const char pos_w093[] PROGMEM = "leave";       const char pos_w094[] PROGMEM = "leaves";
const char pos_w095[] PROGMEM = "left";        const char pos_w096[] PROGMEM = "put";
const char pos_w097[] PROGMEM = "puts";        const char pos_w098[] PROGMEM = "sit";
const char pos_w099[] PROGMEM = "sits";        const char pos_w100[] PROGMEM = "sat";
const char pos_w101[] PROGMEM = "stand";       const char pos_w102[] PROGMEM = "stands";
const char pos_w103[] PROGMEM = "stood";       const char pos_w104[] PROGMEM = "buy";
const char pos_w105[] PROGMEM = "buys";        const char pos_w106[] PROGMEM = "bought";
const char pos_w107[] PROGMEM = "sell";        const char pos_w108[] PROGMEM = "sells";
const char pos_w109[] PROGMEM = "sold";        const char pos_w110[] PROGMEM = "find";
const char pos_w111[] PROGMEM = "finds";       const char pos_w112[] PROGMEM = "found";
const char pos_w113[] PROGMEM = "lose";        const char pos_w114[] PROGMEM = "loses";
const char pos_w115[] PROGMEM = "lost";        const char pos_w116[] PROGMEM = "tell";
const char pos_w117[] PROGMEM = "tells";       const char pos_w118[] PROGMEM = "told";
const char pos_w119[] PROGMEM = "ask";         const char pos_w120[] PROGMEM = "asks";
const char pos_w121[] PROGMEM = "asked";       const char pos_w122[] PROGMEM = "show";
const char pos_w123[] PROGMEM = "shows";       const char pos_w124[] PROGMEM = "showed";
const char pos_w125[] PROGMEM = "try";         const char pos_w126[] PROGMEM = "tries";
const char pos_w127[] PROGMEM = "tried";       const char pos_w128[] PROGMEM = "call";
const char pos_w129[] PROGMEM = "calls";       const char pos_w130[] PROGMEM = "called";
const char pos_w131[] PROGMEM = "move";        const char pos_w132[] PROGMEM = "moves";
const char pos_w133[] PROGMEM = "moved";       const char pos_w134[] PROGMEM = "open";
const char pos_w135[] PROGMEM = "opens";       const char pos_w136[] PROGMEM = "opened";
const char pos_w137[] PROGMEM = "start";       const char pos_w138[] PROGMEM = "starts";
const char pos_w139[] PROGMEM = "started";     const char pos_w140[] PROGMEM = "stop";
const char pos_w141[] PROGMEM = "stops";       const char pos_w142[] PROGMEM = "stopped";
const char pos_w143[] PROGMEM = "turn";        const char pos_w144[] PROGMEM = "turns";
const char pos_w145[] PROGMEM = "turned";      const char pos_w146[] PROGMEM = "walk";
const char pos_w147[] PROGMEM = "walks";       const char pos_w148[] PROGMEM = "walked";
const char pos_w149[] PROGMEM = "talk";        const char pos_w150[] PROGMEM = "talks";
const char pos_w151[] PROGMEM = "talked";      const char pos_w152[] PROGMEM = "listen";
const char pos_w153[] PROGMEM = "listens";     const char pos_w154[] PROGMEM = "listened";
const char pos_w155[] PROGMEM = "learn";       const char pos_w156[] PROGMEM = "learns";
const char pos_w157[] PROGMEM = "learned";     const char pos_w158[] PROGMEM = "teach";
const char pos_w159[] PROGMEM = "teaches";     const char pos_w160[] PROGMEM = "taught";
const char pos_w161[] PROGMEM = "send";        const char pos_w162[] PROGMEM = "sends";
const char pos_w163[] PROGMEM = "sent";        const char pos_w164[] PROGMEM = "receive";
const char pos_w165[] PROGMEM = "receives";    const char pos_w166[] PROGMEM = "received";
const char pos_w167[] PROGMEM = "build";       const char pos_w168[] PROGMEM = "builds";
const char pos_w169[] PROGMEM = "built";       const char pos_w170[] PROGMEM = "grow";
const char pos_w171[] PROGMEM = "grows";       const char pos_w172[] PROGMEM = "grew";
const char pos_w173[] PROGMEM = "meet";        const char pos_w174[] PROGMEM = "meets";
const char pos_w175[] PROGMEM = "met";         const char pos_w176[] PROGMEM = "pay";
const char pos_w177[] PROGMEM = "pays";        const char pos_w178[] PROGMEM = "paid";
const char pos_w179[] PROGMEM = "win";         const char pos_w180[] PROGMEM = "wins";
const char pos_w181[] PROGMEM = "won";         const char pos_w182[] PROGMEM = "hold";
const char pos_w183[] PROGMEM = "holds";       const char pos_w184[] PROGMEM = "held";
const char pos_w185[] PROGMEM = "drive";       const char pos_w186[] PROGMEM = "drives";
const char pos_w187[] PROGMEM = "drove";       const char pos_w188[] PROGMEM = "sleep";
const char pos_w189[] PROGMEM = "sleeps";      const char pos_w190[] PROGMEM = "slept";
const char pos_w191[] PROGMEM = "drink";       const char pos_w192[] PROGMEM = "drinks";
const char pos_w193[] PROGMEM = "drank";       const char pos_w194[] PROGMEM = "swim";
const char pos_w195[] PROGMEM = "swims";       const char pos_w196[] PROGMEM = "swam";
const char pos_w197[] PROGMEM = "fly";         const char pos_w198[] PROGMEM = "flies";
const char pos_w199[] PROGMEM = "flew";

// Articles
const char pos_w200[] PROGMEM = "a";
const char pos_w201[] PROGMEM = "an";
const char pos_w202[] PROGMEM = "the";

// Auxiliaries
const char pos_w203[] PROGMEM = "do";          const char pos_w204[] PROGMEM = "does";
const char pos_w205[] PROGMEM = "did";         const char pos_w206[] PROGMEM = "have";
const char pos_w207[] PROGMEM = "has";         const char pos_w208[] PROGMEM = "had";
const char pos_w209[] PROGMEM = "will";        const char pos_w210[] PROGMEM = "would";
const char pos_w211[] PROGMEM = "can";         const char pos_w212[] PROGMEM = "could";
const char pos_w213[] PROGMEM = "shall";       const char pos_w214[] PROGMEM = "should";
const char pos_w215[] PROGMEM = "may";         const char pos_w216[] PROGMEM = "might";
const char pos_w217[] PROGMEM = "must";

// Negatives
const char pos_w218[] PROGMEM = "not";         const char pos_w219[] PROGMEM = "never";
const char pos_w220[] PROGMEM = "nothing";     const char pos_w221[] PROGMEM = "nobody";
const char pos_w222[] PROGMEM = "nowhere";     const char pos_w223[] PROGMEM = "neither";
const char pos_w224[] PROGMEM = "no";

// Prepositions
const char pos_w225[] PROGMEM = "in";          const char pos_w226[] PROGMEM = "on";
const char pos_w227[] PROGMEM = "at";          const char pos_w228[] PROGMEM = "to";
const char pos_w229[] PROGMEM = "from";        const char pos_w230[] PROGMEM = "with";
const char pos_w231[] PROGMEM = "by";          const char pos_w232[] PROGMEM = "for";
const char pos_w233[] PROGMEM = "of";          const char pos_w234[] PROGMEM = "about";
const char pos_w235[] PROGMEM = "above";       const char pos_w236[] PROGMEM = "below";
const char pos_w237[] PROGMEM = "between";     const char pos_w238[] PROGMEM = "into";
const char pos_w239[] PROGMEM = "through";     const char pos_w240[] PROGMEM = "during";
const char pos_w241[] PROGMEM = "without";     const char pos_w242[] PROGMEM = "under";
const char pos_w243[] PROGMEM = "over";        const char pos_w244[] PROGMEM = "after";
const char pos_w245[] PROGMEM = "before";      const char pos_w246[] PROGMEM = "since";
const char pos_w247[] PROGMEM = "until";       const char pos_w248[] PROGMEM = "toward";
const char pos_w249[] PROGMEM = "against";     const char pos_w250[] PROGMEM = "across";

// Conjunctions
const char pos_w251[] PROGMEM = "and";         const char pos_w252[] PROGMEM = "but";
const char pos_w253[] PROGMEM = "or";          const char pos_w254[] PROGMEM = "because";
const char pos_w255[] PROGMEM = "although";    const char pos_w256[] PROGMEM = "though";
const char pos_w257[] PROGMEM = "while";       const char pos_w258[] PROGMEM = "if";
const char pos_w259[] PROGMEM = "so";          const char pos_w260[] PROGMEM = "yet";
const char pos_w261[] PROGMEM = "nor";         const char pos_w262[] PROGMEM = "when";
const char pos_w263[] PROGMEM = "that";        const char pos_w264[] PROGMEM = "than";

// Past time adverbs
const char pos_w265[] PROGMEM = "yesterday";   const char pos_w266[] PROGMEM = "ago";
const char pos_w267[] PROGMEM = "last";        const char pos_w268[] PROGMEM = "previously";
const char pos_w269[] PROGMEM = "formerly";    const char pos_w270[] PROGMEM = "once";
const char pos_w271[] PROGMEM = "then";

// General adverbs
const char pos_w272[] PROGMEM = "very";        const char pos_w273[] PROGMEM = "really";
const char pos_w274[] PROGMEM = "quickly";     const char pos_w275[] PROGMEM = "slowly";
const char pos_w276[] PROGMEM = "always";      const char pos_w277[] PROGMEM = "often";
const char pos_w278[] PROGMEM = "sometimes";   const char pos_w279[] PROGMEM = "never";
const char pos_w280[] PROGMEM = "already";     const char pos_w281[] PROGMEM = "still";
const char pos_w282[] PROGMEM = "just";        const char pos_w283[] PROGMEM = "also";
const char pos_w284[] PROGMEM = "too";         const char pos_w285[] PROGMEM = "again";
const char pos_w286[] PROGMEM = "here";        const char pos_w287[] PROGMEM = "there";
const char pos_w288[] PROGMEM = "now";         const char pos_w289[] PROGMEM = "today";
const char pos_w290[] PROGMEM = "tomorrow";    const char pos_w291[] PROGMEM = "soon";
const char pos_w292[] PROGMEM = "almost";      const char pos_w293[] PROGMEM = "enough";
const char pos_w300[] PROGMEM = "ability";    const char pos_w301[] PROGMEM = "absence";
const char pos_w302[] PROGMEM = "academic";    const char pos_w303[] PROGMEM = "access";
const char pos_w304[] PROGMEM = "accident";    const char pos_w305[] PROGMEM = "account";
const char pos_w306[] PROGMEM = "action";      const char pos_w307[] PROGMEM = "activity";
const char pos_w308[] PROGMEM = "actor";       const char pos_w309[] PROGMEM = "actress";
const char pos_w310[] PROGMEM = "address";     const char pos_w311[] PROGMEM = "adjust";
const char pos_w312[] PROGMEM = "administration"; const char pos_w313[] PROGMEM = "adult";
const char pos_w314[] PROGMEM = "advance";     const char pos_w315[] PROGMEM = "advantage";
const char pos_w316[] PROGMEM = "adventure";   const char pos_w317[] PROGMEM = "advice";
const char pos_w318[] PROGMEM = "affair";      const char pos_w319[] PROGMEM = "affect";
const char pos_w320[] PROGMEM = "afraid";      const char pos_w321[] PROGMEM = "afternoon";
const char pos_w322[] PROGMEM = "agency";      const char pos_w323[] PROGMEM = "agent";
const char pos_w324[] PROGMEM = "aggressive";  const char pos_w325[] PROGMEM = "agreement";
const char pos_w326[] PROGMEM = "agriculture"; const char pos_w327[] PROGMEM = "ahead";
const char pos_w328[] PROGMEM = "aircraft";    const char pos_w329[] PROGMEM = "airport";
const char pos_w330[] PROGMEM = "alarm";       const char pos_w331[] PROGMEM = "album";
const char pos_w332[] PROGMEM = "alcohol";     const char pos_w333[] PROGMEM = "alert";
const char pos_w334[] PROGMEM = "alive";       const char pos_w335[] PROGMEM = "alleged";
const char pos_w336[] PROGMEM = "allow";       const char pos_w337[] PROGMEM = "almost";
const char pos_w338[] PROGMEM = "alone";       const char pos_w339[] PROGMEM = "alongside";
const char pos_w340[] PROGMEM = "amazed";      const char pos_w341[] PROGMEM = "amazing";
const char pos_w342[] PROGMEM = "ambition";    const char pos_w343[] PROGMEM = "amount";
const char pos_w344[] PROGMEM = "analyze";     const char pos_w345[] PROGMEM = "ancient";
const char pos_w346[] PROGMEM = "anger";       const char pos_w347[] PROGMEM = "angle";
const char pos_w348[] PROGMEM = "angry";       const char pos_w349[] PROGMEM = "animal";
const char pos_w350[] PROGMEM = "anniversary"; const char pos_w351[] PROGMEM = "announce";
const char pos_w352[] PROGMEM = "annual";      const char pos_w353[] PROGMEM = "another";
const char pos_w354[] PROGMEM = "answer";      const char pos_w355[] PROGMEM = "anticipate";
const char pos_w356[] PROGMEM = "anxiety";     const char pos_w357[] PROGMEM = "anxious";
const char pos_w358[] PROGMEM = "anybody";     const char pos_w359[] PROGMEM = "anymore";
const char pos_w360[] PROGMEM = "apart";       const char pos_w361[] PROGMEM = "apartment";
const char pos_w362[] PROGMEM = "apparent";    const char pos_w363[] PROGMEM = "appeal";
const char pos_w364[] PROGMEM = "appear";      const char pos_w365[] PROGMEM = "although";
const char pos_w366[] PROGMEM = "altogether";  const char pos_w367[] PROGMEM = "always";
const char pos_w368[] PROGMEM = "amazingly";   const char pos_w369[] PROGMEM = "anywhere";
const char pos_w370[] PROGMEM = "apparently";  const char pos_w371[] PROGMEM = "approximately";
const char pos_w372[] PROGMEM = "arguably";    const char pos_w373[] PROGMEM = "awfully";
const char pos_w374[] PROGMEM = "badly";       const char pos_w375[] PROGMEM = "barely";
const char pos_w376[] PROGMEM = "basically";   const char pos_w377[] PROGMEM = "briefly";
const char pos_w378[] PROGMEM = "certainly";   const char pos_w379[] PROGMEM = "clearly";
const char pos_w380[] PROGMEM = "myself";      const char pos_w381[] PROGMEM = "yourself";
const char pos_w382[] PROGMEM = "this";        const char pos_w383[] PROGMEM = "that";
const char pos_w384[] PROGMEM = "these";       const char pos_w385[] PROGMEM = "those";
const char pos_w386[] PROGMEM = "some";        const char pos_w387[] PROGMEM = "any";
const char pos_w388[] PROGMEM = "each";        const char pos_w389[] PROGMEM = "every";
const char pos_w390[] PROGMEM = "wow";         const char pos_w391[] PROGMEM = "ouch";
const char pos_w392[] PROGMEM = "apple";       const char pos_w393[] PROGMEM = "banana";
const char pos_w394[] PROGMEM = "car";         const char pos_w395[] PROGMEM = "computer";
const char pos_w396[] PROGMEM = "door";        const char pos_w397[] PROGMEM = "engine";
const char pos_w398[] PROGMEM = "flower";      const char pos_w399[] PROGMEM = "garden";


struct POSTableEntry {
    const char* word;
    POS         pos;
};

const POSTableEntry POS_TABLE[] PROGMEM = {
    // Pronouns (0-12)
    {pos_w000,POS::PRONOUN},{pos_w001,POS::PRONOUN},{pos_w002,POS::PRONOUN},
    {pos_w003,POS::PRONOUN},{pos_w004,POS::PRONOUN},{pos_w005,POS::PRONOUN},
    {pos_w006,POS::PRONOUN},{pos_w007,POS::PRONOUN},{pos_w008,POS::PRONOUN},
    {pos_w009,POS::PRONOUN},{pos_w010,POS::PRONOUN},{pos_w011,POS::PRONOUN},
    {pos_w012,POS::PRONOUN},
    // Verbs (13-199)
    {pos_w013,POS::VERB},{pos_w014,POS::VERB},{pos_w015,POS::VERB},
    {pos_w016,POS::VERB},{pos_w017,POS::VERB},{pos_w018,POS::VERB},
    {pos_w019,POS::VERB},{pos_w020,POS::VERB},{pos_w021,POS::VERB},
    {pos_w022,POS::VERB},{pos_w023,POS::VERB},{pos_w024,POS::VERB},
    {pos_w025,POS::VERB},{pos_w026,POS::VERB},{pos_w027,POS::VERB},
    {pos_w028,POS::VERB},{pos_w029,POS::VERB},{pos_w030,POS::VERB},
    {pos_w031,POS::VERB},{pos_w032,POS::VERB},{pos_w033,POS::VERB},
    {pos_w034,POS::VERB},{pos_w035,POS::VERB},{pos_w036,POS::VERB},
    {pos_w037,POS::VERB},{pos_w038,POS::VERB},{pos_w039,POS::VERB},
    {pos_w040,POS::VERB},{pos_w041,POS::VERB},{pos_w042,POS::VERB},
    {pos_w043,POS::VERB},{pos_w044,POS::VERB},{pos_w045,POS::VERB},
    {pos_w046,POS::VERB},{pos_w047,POS::VERB},{pos_w048,POS::VERB},
    {pos_w049,POS::VERB},{pos_w050,POS::VERB},{pos_w051,POS::VERB},
    {pos_w052,POS::VERB},{pos_w053,POS::VERB},{pos_w054,POS::VERB},
    {pos_w055,POS::VERB},{pos_w056,POS::VERB},{pos_w057,POS::VERB},
    {pos_w058,POS::VERB},{pos_w059,POS::VERB},{pos_w060,POS::VERB},
    {pos_w061,POS::VERB},{pos_w062,POS::VERB},{pos_w063,POS::VERB},
    {pos_w064,POS::VERB},{pos_w065,POS::VERB},{pos_w066,POS::VERB},
    {pos_w067,POS::VERB},{pos_w068,POS::VERB},{pos_w069,POS::VERB},
    {pos_w070,POS::VERB},{pos_w071,POS::VERB},{pos_w072,POS::VERB},
    {pos_w073,POS::VERB},{pos_w074,POS::VERB},{pos_w075,POS::VERB},
    {pos_w076,POS::VERB},{pos_w077,POS::VERB},{pos_w078,POS::VERB},
    {pos_w079,POS::VERB},{pos_w080,POS::VERB},{pos_w081,POS::VERB},
    {pos_w082,POS::VERB},{pos_w083,POS::VERB},{pos_w084,POS::VERB},
    {pos_w085,POS::VERB},{pos_w086,POS::VERB},{pos_w087,POS::VERB},
    {pos_w088,POS::VERB},{pos_w089,POS::VERB},{pos_w090,POS::VERB},
    {pos_w091,POS::VERB},{pos_w092,POS::VERB},{pos_w093,POS::VERB},
    {pos_w094,POS::VERB},{pos_w095,POS::VERB},{pos_w096,POS::VERB},
    {pos_w097,POS::VERB},{pos_w098,POS::VERB},{pos_w099,POS::VERB},
    {pos_w100,POS::VERB},{pos_w101,POS::VERB},{pos_w102,POS::VERB},
    {pos_w103,POS::VERB},{pos_w104,POS::VERB},{pos_w105,POS::VERB},
    {pos_w106,POS::VERB},{pos_w107,POS::VERB},{pos_w108,POS::VERB},
    {pos_w109,POS::VERB},{pos_w110,POS::VERB},{pos_w111,POS::VERB},
    {pos_w112,POS::VERB},{pos_w113,POS::VERB},{pos_w114,POS::VERB},
    {pos_w115,POS::VERB},{pos_w116,POS::VERB},{pos_w117,POS::VERB},
    {pos_w118,POS::VERB},{pos_w119,POS::VERB},{pos_w120,POS::VERB},
    {pos_w121,POS::VERB},{pos_w122,POS::VERB},{pos_w123,POS::VERB},
    {pos_w124,POS::VERB},{pos_w125,POS::VERB},{pos_w126,POS::VERB},
    {pos_w127,POS::VERB},{pos_w128,POS::VERB},{pos_w129,POS::VERB},
    {pos_w130,POS::VERB},{pos_w131,POS::VERB},{pos_w132,POS::VERB},
    {pos_w133,POS::VERB},{pos_w134,POS::VERB},{pos_w135,POS::VERB},
    {pos_w136,POS::VERB},{pos_w137,POS::VERB},{pos_w138,POS::VERB},
    {pos_w139,POS::VERB},{pos_w140,POS::VERB},{pos_w141,POS::VERB},
    {pos_w142,POS::VERB},{pos_w143,POS::VERB},{pos_w144,POS::VERB},
    {pos_w145,POS::VERB},{pos_w146,POS::VERB},{pos_w147,POS::VERB},
    {pos_w148,POS::VERB},{pos_w149,POS::VERB},{pos_w150,POS::VERB},
    {pos_w151,POS::VERB},{pos_w152,POS::VERB},{pos_w153,POS::VERB},
    {pos_w154,POS::VERB},{pos_w155,POS::VERB},{pos_w156,POS::VERB},
    {pos_w157,POS::VERB},{pos_w158,POS::VERB},{pos_w159,POS::VERB},
    {pos_w160,POS::VERB},{pos_w161,POS::VERB},{pos_w162,POS::VERB},
    {pos_w163,POS::VERB},{pos_w164,POS::VERB},{pos_w165,POS::VERB},
    {pos_w166,POS::VERB},{pos_w167,POS::VERB},{pos_w168,POS::VERB},
    {pos_w169,POS::VERB},{pos_w170,POS::VERB},{pos_w171,POS::VERB},
    {pos_w172,POS::VERB},{pos_w173,POS::VERB},{pos_w174,POS::VERB},
    {pos_w175,POS::VERB},{pos_w176,POS::VERB},{pos_w177,POS::VERB},
    {pos_w178,POS::VERB},{pos_w179,POS::VERB},{pos_w180,POS::VERB},
    {pos_w181,POS::VERB},{pos_w182,POS::VERB},{pos_w183,POS::VERB},
    {pos_w184,POS::VERB},{pos_w185,POS::VERB},{pos_w186,POS::VERB},
    {pos_w187,POS::VERB},{pos_w188,POS::VERB},{pos_w189,POS::VERB},
    {pos_w190,POS::VERB},{pos_w191,POS::VERB},{pos_w192,POS::VERB},
    {pos_w193,POS::VERB},{pos_w194,POS::VERB},{pos_w195,POS::VERB},
    {pos_w196,POS::VERB},{pos_w197,POS::VERB},{pos_w198,POS::VERB},
    {pos_w199,POS::VERB},
    // Articles (200-202)
    {pos_w200,POS::ARTICLE},{pos_w201,POS::ARTICLE},{pos_w202,POS::ARTICLE},
    // Auxiliaries (203-217)
    {pos_w203,POS::AUXILIARY},{pos_w204,POS::AUXILIARY},{pos_w205,POS::AUXILIARY},
    {pos_w206,POS::AUXILIARY},{pos_w207,POS::AUXILIARY},{pos_w208,POS::AUXILIARY},
    {pos_w209,POS::AUXILIARY},{pos_w210,POS::AUXILIARY},{pos_w211,POS::AUXILIARY},
    {pos_w212,POS::AUXILIARY},{pos_w213,POS::AUXILIARY},{pos_w214,POS::AUXILIARY},
    {pos_w215,POS::AUXILIARY},{pos_w216,POS::AUXILIARY},{pos_w217,POS::AUXILIARY},
    // Negatives (218-224)
    {pos_w218,POS::NEGATIVE},{pos_w219,POS::NEGATIVE},{pos_w220,POS::NEGATIVE},
    {pos_w221,POS::NEGATIVE},{pos_w222,POS::NEGATIVE},{pos_w223,POS::NEGATIVE},
    {pos_w224,POS::NEGATIVE},
    // Prepositions (225-250)
    {pos_w225,POS::PREPOSITION},{pos_w226,POS::PREPOSITION},{pos_w227,POS::PREPOSITION},
    {pos_w228,POS::PREPOSITION},{pos_w229,POS::PREPOSITION},{pos_w230,POS::PREPOSITION},
    {pos_w231,POS::PREPOSITION},{pos_w232,POS::PREPOSITION},{pos_w233,POS::PREPOSITION},
    {pos_w234,POS::PREPOSITION},{pos_w235,POS::PREPOSITION},{pos_w236,POS::PREPOSITION},
    {pos_w237,POS::PREPOSITION},{pos_w238,POS::PREPOSITION},{pos_w239,POS::PREPOSITION},
    {pos_w240,POS::PREPOSITION},{pos_w241,POS::PREPOSITION},{pos_w242,POS::PREPOSITION},
    {pos_w243,POS::PREPOSITION},{pos_w244,POS::PREPOSITION},{pos_w245,POS::PREPOSITION},
    {pos_w246,POS::PREPOSITION},{pos_w247,POS::PREPOSITION},{pos_w248,POS::PREPOSITION},
    {pos_w249,POS::PREPOSITION},{pos_w250,POS::PREPOSITION},
    // Conjunctions (251-264)
    {pos_w251,POS::CONJUNCTION},{pos_w252,POS::CONJUNCTION},{pos_w253,POS::CONJUNCTION},
    {pos_w254,POS::CONJUNCTION},{pos_w255,POS::CONJUNCTION},{pos_w256,POS::CONJUNCTION},
    {pos_w257,POS::CONJUNCTION},{pos_w258,POS::CONJUNCTION},{pos_w259,POS::CONJUNCTION},
    {pos_w260,POS::CONJUNCTION},{pos_w261,POS::CONJUNCTION},{pos_w262,POS::CONJUNCTION},
    {pos_w263,POS::CONJUNCTION},{pos_w264,POS::CONJUNCTION},
    // Past time adverbs (265-271)
    {pos_w265,POS::TIME_PAST},{pos_w266,POS::TIME_PAST},{pos_w267,POS::TIME_PAST},
    {pos_w268,POS::TIME_PAST},{pos_w269,POS::TIME_PAST},{pos_w270,POS::TIME_PAST},
    {pos_w271,POS::TIME_PAST},
    // Adverbs (272-293)
    {pos_w272,POS::ADVERB},{pos_w273,POS::ADVERB},{pos_w274,POS::ADVERB},
    {pos_w275,POS::ADVERB},{pos_w276,POS::ADVERB},{pos_w277,POS::ADVERB},
    {pos_w278,POS::ADVERB},{pos_w279,POS::ADVERB},{pos_w280,POS::ADVERB},
    {pos_w281,POS::ADVERB},{pos_w282,POS::ADVERB},{pos_w283,POS::ADVERB},
    {pos_w284,POS::ADVERB},{pos_w285,POS::ADVERB},{pos_w286,POS::ADVERB},
    {pos_w287,POS::ADVERB},{pos_w288,POS::ADVERB},{pos_w289,POS::ADVERB},
    {pos_w290,POS::ADVERB},{pos_w291,POS::ADVERB},{pos_w292,POS::ADVERB},
    {pos_w293,POS::ADVERB},
    // Common nouns
{pos_w300,POS::NOUN},{pos_w301,POS::NOUN},{pos_w302,POS::NOUN},
{pos_w303,POS::NOUN},{pos_w304,POS::NOUN},{pos_w305,POS::NOUN},
{pos_w306,POS::NOUN},{pos_w307,POS::NOUN},{pos_w308,POS::NOUN},
{pos_w309,POS::NOUN},{pos_w310,POS::NOUN},{pos_w311,POS::NOUN},
{pos_w312,POS::NOUN},{pos_w313,POS::NOUN},{pos_w314,POS::NOUN},
{pos_w315,POS::NOUN},{pos_w316,POS::NOUN},{pos_w317,POS::NOUN},
{pos_w318,POS::NOUN},{pos_w319,POS::NOUN},
// Adjectives
{pos_w320,POS::ADJECTIVE},{pos_w321,POS::ADJECTIVE},{pos_w322,POS::ADJECTIVE},
{pos_w323,POS::ADJECTIVE},{pos_w324,POS::ADJECTIVE},{pos_w325,POS::ADJECTIVE},
{pos_w326,POS::ADJECTIVE},{pos_w327,POS::ADJECTIVE},{pos_w328,POS::ADJECTIVE},
{pos_w329,POS::ADJECTIVE},{pos_w330,POS::ADJECTIVE},{pos_w331,POS::ADJECTIVE},
{pos_w332,POS::ADJECTIVE},{pos_w333,POS::ADJECTIVE},{pos_w334,POS::ADJECTIVE},
{pos_w335,POS::ADJECTIVE},{pos_w336,POS::ADJECTIVE},{pos_w337,POS::ADJECTIVE},
{pos_w338,POS::ADJECTIVE},{pos_w339,POS::ADJECTIVE},
// More verbs (past/participle forms)
{pos_w340,POS::VERB},{pos_w341,POS::VERB},{pos_w342,POS::VERB},
{pos_w343,POS::VERB},{pos_w344,POS::VERB},{pos_w345,POS::VERB},
{pos_w346,POS::VERB},{pos_w347,POS::VERB},{pos_w348,POS::VERB},
{pos_w349,POS::VERB},{pos_w350,POS::VERB},{pos_w351,POS::VERB},
{pos_w352,POS::VERB},{pos_w353,POS::VERB},{pos_w354,POS::VERB},
{pos_w355,POS::VERB},{pos_w356,POS::VERB},{pos_w357,POS::VERB},
{pos_w358,POS::VERB},{pos_w359,POS::VERB},
// Prepositions & conjunctions
{pos_w360,POS::PREPOSITION},{pos_w361,POS::PREPOSITION},{pos_w362,POS::PREPOSITION},
{pos_w363,POS::PREPOSITION},{pos_w364,POS::PREPOSITION},{pos_w365,POS::CONJUNCTION},
{pos_w366,POS::CONJUNCTION},{pos_w367,POS::CONJUNCTION},{pos_w368,POS::CONJUNCTION},
{pos_w369,POS::CONJUNCTION},
// Adverbs
{pos_w370,POS::ADVERB},{pos_w371,POS::ADVERB},{pos_w372,POS::ADVERB},
{pos_w373,POS::ADVERB},{pos_w374,POS::ADVERB},{pos_w375,POS::ADVERB},
{pos_w376,POS::ADVERB},{pos_w377,POS::ADVERB},{pos_w378,POS::ADVERB},
{pos_w379,POS::ADVERB},
// Pronouns & determiners
{pos_w380,POS::PRONOUN},{pos_w381,POS::PRONOUN},{pos_w382,POS::DETERMINER},
{pos_w383,POS::DETERMINER},{pos_w384,POS::DETERMINER},{pos_w385,POS::DETERMINER},
{pos_w386,POS::DETERMINER},{pos_w387,POS::DETERMINER},{pos_w388,POS::DETERMINER},
{pos_w389,POS::DETERMINER},
// Interjections & misc
{pos_w390,POS::INTERJECTION},{pos_w391,POS::INTERJECTION},{pos_w392,POS::NOUN},
{pos_w393,POS::NOUN},{pos_w394,POS::NOUN},{pos_w395,POS::NOUN},
{pos_w396,POS::NOUN},{pos_w397,POS::NOUN},{pos_w398,POS::NOUN},
{pos_w399,POS::NOUN},


};

const int POS_TABLE_SIZE = 394;
