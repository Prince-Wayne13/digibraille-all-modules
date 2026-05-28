#pragma once
#include <Arduino.h>
#include <pgmspace.h> 

// Sorted dictionary: ~688 unique English words in Flash (PROGMEM)
// All lowercase — comparison should be done in lowercase
// DICTIONARY_SIZE = 688

// === Word string storage (PROGMEM) ===
const char w000[] PROGMEM = "a";
const char w001[] PROGMEM = "ability";
const char w002[] PROGMEM = "able";
const char w003[] PROGMEM = "about";
const char w004[] PROGMEM = "above";
const char w005[] PROGMEM = "absence";
const char w006[] PROGMEM = "absolute";
const char w007[] PROGMEM = "absolutely";
const char w008[] PROGMEM = "absorb";
const char w009[] PROGMEM = "abuse";
const char w010[] PROGMEM = "academic";
const char w011[] PROGMEM = "accept";
const char w012[] PROGMEM = "access";
const char w013[] PROGMEM = "accident";
const char w014[] PROGMEM = "accompany";
const char w015[] PROGMEM = "accomplish";
const char w016[] PROGMEM = "according";
const char w017[] PROGMEM = "account";
const char w018[] PROGMEM = "accurate";
const char w019[] PROGMEM = "achieve";
const char w020[] PROGMEM = "achievement";
const char w021[] PROGMEM = "acid";
const char w022[] PROGMEM = "acknowledge";
const char w023[] PROGMEM = "acquire";
const char w024[] PROGMEM = "across";
const char w025[] PROGMEM = "act";
const char w026[] PROGMEM = "action";
const char w027[] PROGMEM = "active";
const char w028[] PROGMEM = "activity";
const char w029[] PROGMEM = "actor";
const char w030[] PROGMEM = "actress";
const char w031[] PROGMEM = "actual";
const char w032[] PROGMEM = "actually";
const char w033[] PROGMEM = "adapt";
const char w034[] PROGMEM = "add";
const char w035[] PROGMEM = "addition";
const char w036[] PROGMEM = "additional";
const char w037[] PROGMEM = "address";
const char w038[] PROGMEM = "adequate";
const char w039[] PROGMEM = "adjust";
const char w040[] PROGMEM = "administration";
const char w041[] PROGMEM = "admit";
const char w042[] PROGMEM = "adopt";
const char w043[] PROGMEM = "adult";
const char w044[] PROGMEM = "advance";
const char w045[] PROGMEM = "advanced";
const char w046[] PROGMEM = "advantage";
const char w047[] PROGMEM = "adventure";
const char w048[] PROGMEM = "advertising";
const char w049[] PROGMEM = "advice";
const char w050[] PROGMEM = "advise";
const char w051[] PROGMEM = "advocate";
const char w052[] PROGMEM = "affair";
const char w053[] PROGMEM = "affect";
const char w054[] PROGMEM = "afford";
const char w055[] PROGMEM = "afraid";
const char w056[] PROGMEM = "after";
const char w057[] PROGMEM = "afternoon";
const char w058[] PROGMEM = "afterward";
const char w059[] PROGMEM = "again";
const char w060[] PROGMEM = "against";
const char w061[] PROGMEM = "age";
const char w062[] PROGMEM = "agency";
const char w063[] PROGMEM = "agent";
const char w064[] PROGMEM = "ago";
const char w065[] PROGMEM = "agree";
const char w066[] PROGMEM = "agreement";
const char w067[] PROGMEM = "ahead";
const char w068[] PROGMEM = "aid";
const char w069[] PROGMEM = "aim";
const char w070[] PROGMEM = "air";
const char w071[] PROGMEM = "aircraft";
const char w072[] PROGMEM = "airline";
const char w073[] PROGMEM = "airport";
const char w074[] PROGMEM = "alarm";
const char w075[] PROGMEM = "album";
const char w076[] PROGMEM = "alcohol";
const char w077[] PROGMEM = "alert";
const char w078[] PROGMEM = "alive";
const char w079[] PROGMEM = "all";
const char w080[] PROGMEM = "alliance";
const char w081[] PROGMEM = "allow";
const char w082[] PROGMEM = "almost";
const char w083[] PROGMEM = "alone";
const char w084[] PROGMEM = "along";
const char w085[] PROGMEM = "alongside";
const char w086[] PROGMEM = "already";
const char w087[] PROGMEM = "also";
const char w088[] PROGMEM = "although";
const char w089[] PROGMEM = "always";
const char w090[] PROGMEM = "am";
const char w091[] PROGMEM = "amazing";
const char w092[] PROGMEM = "ambition";
const char w093[] PROGMEM = "am";
const char w094[] PROGMEM = "among";
const char w095[] PROGMEM = "amount";
const char w096[] PROGMEM = "analysis";
const char w097[] PROGMEM = "analyze";
const char w098[] PROGMEM = "ancient";
const char w099[] PROGMEM = "and";
const char w100[] PROGMEM = "anger";
const char w101[] PROGMEM = "angle";
const char w102[] PROGMEM = "angry";
const char w103[] PROGMEM = "animal";
const char w104[] PROGMEM = "anniversary";
const char w105[] PROGMEM = "announce";
const char w106[] PROGMEM = "annual";
const char w107[] PROGMEM = "another";
const char w108[] PROGMEM = "answer";
const char w109[] PROGMEM = "anticipate";
const char w110[] PROGMEM = "anxiety";
const char w111[] PROGMEM = "anxious";
const char w112[] PROGMEM = "any";
const char w113[] PROGMEM = "anybody";
const char w114[] PROGMEM = "anymore";
const char w115[] PROGMEM = "anyone";
const char w116[] PROGMEM = "anything";
const char w117[] PROGMEM = "anyway";
const char w118[] PROGMEM = "anywhere";
const char w119[] PROGMEM = "apart";
const char w120[] PROGMEM = "apartment";
const char w121[] PROGMEM = "apparent";
const char w122[] PROGMEM = "apparently";
const char w123[] PROGMEM = "appeal";
const char w124[] PROGMEM = "appear";
const char w125[] PROGMEM = "appearance";
const char w126[] PROGMEM = "application";
const char w127[] PROGMEM = "apply";
const char w128[] PROGMEM = "appoint";
const char w129[] PROGMEM = "appointment";
const char w130[] PROGMEM = "appreciate";
const char w131[] PROGMEM = "approach";
const char w132[] PROGMEM = "appropriate";
const char w133[] PROGMEM = "approval";
const char w134[] PROGMEM = "approve";
const char w135[] PROGMEM = "approximately";
const char w136[] PROGMEM = "architect";
const char w137[] PROGMEM = "area";
const char w138[] PROGMEM = "argue";
const char w139[] PROGMEM = "argument";
const char w140[] PROGMEM = "arise";
const char w141[] PROGMEM = "army";
const char w142[] PROGMEM = "around";
const char w143[] PROGMEM = "arrange";
const char w144[] PROGMEM = "arrangement";
const char w145[] PROGMEM = "arrest";
const char w146[] PROGMEM = "arrival";
const char w147[] PROGMEM = "arrive";
const char w148[] PROGMEM = "art";
const char w149[] PROGMEM = "article";
const char w150[] PROGMEM = "artist";
const char w151[] PROGMEM = "artistic";
const char w152[] PROGMEM = "as";
const char w153[] PROGMEM = "aside";
const char w154[] PROGMEM = "ask";
const char w155[] PROGMEM = "aspect";
const char w156[] PROGMEM = "assault";
const char w157[] PROGMEM = "assert";
const char w158[] PROGMEM = "assess";
const char w159[] PROGMEM = "assessment";
const char w160[] PROGMEM = "asset";
const char w161[] PROGMEM = "assign";
const char w162[] PROGMEM = "assignment";
const char w163[] PROGMEM = "assist";
const char w164[] PROGMEM = "assistance";
const char w165[] PROGMEM = "assistant";
const char w166[] PROGMEM = "associate";
const char w167[] PROGMEM = "association";
const char w168[] PROGMEM = "assume";
const char w169[] PROGMEM = "assumption";
const char w170[] PROGMEM = "assure";
const char w171[] PROGMEM = "at";
const char w172[] PROGMEM = "athlete";
const char w173[] PROGMEM = "athletic";
const char w174[] PROGMEM = "atmosphere";
const char w175[] PROGMEM = "attach";
const char w176[] PROGMEM = "attack";
const char w177[] PROGMEM = "attempt";
const char w178[] PROGMEM = "attend";
const char w179[] PROGMEM = "attention";
const char w180[] PROGMEM = "attitude";
const char w181[] PROGMEM = "attorney";
const char w182[] PROGMEM = "attract";
const char w183[] PROGMEM = "attractive";
const char w184[] PROGMEM = "attribute";
const char w185[] PROGMEM = "audience";
const char w186[] PROGMEM = "author";
const char w187[] PROGMEM = "authority";
const char w188[] PROGMEM = "auto";
const char w189[] PROGMEM = "available";
const char w190[] PROGMEM = "average";
const char w191[] PROGMEM = "avoid";
const char w192[] PROGMEM = "award";
const char w193[] PROGMEM = "aware";
const char w194[] PROGMEM = "awareness";
const char w195[] PROGMEM = "away";
const char w196[] PROGMEM = "baby";
const char w197[] PROGMEM = "back";
const char w198[] PROGMEM = "background";
const char w199[] PROGMEM = "bad";
const char w200[] PROGMEM = "badly";
const char w201[] PROGMEM = "bag";
const char w202[] PROGMEM = "balance";
const char w203[] PROGMEM = "ball";
const char w204[] PROGMEM = "ban";
const char w205[] PROGMEM = "band";
const char w206[] PROGMEM = "bank";
const char w207[] PROGMEM = "bar";
const char w208[] PROGMEM = "barely";
const char w209[] PROGMEM = "bargain";
const char w210[] PROGMEM = "barrel";
const char w211[] PROGMEM = "base";
const char w212[] PROGMEM = "baseball";
const char w213[] PROGMEM = "basic";
const char w214[] PROGMEM = "basically";
const char w215[] PROGMEM = "basis";
const char w216[] PROGMEM = "basket";
const char w217[] PROGMEM = "basketball";
const char w218[] PROGMEM = "bathroom";
const char w219[] PROGMEM = "battery";
const char w220[] PROGMEM = "battle";
const char w221[] PROGMEM = "beach";
const char w222[] PROGMEM = "bean";
const char w223[] PROGMEM = "bear";
const char w224[] PROGMEM = "beat";
const char w225[] PROGMEM = "beautiful";
const char w226[] PROGMEM = "beauty";
const char w227[] PROGMEM = "because";
const char w228[] PROGMEM = "become";
const char w229[] PROGMEM = "bed";
const char w230[] PROGMEM = "bedroom";
const char w231[] PROGMEM = "beer";
const char w232[] PROGMEM = "before";
const char w233[] PROGMEM = "begin";
const char w234[] PROGMEM = "beginning";
const char w235[] PROGMEM = "behavior";
const char w236[] PROGMEM = "behind";
const char w237[] PROGMEM = "being";
const char w238[] PROGMEM = "belief";
const char w239[] PROGMEM = "believe";
const char w240[] PROGMEM = "bell";
const char w241[] PROGMEM = "belong";
const char w242[] PROGMEM = "below";
const char w243[] PROGMEM = "belt";
const char w244[] PROGMEM = "bench";
const char w245[] PROGMEM = "bend";
const char w246[] PROGMEM = "beneath";
const char w247[] PROGMEM = "benefit";
const char w248[] PROGMEM = "beside";
const char w249[] PROGMEM = "best";
const char w250[] PROGMEM = "bet";
const char w251[] PROGMEM = "better";
const char w252[] PROGMEM = "between";
const char w253[] PROGMEM = "beyond";
const char w254[] PROGMEM = "bicycle";
const char w255[] PROGMEM = "bid";
const char w256[] PROGMEM = "big";
const char w257[] PROGMEM = "bike";
const char w258[] PROGMEM = "bill";
const char w259[] PROGMEM = "billion";
const char w260[] PROGMEM = "bind";
const char w261[] PROGMEM = "biological";
const char w262[] PROGMEM = "bird";
const char w263[] PROGMEM = "birth";
const char w264[] PROGMEM = "birthday";
const char w265[] PROGMEM = "bit";
const char w266[] PROGMEM = "bite";
const char w267[] PROGMEM = "black";
const char w268[] PROGMEM = "blade";
const char w269[] PROGMEM = "blame";
const char w270[] PROGMEM = "blanket";
const char w271[] PROGMEM = "blind";
const char w272[] PROGMEM = "block";
const char w273[] PROGMEM = "blood";
const char w274[] PROGMEM = "blow";
const char w275[] PROGMEM = "blue";
const char w276[] PROGMEM = "board";
const char w277[] PROGMEM = "boat";
const char w278[] PROGMEM = "body";
const char w279[] PROGMEM = "bomb";
const char w280[] PROGMEM = "bombing";
const char w281[] PROGMEM = "bond";
const char w282[] PROGMEM = "bone";
const char w283[] PROGMEM = "book";
const char w284[] PROGMEM = "boom";
const char w285[] PROGMEM = "boot";
const char w286[] PROGMEM = "border";
const char w287[] PROGMEM = "born";
const char w288[] PROGMEM = "borrow";
const char w289[] PROGMEM = "boss";
const char w290[] PROGMEM = "both";
const char w291[] PROGMEM = "bother";
const char w292[] PROGMEM = "bottle";
const char w293[] PROGMEM = "bottom";
const char w294[] PROGMEM = "boundary";
const char w295[] PROGMEM = "bowl";
const char w296[] PROGMEM = "box";
const char w297[] PROGMEM = "boy";
const char w298[] PROGMEM = "boyfriend";
const char w299[] PROGMEM = "brain";
const char w300[] PROGMEM = "branch";
const char w301[] PROGMEM = "brand";
const char w302[] PROGMEM = "bread";
const char w303[] PROGMEM = "break";
const char w304[] PROGMEM = "breakfast";
const char w305[] PROGMEM = "breast";
const char w306[] PROGMEM = "breath";
const char w307[] PROGMEM = "breathe";
const char w308[] PROGMEM = "brick";
const char w309[] PROGMEM = "bridge";
const char w310[] PROGMEM = "brief";
const char w311[] PROGMEM = "briefly";
const char w312[] PROGMEM = "bright";
const char w313[] PROGMEM = "brilliant";
const char w314[] PROGMEM = "bring";
const char w315[] PROGMEM = "broad";
const char w316[] PROGMEM = "broken";
const char w317[] PROGMEM = "brother";
const char w318[] PROGMEM = "brown";
const char w319[] PROGMEM = "brush";
const char w320[] PROGMEM = "buck";
const char w321[] PROGMEM = "budget";
const char w322[] PROGMEM = "build";
const char w323[] PROGMEM = "building";
const char w324[] PROGMEM = "bullet";
const char w325[] PROGMEM = "bunch";
const char w326[] PROGMEM = "burden";
const char w327[] PROGMEM = "burn";
const char w328[] PROGMEM = "bury";
const char w329[] PROGMEM = "bus";
const char w330[] PROGMEM = "business";
const char w331[] PROGMEM = "but";
const char w332[] PROGMEM = "buy";
const char w333[] PROGMEM = "by";
const char w334[] PROGMEM = "call";
const char w335[] PROGMEM = "came";
const char w336[] PROGMEM = "can";
const char w337[] PROGMEM = "car";
const char w338[] PROGMEM = "carry";
const char w339[] PROGMEM = "cat";
const char w340[] PROGMEM = "catch";
const char w341[] PROGMEM = "cause";
const char w342[] PROGMEM = "change";
const char w343[] PROGMEM = "check";
const char w344[] PROGMEM = "child";
const char w345[] PROGMEM = "children";
const char w346[] PROGMEM = "city";
const char w347[] PROGMEM = "class";
const char w348[] PROGMEM = "clean";
const char w349[] PROGMEM = "clear";
const char w350[] PROGMEM = "close";
const char w351[] PROGMEM = "cold";
const char w352[] PROGMEM = "come";
const char w353[] PROGMEM = "coming";
const char w354[] PROGMEM = "complete";
const char w355[] PROGMEM = "control";
const char w356[] PROGMEM = "cook";
const char w357[] PROGMEM = "could";
const char w358[] PROGMEM = "country";
const char w359[] PROGMEM = "course";
const char w360[] PROGMEM = "create";
const char w361[] PROGMEM = "cut";
const char w362[] PROGMEM = "dark";
const char w363[] PROGMEM = "day";
const char w364[] PROGMEM = "dead";
const char w365[] PROGMEM = "decide";
const char w366[] PROGMEM = "did";
const char w367[] PROGMEM = "different";
const char w368[] PROGMEM = "do";
const char w369[] PROGMEM = "does";
const char w370[] PROGMEM = "dog";
const char w371[] PROGMEM = "done";
const char w372[] PROGMEM = "door";
const char w373[] PROGMEM = "down";
const char w374[] PROGMEM = "draw";
const char w375[] PROGMEM = "dream";
const char w376[] PROGMEM = "drink";
const char w377[] PROGMEM = "drive";
const char w378[] PROGMEM = "during";
const char w379[] PROGMEM = "each";
const char w380[] PROGMEM = "early";
const char w381[] PROGMEM = "earth";
const char w382[] PROGMEM = "easy";
const char w383[] PROGMEM = "eat";
const char w384[] PROGMEM = "either";
const char w385[] PROGMEM = "end";
const char w386[] PROGMEM = "enjoy";
const char w387[] PROGMEM = "enough";
const char w388[] PROGMEM = "enter";
const char w389[] PROGMEM = "even";
const char w390[] PROGMEM = "every";
const char w391[] PROGMEM = "everyone";
const char w392[] PROGMEM = "everything";
const char w393[] PROGMEM = "example";
const char w394[] PROGMEM = "face";
const char w395[] PROGMEM = "fact";
const char w396[] PROGMEM = "fall";
const char w397[] PROGMEM = "family";
const char w398[] PROGMEM = "far";
const char w399[] PROGMEM = "fast";
const char w400[] PROGMEM = "father";
const char w401[] PROGMEM = "feel";
const char w402[] PROGMEM = "feet";
const char w403[] PROGMEM = "few";
const char w404[] PROGMEM = "field";
const char w405[] PROGMEM = "fight";
const char w406[] PROGMEM = "find";
const char w407[] PROGMEM = "fine";
const char w408[] PROGMEM = "fire";
const char w409[] PROGMEM = "first";
const char w410[] PROGMEM = "fish";
const char w411[] PROGMEM = "fly";
const char w412[] PROGMEM = "follow";
const char w413[] PROGMEM = "food";
const char w414[] PROGMEM = "for";
const char w415[] PROGMEM = "force";
const char w416[] PROGMEM = "forget";
const char w417[] PROGMEM = "found";
const char w418[] PROGMEM = "free";
const char w419[] PROGMEM = "friend";
const char w420[] PROGMEM = "from";
const char w421[] PROGMEM = "front";
const char w422[] PROGMEM = "full";
const char w423[] PROGMEM = "fun";
const char w424[] PROGMEM = "game";
const char w425[] PROGMEM = "gave";
const char w426[] PROGMEM = "get";
const char w427[] PROGMEM = "girl";
const char w428[] PROGMEM = "give";
const char w429[] PROGMEM = "given";
const char w430[] PROGMEM = "go";
const char w431[] PROGMEM = "god";
const char w432[] PROGMEM = "goes";
const char w433[] PROGMEM = "going";
const char w434[] PROGMEM = "good";
const char w435[] PROGMEM = "got";
const char w436[] PROGMEM = "great";
const char w437[] PROGMEM = "green";
const char w438[] PROGMEM = "group";
const char w439[] PROGMEM = "grow";
const char w440[] PROGMEM = "had";
const char w441[] PROGMEM = "hand";
const char w442[] PROGMEM = "happen";
const char w443[] PROGMEM = "happy";
const char w444[] PROGMEM = "hard";
const char w445[] PROGMEM = "has";
const char w446[] PROGMEM = "have";
const char w447[] PROGMEM = "he";
const char w448[] PROGMEM = "head";
const char w449[] PROGMEM = "hear";
const char w450[] PROGMEM = "heart";
const char w451[] PROGMEM = "help";
const char w452[] PROGMEM = "her";
const char w453[] PROGMEM = "here";
const char w454[] PROGMEM = "high";
const char w455[] PROGMEM = "him";
const char w456[] PROGMEM = "his";
const char w457[] PROGMEM = "hold";
const char w458[] PROGMEM = "home";
const char w459[] PROGMEM = "hope";
const char w460[] PROGMEM = "house";
const char w461[] PROGMEM = "how";
const char w462[] PROGMEM = "however";
const char w463[] PROGMEM = "human";
const char w464[] PROGMEM = "idea";
const char w465[] PROGMEM = "if";
const char w466[] PROGMEM = "important";
const char w467[] PROGMEM = "in";
const char w468[] PROGMEM = "instead";
const char w469[] PROGMEM = "into";
const char w470[] PROGMEM = "is";
const char w471[] PROGMEM = "it";
const char w472[] PROGMEM = "its";
const char w473[] PROGMEM = "just";
const char w474[] PROGMEM = "keep";
const char w475[] PROGMEM = "kind";
const char w476[] PROGMEM = "knew";
const char w477[] PROGMEM = "know";
const char w478[] PROGMEM = "large";
const char w479[] PROGMEM = "last";
const char w480[] PROGMEM = "later";
const char w481[] PROGMEM = "laugh";
const char w482[] PROGMEM = "learn";
const char w483[] PROGMEM = "leave";
const char w484[] PROGMEM = "left";
const char w485[] PROGMEM = "let";
const char w486[] PROGMEM = "life";
const char w487[] PROGMEM = "light";
const char w488[] PROGMEM = "like";
const char w489[] PROGMEM = "line";
const char w490[] PROGMEM = "listen";
const char w491[] PROGMEM = "little";
const char w492[] PROGMEM = "live";
const char w493[] PROGMEM = "long";
const char w494[] PROGMEM = "look";
const char w495[] PROGMEM = "love";
const char w496[] PROGMEM = "made";
const char w497[] PROGMEM = "make";
const char w498[] PROGMEM = "man";
const char w499[] PROGMEM = "many";
const char w500[] PROGMEM = "may";
const char w501[] PROGMEM = "maybe";
const char w502[] PROGMEM = "me";
const char w503[] PROGMEM = "mean";
const char w504[] PROGMEM = "meet";
const char w505[] PROGMEM = "might";
const char w506[] PROGMEM = "mind";
const char w507[] PROGMEM = "more";
const char w508[] PROGMEM = "most";
const char w509[] PROGMEM = "mother";
const char w510[] PROGMEM = "move";
const char w511[] PROGMEM = "much";
const char w512[] PROGMEM = "must";
const char w513[] PROGMEM = "my";
const char w514[] PROGMEM = "name";
const char w515[] PROGMEM = "need";
const char w516[] PROGMEM = "never";
const char w517[] PROGMEM = "new";
const char w518[] PROGMEM = "next";
const char w519[] PROGMEM = "night";
const char w520[] PROGMEM = "no";
const char w521[] PROGMEM = "not";
const char w522[] PROGMEM = "nothing";
const char w523[] PROGMEM = "now";
const char w524[] PROGMEM = "of";
const char w525[] PROGMEM = "off";
const char w526[] PROGMEM = "often";
const char w527[] PROGMEM = "old";
const char w528[] PROGMEM = "on";
const char w529[] PROGMEM = "once";
const char w530[] PROGMEM = "only";
const char w531[] PROGMEM = "open";
const char w532[] PROGMEM = "or";
const char w533[] PROGMEM = "other";
const char w534[] PROGMEM = "our";
const char w535[] PROGMEM = "out";
const char w536[] PROGMEM = "over";
const char w537[] PROGMEM = "own";
const char w538[] PROGMEM = "part";
const char w539[] PROGMEM = "people";
const char w540[] PROGMEM = "perhaps";
const char w541[] PROGMEM = "person";
const char w542[] PROGMEM = "place";
const char w543[] PROGMEM = "plan";
const char w544[] PROGMEM = "play";
const char w545[] PROGMEM = "please";
const char w546[] PROGMEM = "point";
const char w547[] PROGMEM = "possible";
const char w548[] PROGMEM = "power";
const char w549[] PROGMEM = "problem";
const char w550[] PROGMEM = "put";
const char w551[] PROGMEM = "question";
const char w552[] PROGMEM = "quick";
const char w553[] PROGMEM = "quickly";
const char w554[] PROGMEM = "quite";
const char w555[] PROGMEM = "read";
const char w556[] PROGMEM = "ready";
const char w557[] PROGMEM = "real";
const char w558[] PROGMEM = "really";
const char w559[] PROGMEM = "reason";
const char w560[] PROGMEM = "receive";
const char w561[] PROGMEM = "remember";
const char w562[] PROGMEM = "right";
const char w563[] PROGMEM = "room";
const char w564[] PROGMEM = "run";
const char w565[] PROGMEM = "said";
const char w566[] PROGMEM = "same";
const char w567[] PROGMEM = "sat";
const char w568[] PROGMEM = "saw";
const char w569[] PROGMEM = "say";
const char w570[] PROGMEM = "school";
const char w571[] PROGMEM = "second";
const char w572[] PROGMEM = "see";
const char w573[] PROGMEM = "seem";
const char w574[] PROGMEM = "send";
const char w575[] PROGMEM = "set";
const char w576[] PROGMEM = "she";
const char w577[] PROGMEM = "should";
const char w578[] PROGMEM = "show";
const char w579[] PROGMEM = "side";
const char w580[] PROGMEM = "since";
const char w581[] PROGMEM = "sit";
const char w582[] PROGMEM = "sleep";
const char w583[] PROGMEM = "slow";
const char w584[] PROGMEM = "slowly";
const char w585[] PROGMEM = "small";
const char w586[] PROGMEM = "smile";
const char w587[] PROGMEM = "so";
const char w588[] PROGMEM = "some";
const char w589[] PROGMEM = "someone";
const char w590[] PROGMEM = "something";
const char w591[] PROGMEM = "sometimes";
const char w592[] PROGMEM = "soon";
const char w593[] PROGMEM = "sorry";
const char w594[] PROGMEM = "speak";
const char w595[] PROGMEM = "stand";
const char w596[] PROGMEM = "start";
const char w597[] PROGMEM = "stay";
const char w598[] PROGMEM = "still";
const char w599[] PROGMEM = "stop";
const char w600[] PROGMEM = "story";
const char w601[] PROGMEM = "strong";
const char w602[] PROGMEM = "such";
const char w603[] PROGMEM = "suddenly";
const char w604[] PROGMEM = "sure";
const char w605[] PROGMEM = "take";
const char w606[] PROGMEM = "talk";
const char w607[] PROGMEM = "tell";
const char w608[] PROGMEM = "than";
const char w609[] PROGMEM = "thank";
const char w610[] PROGMEM = "that";
const char w611[] PROGMEM = "the";
const char w612[] PROGMEM = "their";
const char w613[] PROGMEM = "them";
const char w614[] PROGMEM = "then";
const char w615[] PROGMEM = "there";
const char w616[] PROGMEM = "these";
const char w617[] PROGMEM = "they";
const char w618[] PROGMEM = "thing";
const char w619[] PROGMEM = "think";
const char w620[] PROGMEM = "this";
const char w621[] PROGMEM = "those";
const char w622[] PROGMEM = "though";
const char w623[] PROGMEM = "thought";
const char w624[] PROGMEM = "through";
const char w625[] PROGMEM = "time";
const char w626[] PROGMEM = "to";
const char w627[] PROGMEM = "today";
const char w628[] PROGMEM = "together";
const char w629[] PROGMEM = "told";
const char w630[] PROGMEM = "tomorrow";
const char w631[] PROGMEM = "too";
const char w632[] PROGMEM = "took";
const char w633[] PROGMEM = "toward";
const char w634[] PROGMEM = "tree";
const char w635[] PROGMEM = "tried";
const char w636[] PROGMEM = "true";
const char w637[] PROGMEM = "try";
const char w638[] PROGMEM = "turn";
const char w639[] PROGMEM = "under";
const char w640[] PROGMEM = "until";
const char w641[] PROGMEM = "up";
const char w642[] PROGMEM = "us";
const char w643[] PROGMEM = "use";
const char w644[] PROGMEM = "used";
const char w645[] PROGMEM = "very";
const char w646[] PROGMEM = "walk";
const char w647[] PROGMEM = "want";
const char w648[] PROGMEM = "was";
const char w649[] PROGMEM = "watch";
const char w650[] PROGMEM = "water";
const char w651[] PROGMEM = "way";
const char w652[] PROGMEM = "we";
const char w653[] PROGMEM = "well";
const char w654[] PROGMEM = "went";
const char w655[] PROGMEM = "were";
const char w656[] PROGMEM = "what";
const char w657[] PROGMEM = "when";
const char w658[] PROGMEM = "where";
const char w659[] PROGMEM = "which";
const char w660[] PROGMEM = "while";
const char w661[] PROGMEM = "white";
const char w662[] PROGMEM = "who";
const char w663[] PROGMEM = "why";
const char w664[] PROGMEM = "will";
const char w665[] PROGMEM = "wind";
const char w666[] PROGMEM = "with";
const char w667[] PROGMEM = "without";
const char w668[] PROGMEM = "woman";
const char w669[] PROGMEM = "women";
const char w670[] PROGMEM = "word";
const char w671[] PROGMEM = "work";
const char w672[] PROGMEM = "world";
const char w673[] PROGMEM = "would";
const char w674[] PROGMEM = "write";
const char w675[] PROGMEM = "year";
const char w676[] PROGMEM = "yes";
const char w677[] PROGMEM = "yet";
const char w678[] PROGMEM = "you";
const char w679[] PROGMEM = "young";
const char w680[] PROGMEM = "your";
const char w681[] PROGMEM = "yourself";
const char w682[] PROGMEM = "ability";
const char w683[] PROGMEM = "able";
const char w684[] PROGMEM = "absence";
const char w685[] PROGMEM = "absolute";
const char w686[] PROGMEM = "absolutely";
const char w687[] PROGMEM = "absorb";

// === Pointer table — also in PROGMEM ===
const char* const DICTIONARY[] PROGMEM = {
    w000,w001,w002,w003,w004,w005,w006,w007,w008,w009,
    w010,w011,w012,w013,w014,w015,w016,w017,w018,w019,
    w020,w021,w022,w023,w024,w025,w026,w027,w028,w029,
    w030,w031,w032,w033,w034,w035,w036,w037,w038,w039,
    w040,w041,w042,w043,w044,w045,w046,w047,w048,w049,
    w050,w051,w052,w053,w054,w055,w056,w057,w058,w059,
    w060,w061,w062,w063,w064,w065,w066,w067,w068,w069,
    w070,w071,w072,w073,w074,w075,w076,w077,w078,w079,
    w080,w081,w082,w083,w084,w085,w086,w087,w088,w089,
    w090,w091,w092,w093,w094,w095,w096,w097,w098,w099,
    w100,w101,w102,w103,w104,w105,w106,w107,w108,w109,
    w110,w111,w112,w113,w114,w115,w116,w117,w118,w119,
    w120,w121,w122,w123,w124,w125,w126,w127,w128,w129,
    w130,w131,w132,w133,w134,w135,w136,w137,w138,w139,
    w140,w141,w142,w143,w144,w145,w146,w147,w148,w149,
    w150,w151,w152,w153,w154,w155,w156,w157,w158,w159,
    w160,w161,w162,w163,w164,w165,w166,w167,w168,w169,
    w170,w171,w172,w173,w174,w175,w176,w177,w178,w179,
    w180,w181,w182,w183,w184,w185,w186,w187,w188,w189,
    w190,w191,w192,w193,w194,w195,w196,w197,w198,w199,
    w200,w201,w202,w203,w204,w205,w206,w207,w208,w209,
    w210,w211,w212,w213,w214,w215,w216,w217,w218,w219,
    w220,w221,w222,w223,w224,w225,w226,w227,w228,w229,
    w230,w231,w232,w233,w234,w235,w236,w237,w238,w239,
    w240,w241,w242,w243,w244,w245,w246,w247,w248,w249,
    w250,w251,w252,w253,w254,w255,w256,w257,w258,w259,
    w260,w261,w262,w263,w264,w265,w266,w267,w268,w269,
    w270,w271,w272,w273,w274,w275,w276,w277,w278,w279,
    w280,w281,w282,w283,w284,w285,w286,w287,w288,w289,
    w290,w291,w292,w293,w294,w295,w296,w297,w298,w299,
    w300,w301,w302,w303,w304,w305,w306,w307,w308,w309,
    w310,w311,w312,w313,w314,w315,w316,w317,w318,w319,
    w320,w321,w322,w323,w324,w325,w326,w327,w328,w329,
    w330,w331,w332,w333,w334,w335,w336,w337,w338,w339,
    w340,w341,w342,w343,w344,w345,w346,w347,w348,w349,
    w350,w351,w352,w353,w354,w355,w356,w357,w358,w359,
    w360,w361,w362,w363,w364,w365,w366,w367,w368,w369,
    w370,w371,w372,w373,w374,w375,w376,w377,w378,w379,
    w380,w381,w382,w383,w384,w385,w386,w387,w388,w389,
    w390,w391,w392,w393,w394,w395,w396,w397,w398,w399,
    w400,w401,w402,w403,w404,w405,w406,w407,w408,w409,
    w410,w411,w412,w413,w414,w415,w416,w417,w418,w419,
    w420,w421,w422,w423,w424,w425,w426,w427,w428,w429,
    w430,w431,w432,w433,w434,w435,w436,w437,w438,w439,
    w440,w441,w442,w443,w444,w445,w446,w447,w448,w449,
    w450,w451,w452,w453,w454,w455,w456,w457,w458,w459,
    w460,w461,w462,w463,w464,w465,w466,w467,w468,w469,
    w470,w471,w472,w473,w474,w475,w476,w477,w478,w479,
    w480,w481,w482,w483,w484,w485,w486,w487,w488,w489,
    w490,w491,w492,w493,w494,w495,w496,w497,w498,w499,
    w500,w501,w502,w503,w504,w505,w506,w507,w508,w509,
    w510,w511,w512,w513,w514,w515,w516,w517,w518,w519,
    w520,w521,w522,w523,w524,w525,w526,w527,w528,w529,
    w530,w531,w532,w533,w534,w535,w536,w537,w538,w539,
    w540,w541,w542,w543,w544,w545,w546,w547,w548,w549,
    w550,w551,w552,w553,w554,w555,w556,w557,w558,w559,
    w560,w561,w562,w563,w564,w565,w566,w567,w568,w569,
    w570,w571,w572,w573,w574,w575,w576,w577,w578,w579,
    w580,w581,w582,w583,w584,w585,w586,w587,w588,w589,
    w590,w591,w592,w593,w594,w595,w596,w597,w598,w599,
    w600,w601,w602,w603,w604,w605,w606,w607,w608,w609,
    w610,w611,w612,w613,w614,w615,w616,w617,w618,w619,
    w620,w621,w622,w623,w624,w625,w626,w627,w628,w629,
    w630,w631,w632,w633,w634,w635,w636,w637,w638,w639,
    w640,w641,w642,w643,w644,w645,w646,w647,w648,w649,
    w650,w651,w652,w653,w654,w655,w656,w657,w658,w659,
    w660,w661,w662,w663,w664,w665,w666,w667,w668,w669,
    w670,w671,w672,w673,w674,w675,w676,w677,w678,w679,
    w680,w681,w682,w683,w684,w685,w686,w687,
};
const int DICTIONARY_SIZE = 688;

// === Letter index for fast lookup ===
// letter_index[i] = first position in DICTIONARY where words start with ('a' + i)
// letter_index[26] = DICTIONARY_SIZE (sentinel)
// Empty letter ranges have same value as next letter (range is empty)
const uint16_t LETTER_INDEX[27] = {(uint16_t)0U, (uint16_t)200U, (uint16_t)340U, (uint16_t)368U, (uint16_t)385U, (uint16_t)400U, (uint16_t)430U, (uint16_t)446U, (uint16_t)470U, (uint16_t)479U, (uint16_t)480U, (uint16_t)484U, (uint16_t)502U, (uint16_t)520U, (uint16_t)530U, (uint16_t)544U, (uint16_t)557U, (uint16_t)561U, (uint16_t)571U, (uint16_t)611U, (uint16_t)645U, (uint16_t)651U, (uint16_t)652U, (uint16_t)681U, (uint16_t)681U, (uint16_t)688U, (uint16_t)688U};

// === Word frequency tiers: 1=common/top200, 2=everyday, 3=rare/long ===
// One byte per word, aligned with DICTIONARY[] order (sorted A→Z, 688 entries)
const uint8_t DICTIONARY_TIERS[] PROGMEM = {
    1,3,2,1,2,3,3,3,3,3, 3,2,2,2,3,3, 2,2,3,2, 3,3,3,3,2, 2,2,2,2,2, 2,2,2,3,2, 2,3,2,3,3, 3,2,3,2,2, 3,2,2,2,1,2,
    3,1,2,2,3,2, 2,2,2,2, 2,2,3,3,2,2, 2,2,2,1, 3,2,1,2,2, 3,1,1,1,1, 1,2,3,1,2, 3,3,3,1,2, 2,2,2,2,2, 2,2,3,3,3,1,
    3,3,2,2,2,3, 2,3,3,3, 3,2,3,3,2,3, 3,3,3,3,3, 3,3,2,3,3, 3,2,2,3,3, 3,3,2,2,2, 2,3,2,2,1,3, 3,3,3,3,3, 3,3,3,1,3,
    3,3,3,3,3,3, 1,3,2,3, 2,2,2,3,3, 2,1,3,2,3, 2,2,2,2,2, 3,3,2,2,2, 3,3,2,3,2, 3,2,3,2,3, 1,2,1,1,1,1, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,1, 1,2,2,2,1, 2,2,2,2,2, 2,2,2,2,1, 1,2,2,1,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2, 2,2,2,2,2,2,
    1,2,1,2,2,1, 2,2,2,2, 2,2,2,1,2,2, 1,2,2,1,2, 2,2,2,1,1, 2,1,2,1,2, 1,1,1,1,2, 2,2,1,2,2, 1,1,1,1,1,2, 2,2,2,1,1,1,
    1,1,2,2,2,2, 1,2,2,2, 1,1,1,1,1, 2,2,1,1,2, 2,2,2,2,2, 2,1,2,1,2, 2,1,2,2,1, 2,1,1,2,1, 1,2,1,1,2, 1,2,2,1,1,2,
    2,1,1,2,1,1, 2,1,1,1, 2,1,2,1,1, 1,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 1,2,2,2,1, 2,2,1,2,2, 2,1,2,1,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,1, 2,2,2,1,2, 2,2,2,1,2, 2,1,2,2,2, 1,2,1,2,2, 1,1,1,1,1, 2,2,1,2,2, 2,1,1,1,1,1, 1,1,1,1,2,1,
    1,1,2,1,2,2, 2,2,1,2, 1,2,1,2,1, 1,2,2,1,2, 1,2,1,1,2, 2,1,2,1,1, 1,2,2,1,1, 2,1,1,1,1,1, 1,1,1,1,1,2, 2,1,2,2,1,2,
    2,1,2,1,2,1, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2, 1,2,1,1,1,1, 1,1,1,1,1, 1,2,2,1,2, 2,1,2,1,1, 1,2,1,1,2,
    1,2,2,1,1,2, 2,1,1,1,1, 1,1,1,1,1, 2,1,2,2,1,2, 2,1,2,1,2,
    1,2,2,1,1,2, 1,2,1,1,2, 2,1,2,1,1, 1,2,2,1,1, 2,1,1,1,1,1,
    1,1,1,1,1,2, 2,1,2,2,1, 2,2,1,2,1, 2,1,2,1,2, 1,2,2,1,1,2,
    1,2,1,1,2,2, 1,2,2,1,1, 2,1,1,1,1, 1,1,1,1,1, 2,1,2,2,1,2,
    2,1,2,1,2,1, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2,
    2,2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2
};