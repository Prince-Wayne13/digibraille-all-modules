#pragma once
#include <Arduino.h>
#include <pgmspace.h>

// Common misspellings → correct word
// Stored in Flash (PROGMEM)
// Checked BEFORE running expensive Levenshtein

struct HotfixEntry {
    const char* wrong;
    const char* right;
};

const char hf_wrong000[] PROGMEM = "teh";        const char hf_right000[] PROGMEM = "the";
const char hf_wrong001[] PROGMEM = "recieve";    const char hf_right001[] PROGMEM = "receive";
const char hf_wrong002[] PROGMEM = "beleive";    const char hf_right002[] PROGMEM = "believe";
const char hf_wrong003[] PROGMEM = "wierd";      const char hf_right003[] PROGMEM = "weird";
const char hf_wrong004[] PROGMEM = "freind";     const char hf_right004[] PROGMEM = "friend";
const char hf_wrong005[] PROGMEM = "thier";      const char hf_right005[] PROGMEM = "their";
const char hf_wrong006[] PROGMEM = "wont";       const char hf_right006[] PROGMEM = "won't";
const char hf_wrong007[] PROGMEM = "cant";       const char hf_right007[] PROGMEM = "can't";
const char hf_wrong008[] PROGMEM = "dont";       const char hf_right008[] PROGMEM = "don't";
const char hf_wrong009[] PROGMEM = "doesnt";     const char hf_right009[] PROGMEM = "doesn't";
const char hf_wrong010[] PROGMEM = "isnt";       const char hf_right010[] PROGMEM = "isn't";
const char hf_wrong011[] PROGMEM = "arent";      const char hf_right011[] PROGMEM = "aren't";
const char hf_wrong012[] PROGMEM = "wasnt";      const char hf_right012[] PROGMEM = "wasn't";
const char hf_wrong013[] PROGMEM = "werent";     const char hf_right013[] PROGMEM = "weren't";
const char hf_wrong014[] PROGMEM = "didnt";      const char hf_right014[] PROGMEM = "didn't";
const char hf_wrong015[] PROGMEM = "couldnt";    const char hf_right015[] PROGMEM = "couldn't";
const char hf_wrong016[] PROGMEM = "wouldnt";    const char hf_right016[] PROGMEM = "wouldn't";
const char hf_wrong017[] PROGMEM = "shouldnt";   const char hf_right017[] PROGMEM = "shouldn't";
const char hf_wrong018[] PROGMEM = "im";         const char hf_right018[] PROGMEM = "I'm";
const char hf_wrong019[] PROGMEM = "ive";        const char hf_right019[] PROGMEM = "I've";
const char hf_wrong020[] PROGMEM = "id";         const char hf_right020[] PROGMEM = "I'd";
const char hf_wrong021[] PROGMEM = "ill";        const char hf_right021[] PROGMEM = "I'll";
const char hf_wrong022[] PROGMEM = "youre";      const char hf_right022[] PROGMEM = "you're";
const char hf_wrong023[] PROGMEM = "theyre";     const char hf_right023[] PROGMEM = "they're";
const char hf_wrong024[] PROGMEM = "weve";       const char hf_right024[] PROGMEM = "we've";
const char hf_wrong025[] PROGMEM = "hes";        const char hf_right025[] PROGMEM = "he's";
const char hf_wrong026[] PROGMEM = "shes";       const char hf_right026[] PROGMEM = "she's";
const char hf_wrong027[] PROGMEM = "its";        const char hf_right027[] PROGMEM = "it's";
const char hf_wrong028[] PROGMEM = "tomorow";    const char hf_right028[] PROGMEM = "tomorrow";
const char hf_wrong029[] PROGMEM = "tommorow";   const char hf_right029[] PROGMEM = "tomorrow";
const char hf_wrong030[] PROGMEM = "tommorrow";  const char hf_right030[] PROGMEM = "tomorrow";
const char hf_wrong301[] PROGMEM = "tomorroow";  const char hf_right301[] PROGMEM = "tomorrow";
const char hf_wrong031[] PROGMEM = "begining";   const char hf_right031[] PROGMEM = "beginning";
const char hf_wrong032[] PROGMEM = "occured";    const char hf_right032[] PROGMEM = "occurred";
const char hf_wrong033[] PROGMEM = "occurance";  const char hf_right033[] PROGMEM = "occurrence";
const char hf_wrong034[] PROGMEM = "seperate";   const char hf_right034[] PROGMEM = "separate";
const char hf_wrong035[] PROGMEM = "definately"; const char hf_right035[] PROGMEM = "definitely";
const char hf_wrong036[] PROGMEM = "defiantly";  const char hf_right036[] PROGMEM = "definitely";
const char hf_wrong037[] PROGMEM = "goverment";  const char hf_right037[] PROGMEM = "government";
const char hf_wrong038[] PROGMEM = "enviroment"; const char hf_right038[] PROGMEM = "environment";
const char hf_wrong039[] PROGMEM = "experiance"; const char hf_right039[] PROGMEM = "experience";
const char hf_wrong040[] PROGMEM = "knowlege";   const char hf_right040[] PROGMEM = "knowledge";
const char hf_wrong041[] PROGMEM = "bussiness";  const char hf_right041[] PROGMEM = "business";
const char hf_wrong042[] PROGMEM = "busines";    const char hf_right042[] PROGMEM = "business";
const char hf_wrong043[] PROGMEM = "grammer";    const char hf_right043[] PROGMEM = "grammar";
const char hf_wrong044[] PROGMEM = "writting";   const char hf_right044[] PROGMEM = "writing";
const char hf_wrong045[] PROGMEM = "writeing";   const char hf_right045[] PROGMEM = "writing";
const char hf_wrong046[] PROGMEM = "comming";    const char hf_right046[] PROGMEM = "coming";
const char hf_wrong047[] PROGMEM = "runing";     const char hf_right047[] PROGMEM = "running";
const char hf_wrong302[] PROGMEM = "ru";         const char hf_right302[] PROGMEM = "run";
const char hf_wrong048[] PROGMEM = "stoping";    const char hf_right048[] PROGMEM = "stopping";
const char hf_wrong049[] PROGMEM = "hopeing";    const char hf_right049[] PROGMEM = "hoping";
const char hf_wrong050[] PROGMEM = "haveing";    const char hf_right050[] PROGMEM = "having";
const char hf_wrong051[] PROGMEM = "makeing";    const char hf_right051[] PROGMEM = "making";
const char hf_wrong052[] PROGMEM = "takeing";    const char hf_right052[] PROGMEM = "taking";
const char hf_wrong053[] PROGMEM = "liveing";    const char hf_right053[] PROGMEM = "living";
const char hf_wrong054[] PROGMEM = "giveing";    const char hf_right054[] PROGMEM = "giving";
const char hf_wrong055[] PROGMEM = "loveing";    const char hf_right055[] PROGMEM = "loving";
const char hf_wrong056[] PROGMEM = "alot";       const char hf_right056[] PROGMEM = "a lot";
const char hf_wrong057[] PROGMEM = "allright";   const char hf_right057[] PROGMEM = "alright";
const char hf_wrong058[] PROGMEM = "alright";    const char hf_right058[] PROGMEM = "all right";
const char hf_wrong059[] PROGMEM = "noone";      const char hf_right059[] PROGMEM = "no one";
const char hf_wrong060[] PROGMEM = "everytime";  const char hf_right060[] PROGMEM = "every time";
const char hf_wrong061[] PROGMEM = "aswell";     const char hf_right061[] PROGMEM = "as well";
const char hf_wrong062[] PROGMEM = "ofcourse";   const char hf_right062[] PROGMEM = "of course";
const char hf_wrong063[] PROGMEM = "thankyou";   const char hf_right063[] PROGMEM = "thank you";
const char hf_wrong064[] PROGMEM = "untill";     const char hf_right064[] PROGMEM = "until";
const char hf_wrong065[] PROGMEM = "accomodate"; const char hf_right065[] PROGMEM = "accommodate";
const char hf_wrong066[] PROGMEM = "acheive";    const char hf_right066[] PROGMEM = "achieve";
const char hf_wrong067[] PROGMEM = "accross";    const char hf_right067[] PROGMEM = "across";
const char hf_wrong068[] PROGMEM = "agressive";  const char hf_right068[] PROGMEM = "aggressive";
const char hf_wrong069[] PROGMEM = "apparant";   const char hf_right069[] PROGMEM = "apparent";
const char hf_wrong070[] PROGMEM = "arguement";  const char hf_right070[] PROGMEM = "argument";
const char hf_wrong071[] PROGMEM = "basicly";    const char hf_right071[] PROGMEM = "basically";
const char hf_wrong072[] PROGMEM = "calender";   const char hf_right072[] PROGMEM = "calendar";
const char hf_wrong073[] PROGMEM = "catagory";   const char hf_right073[] PROGMEM = "category";
const char hf_wrong074[] PROGMEM = "collegue";   const char hf_right074[] PROGMEM = "colleague";
const char hf_wrong075[] PROGMEM = "concious";   const char hf_right075[] PROGMEM = "conscious";
const char hf_wrong076[] PROGMEM = "continous";  const char hf_right076[] PROGMEM = "continuous";
const char hf_wrong077[] PROGMEM = "convience";  const char hf_right077[] PROGMEM = "convenience";
const char hf_wrong078[] PROGMEM = "curiousity"; const char hf_right078[] PROGMEM = "curiosity";
const char hf_wrong079[] PROGMEM = "dependant";  const char hf_right079[] PROGMEM = "dependent";
const char hf_wrong080[] PROGMEM = "existance";  const char hf_right080[] PROGMEM = "existence";
const char hf_wrong081[] PROGMEM = "foriegn";    const char hf_right081[] PROGMEM = "foreign";
const char hf_wrong082[] PROGMEM = "fourty";     const char hf_right082[] PROGMEM = "forty";
const char hf_wrong083[] PROGMEM = "futher";     const char hf_right083[] PROGMEM = "further";
const char hf_wrong084[] PROGMEM = "gaurd";      const char hf_right084[] PROGMEM = "guard";
const char hf_wrong085[] PROGMEM = "glamourous"; const char hf_right085[] PROGMEM = "glamorous";
const char hf_wrong086[] PROGMEM = "greatful";   const char hf_right086[] PROGMEM = "grateful";
const char hf_wrong087[] PROGMEM = "guarentee";  const char hf_right087[] PROGMEM = "guarantee";
const char hf_wrong088[] PROGMEM = "happend";    const char hf_right088[] PROGMEM = "happened";
const char hf_wrong089[] PROGMEM = "harrass";    const char hf_right089[] PROGMEM = "harass";
const char hf_wrong090[] PROGMEM = "humerous";   const char hf_right090[] PROGMEM = "humorous";
const char hf_wrong091[] PROGMEM = "ignorance";  const char hf_right091[] PROGMEM = "ignorance";
const char hf_wrong092[] PROGMEM = "immediatly"; const char hf_right092[] PROGMEM = "immediately";
const char hf_wrong093[] PROGMEM = "independant";const char hf_right093[] PROGMEM = "independent";
const char hf_wrong094[] PROGMEM = "intresting"; const char hf_right094[] PROGMEM = "interesting";
const char hf_wrong095[] PROGMEM = "irresistable";const char hf_right095[] PROGMEM = "irresistible";
const char hf_wrong096[] PROGMEM = "jewelery";   const char hf_right096[] PROGMEM = "jewellery";
const char hf_wrong097[] PROGMEM = "judgement";  const char hf_right097[] PROGMEM = "judgment";
const char hf_wrong098[] PROGMEM = "knowlege";   const char hf_right098[] PROGMEM = "knowledge";
const char hf_wrong099[] PROGMEM = "liason";     const char hf_right099[] PROGMEM = "liaison";
const char hf_wrong100[] PROGMEM = "recieved";   const char hf_right100[] PROGMEM = "received";
const char hf_wrong101[] PROGMEM = "seperate";   const char hf_right101[] PROGMEM = "separate";
const char hf_wrong102[] PROGMEM = "definately"; const char hf_right102[] PROGMEM = "definitely";
const char hf_wrong103[] PROGMEM = "occured";    const char hf_right103[] PROGMEM = "occurred";
const char hf_wrong104[] PROGMEM = "occurence";  const char hf_right104[] PROGMEM = "occurrence";
const char hf_wrong105[] PROGMEM = "maintainence"; const char hf_right105[] PROGMEM = "maintenance";
const char hf_wrong106[] PROGMEM = "publically"; const char hf_right106[] PROGMEM = "publicly";
const char hf_wrong107[] PROGMEM = "suprise";    const char hf_right107[] PROGMEM = "surprise";
const char hf_wrong108[] PROGMEM = "suprised";   const char hf_right108[] PROGMEM = "surprised";
const char hf_wrong109[] PROGMEM = "suprising";  const char hf_right109[] PROGMEM = "surprising";
const char hf_wrong110[] PROGMEM = "thru";       const char hf_right110[] PROGMEM = "through";
const char hf_wrong111[] PROGMEM = "alot";       const char hf_right111[] PROGMEM = "a lot";
const char hf_wrong112[] PROGMEM = "alright";    const char hf_right112[] PROGMEM = "all right";
const char hf_wrong113[] PROGMEM = "anyway";     const char hf_right113[] PROGMEM = "any way";
const char hf_wrong114[] PROGMEM = "everyday";   const char hf_right114[] PROGMEM = "every day";
const char hf_wrong115[] PROGMEM = "everytime";  const char hf_right115[] PROGMEM = "every time";
const char hf_wrong116[] PROGMEM = "anymore";    const char hf_right116[] PROGMEM = "any more";
const char hf_wrong117[] PROGMEM = "alot";       const char hf_right117[] PROGMEM = "a lot";
const char hf_wrong118[] PROGMEM = "kindof";     const char hf_right118[] PROGMEM = "kind of";
const char hf_wrong119[] PROGMEM = "sortof";     const char hf_right119[] PROGMEM = "sort of";
const char hf_wrong120[] PROGMEM = "alot";       const char hf_right120[] PROGMEM = "a lot";
const char hf_wrong121[] PROGMEM = "couldnt";    const char hf_right121[] PROGMEM = "couldn't";
const char hf_wrong122[] PROGMEM = "wouldnt";    const char hf_right122[] PROGMEM = "wouldn't";
const char hf_wrong123[] PROGMEM = "shouldnt";   const char hf_right123[] PROGMEM = "shouldn't";
const char hf_wrong124[] PROGMEM = "mightnt";    const char hf_right124[] PROGMEM = "mightn't";
const char hf_wrong125[] PROGMEM = "mustnt";     const char hf_right125[] PROGMEM = "mustn't";
const char hf_wrong126[] PROGMEM = "neednt";     const char hf_right126[] PROGMEM = "needn't";
const char hf_wrong127[] PROGMEM = "darent";     const char hf_right127[] PROGMEM = "daren't";
const char hf_wrong128[] PROGMEM = "oughtnt";    const char hf_right128[] PROGMEM = "oughtn't";
const char hf_wrong129[] PROGMEM = "shant";      const char hf_right129[] PROGMEM = "shan't";
const char hf_wrong130[] PROGMEM = "whos";       const char hf_right130[] PROGMEM = "who's";
const char hf_wrong131[] PROGMEM = "whoms";      const char hf_right131[] PROGMEM = "whom's";
const char hf_wrong132[] PROGMEM = "whoses";     const char hf_right132[] PROGMEM = "whose";
const char hf_wrong133[] PROGMEM = "its";        const char hf_right133[] PROGMEM = "it's";
const char hf_wrong134[] PROGMEM = "their";      const char hf_right134[] PROGMEM = "they're";
const char hf_wrong135[] PROGMEM = "there";      const char hf_right135[] PROGMEM = "their";
const char hf_wrong136[] PROGMEM = "your";       const char hf_right136[] PROGMEM = "you're";
const char hf_wrong137[] PROGMEM = "were";       const char hf_right137[] PROGMEM = "we're";
const char hf_wrong138[] PROGMEM = "lets";       const char hf_right138[] PROGMEM = "let's";
const char hf_wrong139[] PROGMEM = "thats";      const char hf_right139[] PROGMEM = "that's";
const char hf_wrong140[] PROGMEM = "whats";      const char hf_right140[] PROGMEM = "what's";
const char hf_wrong141[] PROGMEM = "heres";      const char hf_right141[] PROGMEM = "here's";
const char hf_wrong142[] PROGMEM = "theres";     const char hf_right142[] PROGMEM = "there's";
const char hf_wrong143[] PROGMEM = "wheres";     const char hf_right143[] PROGMEM = "where's";
const char hf_wrong144[] PROGMEM = "hows";       const char hf_right144[] PROGMEM = "how's";
const char hf_wrong145[] PROGMEM = "whens";      const char hf_right145[] PROGMEM = "when's";
const char hf_wrong146[] PROGMEM = "whys";       const char hf_right146[] PROGMEM = "why's";
const char hf_wrong147[] PROGMEM = "aint";       const char hf_right147[] PROGMEM = "ain't";
const char hf_wrong148[] PROGMEM = "gonna";      const char hf_right148[] PROGMEM = "going to";
const char hf_wrong149[] PROGMEM = "wanna";      const char hf_right149[] PROGMEM = "want to";
const char hf_wrong150[] PROGMEM = "gotta";      const char hf_right150[] PROGMEM = "got to";
const char hf_wrong151[] PROGMEM = "lemme";      const char hf_right151[] PROGMEM = "let me";
const char hf_wrong152[] PROGMEM = "gimme";      const char hf_right152[] PROGMEM = "give me";
const char hf_wrong153[] PROGMEM = "kinda";      const char hf_right153[] PROGMEM = "kind of";
const char hf_wrong154[] PROGMEM = "sorta";      const char hf_right154[] PROGMEM = "sort of";
const char hf_wrong155[] PROGMEM = "outta";      const char hf_right155[] PROGMEM = "out of";
const char hf_wrong156[] PROGMEM = "intta";      const char hf_right156[] PROGMEM = "into the";
const char hf_wrong157[] PROGMEM = "dunno";      const char hf_right157[] PROGMEM = "don't know";
const char hf_wrong158[] PROGMEM = "coulda";     const char hf_right158[] PROGMEM = "could have";
const char hf_wrong159[] PROGMEM = "woulda";     const char hf_right159[] PROGMEM = "would have";
const char hf_wrong160[] PROGMEM = "shoulda";    const char hf_right160[] PROGMEM = "should have";
const char hf_wrong161[] PROGMEM = "mighta";     const char hf_right161[] PROGMEM = "might have";
const char hf_wrong162[] PROGMEM = "musta";      const char hf_right162[] PROGMEM = "must have";
const char hf_wrong163[] PROGMEM = "needa";      const char hf_right163[] PROGMEM = "need to";
const char hf_wrong164[] PROGMEM = "hafta";      const char hf_right164[] PROGMEM = "have to";
const char hf_wrong165[] PROGMEM = "oughta";     const char hf_right165[] PROGMEM = "ought to";
const char hf_wrong166[] PROGMEM = "useta";      const char hf_right166[] PROGMEM = "used to";
const char hf_wrong167[] PROGMEM = "supposeta";  const char hf_right167[] PROGMEM = "supposed to";
const char hf_wrong168[] PROGMEM = "kinda";      const char hf_right168[] PROGMEM = "kind of";
const char hf_wrong169[] PROGMEM = "sorta";      const char hf_right169[] PROGMEM = "sort of";
const char hf_wrong170[] PROGMEM = "alotta";     const char hf_right170[] PROGMEM = "a lot of";
const char hf_wrong171[] PROGMEM = "plentya";    const char hf_right171[] PROGMEM = "plenty of";
const char hf_wrong172[] PROGMEM = "coupla";     const char hf_right172[] PROGMEM = "couple of";
const char hf_wrong173[] PROGMEM = "buncha";     const char hf_right173[] PROGMEM = "bunch of";
const char hf_wrong174[] PROGMEM = "heap";       const char hf_right174[] PROGMEM = "heaps of";
const char hf_wrong175[] PROGMEM = "ton";        const char hf_right175[] PROGMEM = "tons of";
const char hf_wrong176[] PROGMEM = "bunch";      const char hf_right176[] PROGMEM = "bunches of";
const char hf_wrong177[] PROGMEM = "kind";       const char hf_right177[] PROGMEM = "kinds of";
const char hf_wrong178[] PROGMEM = "sort";       const char hf_right178[] PROGMEM = "sorts of";
const char hf_wrong179[] PROGMEM = "type";       const char hf_right179[] PROGMEM = "types of";
const char hf_wrong180[] PROGMEM = "variety";    const char hf_right180[] PROGMEM = "varieties of";
const char hf_wrong181[] PROGMEM = "number";     const char hf_right181[] PROGMEM = "numbers of";
const char hf_wrong182[] PROGMEM = "amount";     const char hf_right182[] PROGMEM = "amounts of";
const char hf_wrong183[] PROGMEM = "piece";      const char hf_right183[] PROGMEM = "pieces of";
const char hf_wrong184[] PROGMEM = "bit";        const char hf_right184[] PROGMEM = "bits of";
const char hf_wrong185[] PROGMEM = "part";       const char hf_right185[] PROGMEM = "parts of";
const char hf_wrong186[] PROGMEM = "section";    const char hf_right186[] PROGMEM = "sections of";
const char hf_wrong187[] PROGMEM = "portion";    const char hf_right187[] PROGMEM = "portions of";
const char hf_wrong188[] PROGMEM = "fraction";   const char hf_right188[] PROGMEM = "fractions of";
const char hf_wrong189[] PROGMEM = "percentage"; const char hf_right189[] PROGMEM = "percentages of";
const char hf_wrong190[] PROGMEM = "ratio";      const char hf_right190[] PROGMEM = "ratios of";
const char hf_wrong191[] PROGMEM = "rate";       const char hf_right191[] PROGMEM = "rates of";
const char hf_wrong192[] PROGMEM = "speed";      const char hf_right192[] PROGMEM = "speeds of";
const char hf_wrong193[] PROGMEM = "velocity";   const char hf_right193[] PROGMEM = "velocities of";
const char hf_wrong194[] PROGMEM = "acceleration"; const char hf_right194[] PROGMEM = "accelerations of";
const char hf_wrong195[] PROGMEM = "force";      const char hf_right195[] PROGMEM = "forces of";
const char hf_wrong196[] PROGMEM = "energy";     const char hf_right196[] PROGMEM = "energies of";
const char hf_wrong197[] PROGMEM = "power";      const char hf_right197[] PROGMEM = "powers of";
const char hf_wrong198[] PROGMEM = "work";       const char hf_right198[] PROGMEM = "works of";
const char hf_wrong199[] PROGMEM = "heat";       const char hf_right199[] PROGMEM = "heats of";
const char hf_wrong200[] PROGMEM = "light";      const char hf_right200[] PROGMEM = "lights of";
const char hf_wrong201[] PROGMEM = "sound";      const char hf_right201[] PROGMEM = "sounds of";
const char hf_wrong202[] PROGMEM = "wave";       const char hf_right202[] PROGMEM = "waves of";
const char hf_wrong203[] PROGMEM = "particle";   const char hf_right203[] PROGMEM = "particles of";
const char hf_wrong204[] PROGMEM = "atom";       const char hf_right204[] PROGMEM = "atoms of";
const char hf_wrong205[] PROGMEM = "molecule";   const char hf_right205[] PROGMEM = "molecules of";
const char hf_wrong206[] PROGMEM = "element";    const char hf_right206[] PROGMEM = "elements of";
const char hf_wrong207[] PROGMEM = "compound";   const char hf_right207[] PROGMEM = "compounds of";
const char hf_wrong208[] PROGMEM = "solution";   const char hf_right208[] PROGMEM = "solutions of";
const char hf_wrong209[] PROGMEM = "mixture";    const char hf_right209[] PROGMEM = "mixtures of";
const char hf_wrong210[] PROGMEM = "reaction";   const char hf_right210[] PROGMEM = "reactions of";
const char hf_wrong211[] PROGMEM = "process";    const char hf_right211[] PROGMEM = "processes of";
const char hf_wrong212[] PROGMEM = "system";     const char hf_right212[] PROGMEM = "systems of";
const char hf_wrong213[] PROGMEM = "structure";  const char hf_right213[] PROGMEM = "structures of";
const char hf_wrong214[] PROGMEM = "function";   const char hf_right214[] PROGMEM = "functions of";
const char hf_wrong215[] PROGMEM = "method";     const char hf_right215[] PROGMEM = "methods of";
const char hf_wrong216[] PROGMEM = "technique";  const char hf_right216[] PROGMEM = "techniques of";
const char hf_wrong217[] PROGMEM = "approach";   const char hf_right217[] PROGMEM = "approaches of";
const char hf_wrong218[] PROGMEM = "strategy";   const char hf_right218[] PROGMEM = "strategies of";
const char hf_wrong219[] PROGMEM = "plan";       const char hf_right219[] PROGMEM = "plans of";
const char hf_wrong220[] PROGMEM = "design";     const char hf_right220[] PROGMEM = "designs of";
const char hf_wrong221[] PROGMEM = "pattern";    const char hf_right221[] PROGMEM = "patterns of";
const char hf_wrong222[] PROGMEM = "model";      const char hf_right222[] PROGMEM = "models of";
const char hf_wrong223[] PROGMEM = "theory";     const char hf_right223[] PROGMEM = "theories of";
const char hf_wrong224[] PROGMEM = "hypothesis"; const char hf_right224[] PROGMEM = "hypotheses of";
const char hf_wrong225[] PROGMEM = "experiment"; const char hf_right225[] PROGMEM = "experiments of";
const char hf_wrong226[] PROGMEM = "observation";const char hf_right226[] PROGMEM = "observations of";
const char hf_wrong227[] PROGMEM = "measurement";const char hf_right227[] PROGMEM = "measurements of";
const char hf_wrong228[] PROGMEM = "calculation";const char hf_right228[] PROGMEM = "calculations of";
const char hf_wrong229[] PROGMEM = "analysis";   const char hf_right229[] PROGMEM = "analyses of";
const char hf_wrong230[] PROGMEM = "synthesis";  const char hf_right230[] PROGMEM = "syntheses of";
const char hf_wrong231[] PROGMEM = "evaluation"; const char hf_right231[] PROGMEM = "evaluations of";
const char hf_wrong232[] PROGMEM = "assessment"; const char hf_right232[] PROGMEM = "assessments of";
const char hf_wrong233[] PROGMEM = "judgment";   const char hf_right233[] PROGMEM = "judgments of";
const char hf_wrong234[] PROGMEM = "decision";   const char hf_right234[] PROGMEM = "decisions of";
const char hf_wrong235[] PROGMEM = "choice";     const char hf_right235[] PROGMEM = "choices of";
const char hf_wrong236[] PROGMEM = "option";     const char hf_right236[] PROGMEM = "options of";
const char hf_wrong237[] PROGMEM = "alternative";const char hf_right237[] PROGMEM = "alternatives of";
const char hf_wrong238[] PROGMEM = "possibility";const char hf_right238[] PROGMEM = "possibilities of";
const char hf_wrong239[] PROGMEM = "probability";const char hf_right239[] PROGMEM = "probabilities of";
const char hf_wrong240[] PROGMEM = "chance";     const char hf_right240[] PROGMEM = "chances of";
const char hf_wrong241[] PROGMEM = "risk";       const char hf_right241[] PROGMEM = "risks of";
const char hf_wrong242[] PROGMEM = "danger";     const char hf_right242[] PROGMEM = "dangers of";
const char hf_wrong243[] PROGMEM = "threat";     const char hf_right243[] PROGMEM = "threats of";
const char hf_wrong244[] PROGMEM = "opportunity";const char hf_right244[] PROGMEM = "opportunities of";
const char hf_wrong245[] PROGMEM = "advantage";  const char hf_right245[] PROGMEM = "advantages of";
const char hf_wrong246[] PROGMEM = "disadvantage";const char hf_right246[] PROGMEM = "disadvantages of";
const char hf_wrong247[] PROGMEM = "benefit";    const char hf_right247[] PROGMEM = "benefits of";
const char hf_wrong248[] PROGMEM = "cost";       const char hf_right248[] PROGMEM = "costs of";
const char hf_wrong249[] PROGMEM = "price";      const char hf_right249[] PROGMEM = "prices of";
const char hf_wrong250[] PROGMEM = "value";      const char hf_right250[] PROGMEM = "values of";
const char hf_wrong251[] PROGMEM = "worth";      const char hf_right251[] PROGMEM = "worth of";
const char hf_wrong252[] PROGMEM = "quality";    const char hf_right252[] PROGMEM = "qualities of";
const char hf_wrong253[] PROGMEM = "quantity";   const char hf_right253[] PROGMEM = "quantities of";
const char hf_wrong254[] PROGMEM = "size";       const char hf_right254[] PROGMEM = "sizes of";
const char hf_wrong255[] PROGMEM = "shape";      const char hf_right255[] PROGMEM = "shapes of";
const char hf_wrong256[] PROGMEM = "color";      const char hf_right256[] PROGMEM = "colors of";
const char hf_wrong257[] PROGMEM = "texture";    const char hf_right257[] PROGMEM = "textures of";
const char hf_wrong258[] PROGMEM = "weight";     const char hf_right258[] PROGMEM = "weights of";
const char hf_wrong259[] PROGMEM = "height";     const char hf_right259[] PROGMEM = "heights of";
const char hf_wrong260[] PROGMEM = "length";     const char hf_right260[] PROGMEM = "lengths of";
const char hf_wrong261[] PROGMEM = "width";      const char hf_right261[] PROGMEM = "widths of";
const char hf_wrong262[] PROGMEM = "depth";      const char hf_right262[] PROGMEM = "depths of";
const char hf_wrong263[] PROGMEM = "volume";     const char hf_right263[] PROGMEM = "volumes of";
const char hf_wrong264[] PROGMEM = "area";       const char hf_right264[] PROGMEM = "areas of";
const char hf_wrong265[] PROGMEM = "space";      const char hf_right265[] PROGMEM = "spaces of";
const char hf_wrong266[] PROGMEM = "distance";   const char hf_right266[] PROGMEM = "distances of";
const char hf_wrong267[] PROGMEM = "direction";  const char hf_right267[] PROGMEM = "directions of";
const char hf_wrong268[] PROGMEM = "position";   const char hf_right268[] PROGMEM = "positions of";
const char hf_wrong269[] PROGMEM = "location";   const char hf_right269[] PROGMEM = "locations of";
const char hf_wrong270[] PROGMEM = "place";      const char hf_right270[] PROGMEM = "places of";
const char hf_wrong271[] PROGMEM = "spot";       const char hf_right271[] PROGMEM = "spots of";
const char hf_wrong272[] PROGMEM = "point";      const char hf_right272[] PROGMEM = "points of";
const char hf_wrong273[] PROGMEM = "line";       const char hf_right273[] PROGMEM = "lines of";
const char hf_wrong274[] PROGMEM = "curve";      const char hf_right274[] PROGMEM = "curves of";
const char hf_wrong275[] PROGMEM = "angle";      const char hf_right275[] PROGMEM = "angles of";
const char hf_wrong276[] PROGMEM = "circle";     const char hf_right276[] PROGMEM = "circles of";
const char hf_wrong277[] PROGMEM = "square";     const char hf_right277[] PROGMEM = "squares of";
const char hf_wrong278[] PROGMEM = "rectangle";  const char hf_right278[] PROGMEM = "rectangles of";
const char hf_wrong279[] PROGMEM = "triangle";   const char hf_right279[] PROGMEM = "triangles of";
const char hf_wrong280[] PROGMEM = "polygon";    const char hf_right280[] PROGMEM = "polygons of";
const char hf_wrong281[] PROGMEM = "sphere";     const char hf_right281[] PROGMEM = "spheres of";
const char hf_wrong282[] PROGMEM = "cube";       const char hf_right282[] PROGMEM = "cubes of";
const char hf_wrong283[] PROGMEM = "cylinder";   const char hf_right283[] PROGMEM = "cylinders of";
const char hf_wrong284[] PROGMEM = "cone";       const char hf_right284[] PROGMEM = "cones of";
const char hf_wrong285[] PROGMEM = "pyramid";    const char hf_right285[] PROGMEM = "pyramids of";
const char hf_wrong286[] PROGMEM = "prism";      const char hf_right286[] PROGMEM = "prisms of";
const char hf_wrong287[] PROGMEM = "ellipsoid";  const char hf_right287[] PROGMEM = "ellipsoids of";
const char hf_wrong288[] PROGMEM = "paraboloid"; const char hf_right288[] PROGMEM = "paraboloids of";
const char hf_wrong289[] PROGMEM = "hyperboloid";const char hf_right289[] PROGMEM = "hyperboloids of";
const char hf_wrong290[] PROGMEM = "torus";      const char hf_right290[] PROGMEM = "tori of";
const char hf_wrong291[] PROGMEM = "surface";    const char hf_right291[] PROGMEM = "surfaces of";
const char hf_wrong292[] PROGMEM = "edge";       const char hf_right292[] PROGMEM = "edges of";
const char hf_wrong293[] PROGMEM = "vertex";     const char hf_right293[] PROGMEM = "vertices of";
const char hf_wrong294[] PROGMEM = "face";       const char hf_right294[] PROGMEM = "faces of";
const char hf_wrong295[] PROGMEM = "side";       const char hf_right295[] PROGMEM = "sides of";
const char hf_wrong296[] PROGMEM = "corner";     const char hf_right296[] PROGMEM = "corners of";
const char hf_wrong297[] PROGMEM = "base";       const char hf_right297[] PROGMEM = "bases of";
const char hf_wrong298[] PROGMEM = "top";        const char hf_right298[] PROGMEM = "tops of";
const char hf_wrong299[] PROGMEM = "bottom";     const char hf_right299[] PROGMEM = "bottoms of";
const char hf_wrong300[] PROGMEM = "flowrs";     const char hf_right300[] PROGMEM = "flowers";

const char* const HF_WRONG[] PROGMEM = {
    hf_wrong000,hf_wrong001,hf_wrong002,hf_wrong003,hf_wrong004,
    hf_wrong005,hf_wrong006,hf_wrong007,hf_wrong008,hf_wrong009,
    hf_wrong010,hf_wrong011,hf_wrong012,hf_wrong013,hf_wrong014,
    hf_wrong015,hf_wrong016,hf_wrong017,hf_wrong018,hf_wrong019,
    hf_wrong020,hf_wrong021,hf_wrong022,hf_wrong023,hf_wrong024,
    hf_wrong025,hf_wrong026,hf_wrong027,hf_wrong028,hf_wrong029,
    hf_wrong030,hf_wrong031,hf_wrong032,hf_wrong033,hf_wrong034,
    hf_wrong035,hf_wrong036,hf_wrong037,hf_wrong038,hf_wrong039,
    hf_wrong040,hf_wrong041,hf_wrong042,hf_wrong043,hf_wrong044,
    hf_wrong045,hf_wrong046,hf_wrong047,hf_wrong048,hf_wrong049,
    hf_wrong050,hf_wrong051,hf_wrong052,hf_wrong053,hf_wrong054,
    hf_wrong055,hf_wrong056,hf_wrong057,hf_wrong058,hf_wrong059,
    hf_wrong060,hf_wrong061,hf_wrong062,hf_wrong063,hf_wrong064,
    hf_wrong065,hf_wrong066,hf_wrong067,hf_wrong068,hf_wrong069,
    hf_wrong070,hf_wrong071,hf_wrong072,hf_wrong073,hf_wrong074,
    hf_wrong075,hf_wrong076,hf_wrong077,hf_wrong078,hf_wrong079,
    hf_wrong080,hf_wrong081,hf_wrong082,hf_wrong083,hf_wrong084,
    hf_wrong085,hf_wrong086,hf_wrong087,hf_wrong088,hf_wrong089,
    hf_wrong090,hf_wrong091,hf_wrong092,hf_wrong093,hf_wrong094,
    hf_wrong095,hf_wrong096,hf_wrong097,hf_wrong098,hf_wrong099
,hf_wrong100,hf_wrong101,hf_wrong102,hf_wrong103,hf_wrong104,
    hf_wrong105,hf_wrong106,hf_wrong107,hf_wrong108,hf_wrong109,
    hf_wrong110,hf_wrong111,hf_wrong112,hf_wrong113,hf_wrong114,
    hf_wrong115,hf_wrong116,hf_wrong117,hf_wrong118,hf_wrong119,
    hf_wrong120,hf_wrong121,hf_wrong122,hf_wrong123,hf_wrong124,
    hf_wrong125,hf_wrong126,hf_wrong127,hf_wrong128,hf_wrong129,
    hf_wrong130,hf_wrong131,hf_wrong132,hf_wrong133,hf_wrong134,
    hf_wrong135,hf_wrong136,hf_wrong137,hf_wrong138,hf_wrong139,
    hf_wrong140,hf_wrong141,hf_wrong142,hf_wrong143,hf_wrong144,
    hf_wrong145,hf_wrong146,hf_wrong147,hf_wrong148,hf_wrong149,
    hf_wrong150,hf_wrong151,hf_wrong152,hf_wrong153,hf_wrong154,
    hf_wrong155,hf_wrong156,hf_wrong157,hf_wrong158,hf_wrong159,
    hf_wrong160,hf_wrong161,hf_wrong162,hf_wrong163,hf_wrong164,
    hf_wrong165,hf_wrong166,hf_wrong167,hf_wrong168,hf_wrong169,
    hf_wrong170,hf_wrong171,hf_wrong172,hf_wrong173,hf_wrong174,
    hf_wrong175,hf_wrong176,hf_wrong177,hf_wrong178,hf_wrong179,
    hf_wrong180,hf_wrong181,hf_wrong182,hf_wrong183,hf_wrong184,
    hf_wrong185,hf_wrong186,hf_wrong187,hf_wrong188,hf_wrong189,
    hf_wrong190,hf_wrong191,hf_wrong192,hf_wrong193,hf_wrong194,
    hf_wrong195,hf_wrong196,hf_wrong197,hf_wrong198,hf_wrong199,
    hf_wrong200,hf_wrong201,hf_wrong202,hf_wrong203,hf_wrong204,
    hf_wrong205,hf_wrong206,hf_wrong207,hf_wrong208,hf_wrong209,
    hf_wrong210,hf_wrong211,hf_wrong212,hf_wrong213,hf_wrong214,
    hf_wrong215,hf_wrong216,hf_wrong217,hf_wrong218,hf_wrong219,
    hf_wrong220,hf_wrong221,hf_wrong222,hf_wrong223,hf_wrong224,
    hf_wrong225,hf_wrong226,hf_wrong227,hf_wrong228,hf_wrong229,
    hf_wrong230,hf_wrong231,hf_wrong232,hf_wrong233,hf_wrong234,
    hf_wrong235,hf_wrong236,hf_wrong237,hf_wrong238,hf_wrong239,
    hf_wrong240,hf_wrong241,hf_wrong242,hf_wrong243,hf_wrong244,
    hf_wrong245,hf_wrong246,hf_wrong247,hf_wrong248,hf_wrong249,
    hf_wrong250,hf_wrong251,hf_wrong252,hf_wrong253,hf_wrong254,
    hf_wrong255,hf_wrong256,hf_wrong257,hf_wrong258,hf_wrong259,
    hf_wrong260,hf_wrong261,hf_wrong262,hf_wrong263,hf_wrong264,
    hf_wrong265,hf_wrong266,hf_wrong267,hf_wrong268,hf_wrong269,
    hf_wrong270,hf_wrong271,hf_wrong272,hf_wrong273,hf_wrong274,
    hf_wrong275,hf_wrong276,hf_wrong277,hf_wrong278,hf_wrong279,
    hf_wrong280,hf_wrong281,hf_wrong282,hf_wrong283,hf_wrong284,
    hf_wrong285,hf_wrong286,hf_wrong287,hf_wrong288,hf_wrong289,
    hf_wrong290,hf_wrong291,hf_wrong292,hf_wrong293,hf_wrong294,
    hf_wrong295,hf_wrong296,hf_wrong297,hf_wrong298,hf_wrong299,
    hf_wrong300,hf_wrong301,hf_wrong302
};

const char* const HF_RIGHT[] PROGMEM = {
    hf_right000,hf_right001,hf_right002,hf_right003,hf_right004,
    hf_right005,hf_right006,hf_right007,hf_right008,hf_right009,
    hf_right010,hf_right011,hf_right012,hf_right013,hf_right014,
    hf_right015,hf_right016,hf_right017,hf_right018,hf_right019,
    hf_right020,hf_right021,hf_right022,hf_right023,hf_right024,
    hf_right025,hf_right026,hf_right027,hf_right028,hf_right029,
    hf_right030,hf_right031,hf_right032,hf_right033,hf_right034,
    hf_right035,hf_right036,hf_right037,hf_right038,hf_right039,
    hf_right040,hf_right041,hf_right042,hf_right043,hf_right044,
    hf_right045,hf_right046,hf_right047,hf_right048,hf_right049,
    hf_right050,hf_right051,hf_right052,hf_right053,hf_right054,
    hf_right055,hf_right056,hf_right057,hf_right058,hf_right059,
    hf_right060,hf_right061,hf_right062,hf_right063,hf_right064,
    hf_right065,hf_right066,hf_right067,hf_right068,hf_right069,
    hf_right070,hf_right071,hf_right072,hf_right073,hf_right074,
    hf_right075,hf_right076,hf_right077,hf_right078,hf_right079,
    hf_right080,hf_right081,hf_right082,hf_right083,hf_right084,
    hf_right085,hf_right086,hf_right087,hf_right088,hf_right089,
    hf_right090,hf_right091,hf_right092,hf_right093,hf_right094,
    hf_right095,hf_right096,hf_right097,hf_right098,hf_right099,
    hf_right100,hf_right101,hf_right102,hf_right103,hf_right104,
    hf_right105,hf_right106,hf_right107,hf_right108,hf_right109,
    hf_right110,hf_right111,hf_right112,hf_right113,hf_right114,
    hf_right115,hf_right116,hf_right117,hf_right118,hf_right119,
    hf_right120,hf_right121,hf_right122,hf_right123,hf_right124,
    hf_right125,hf_right126,hf_right127,hf_right128,hf_right129,
    hf_right130,hf_right131,hf_right132,hf_right133,hf_right134,
    hf_right135,hf_right136,hf_right137,hf_right138,hf_right139,
    hf_right140,hf_right141,hf_right142,hf_right143,hf_right144,
    hf_right145,hf_right146,hf_right147,hf_right148,hf_right149,
    hf_right150,hf_right151,hf_right152,hf_right153,hf_right154,
    hf_right155,hf_right156,hf_right157,hf_right158,hf_right159,
    hf_right160,hf_right161,hf_right162,hf_right163,hf_right164,
    hf_right165,hf_right166,hf_right167,hf_right168,hf_right169,
    hf_right170,hf_right171,hf_right172,hf_right173,hf_right174,
    hf_right175,hf_right176,hf_right177,hf_right178,hf_right179,
    hf_right180,hf_right181,hf_right182,hf_right183,hf_right184,
    hf_right185,hf_right186,hf_right187,hf_right188,hf_right189,
    hf_right190,hf_right191,hf_right192,hf_right193,hf_right194,
    hf_right195,hf_right196,hf_right197,hf_right198,hf_right199,
    hf_right200,hf_right201,hf_right202,hf_right203,hf_right204,
    hf_right205,hf_right206,hf_right207,hf_right208,hf_right209,
    hf_right210,hf_right211,hf_right212,hf_right213,hf_right214,
    hf_right215,hf_right216,hf_right217,hf_right218,hf_right219,
    hf_right220,hf_right221,hf_right222,hf_right223,hf_right224,
    hf_right225,hf_right226,hf_right227,hf_right228,hf_right229,
    hf_right230,hf_right231,hf_right232,hf_right233,hf_right234,
    hf_right235,hf_right236,hf_right237,hf_right238,hf_right239,
    hf_right240,hf_right241,hf_right242,hf_right243,hf_right244,
    hf_right245,hf_right246,hf_right247,hf_right248,hf_right249,
    hf_right250,hf_right251,hf_right252,hf_right253,hf_right254,
    hf_right255,hf_right256,hf_right257,hf_right258,hf_right259,
    hf_right260,hf_right261,hf_right262,hf_right263,hf_right264,
    hf_right265,hf_right266,hf_right267,hf_right268,hf_right269,
    hf_right270,hf_right271,hf_right272,hf_right273,hf_right274,
    hf_right275,hf_right276,hf_right277,hf_right278,hf_right279,
    hf_right280,hf_right281,hf_right282,hf_right283,hf_right284,
    hf_right285,hf_right286,hf_right287,hf_right288,hf_right289,
    hf_right290,hf_right291,hf_right292,hf_right293,hf_right294,
    hf_right295,hf_right296,hf_right297,hf_right298,hf_right299,
    hf_right300,hf_right301,hf_right302
};

const int HOTFIX_SIZE = 303; 
