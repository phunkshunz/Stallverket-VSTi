# MASKINHALLEN : STÄLLVERKET

**Phunkshunz AUDIO** · mottagare + motor. Alla kanaler kopplas in i ställverket, och motorn bygger en låt av allt som strömmar igenom: intro, groove, break, uppbyggnad, drop och klimax, i takt med din DAW. VST3 + AU (Mac), VST3 (Windows), VST3 + LV2 (Linux).

## Ladda ner

- Mac: https://github.com/phunkshunz/Stallverket-VSTi/releases/latest/download/STALLVERKET-macOS.zip
- Windows: https://github.com/phunkshunz/Stallverket-VSTi/releases/latest/download/STALLVERKET-Windows.zip
- Linux: https://github.com/phunkshunz/Stallverket-VSTi/releases/latest/download/STALLVERKET-Linux.zip

Mac: packa upp och kör `bash installera.command` i Terminal. Windows: dubbelklicka `installera.bat`.

## Så kopplar du in det (Bitwig)

1. **Inställningar > Plug-ins > Plug-in hosting mode: "Together" eller "By manufacturer".** Alla STÄLLVERKET måste ligga i samma process för att prata med varandra.
2. Lägg STÄLLVERKET **sist på varje kanal** som ska vara med i låten (kick, bas, hats, synt ...).
3. Den första du lägger in blir **CENTRAL** av sig själv, de andra tar nästa lediga **KANAL** (1-16). En CENTRAL per projekt.
4. **ROLL** står på AUTO: STÄLLVERKET lyssnar fyra takter och väljer själv (KICK, BAS, HATS, CLAP, RIDE, PERC, SYNT, ATMO, LOOP). Du kan också välja själv, även FX och VOX, eller ORÖRD = går rakt igenom. Kanaler utan ljud räknas inte med.
5. Låt dina klipp/mönster loopa och tryck play. Motorn tar över: tar bort och lägger till element, filtrerar, rullar trummor och bygger upp till droppen.

CENTRAL behandlar också ljudet på sin egen kanal, så den kan ligga på t.ex. kicken. Vill du ha den på mastern: välj ROLL = ORÖRD.

## CENTRAL

- **MALL**: hur låten byggs. KLASSISK, MINIMAL, PEAK, HYPNOS, DOCKLANDS, STÖK, DUB, BERG-O-DAL. Varje mall har egna längder, sannolikheter och övergångar (hårda klipp eller mjuka filtersvep).
- **FRÖ** + **SLUMPA**: samma mall och känsla, en helt ny låt. **SLUMPA ALLT** slumpar allt.
- **FORM**: EVIG (bygger vidare i timmar utan att upprepa sig) eller 64/128/192/256 takter som börjar om efter OUTRO.
- **ENERGI** (färre breaks, fler klimax) · **VARIATION** (hur ofta element kommer och går) · **BRYTDJUP** (hur mycket som försvinner i breaken) · **BYGGLÄNGD** · **STÖK** (stutter, trumrullar, fills) · **FILTER** (hur mycket filtren rör sig) · **ANDAS** (tystnad sista slaget innan droppet) · **DUBKAST** (eko som ringer vidare) · **DJUP** (hur mycket motorn styr totalt).
- **ÖVERGÅNG**: övergångseffekter utan brus. Tape stop sista slaget innan breaket, stutter som stiger i tonhöjd i slutet av uppbyggnaden, kickrulle ner till 1/32 och ett rum som sväller in i droppet.
- **MJUKHET**: hur mjukt motorn byter. Högt värde: element glider in och ut med filter under upp till två takter (kick och bas lyfts ur med högpass, synt och atmo stängs med lågpass, nya element kommer som en filtrerad försmak och fullt på ettan). Bara en grupp kanaler byter per 8-taktsfras, och uppbyggnader, breaks och lyft blir aldrig kortare än 8 takter. In i droppet glider ingenting: där smäller det. 0 = tvära byten som förr.
- **TRYCK**: mättnad som gör varje kanal tätare och hårdare utan att den blir högre (nivån räknas tillbaka). Kick och bas tål mest, hats och atmo minst. Motorn trycker hårdast i droppet och klimax, minst i breaket.
- **PUMP** + **PUMPTID**: kicken pumpar bas, synt, atmo och de andra kanalerna, utan någon sidechain-routing. Pumpen följer motorn: tystnar kicken i breaket, slutar det pumpa.
- **MIDI UT**: CENTRAL skickar MIDI på kanal 16 när en ny del börjar. Not C1 = INTRO, C#1 = GROOVE, D1 = LYFT, D#1 = BREAK, E1 = BYGG, F1 = DROP, F#1 = KLIMAX, G1 = UTANDNING, G#1 = OUTRO. CC 20 = delens nummer, CC 21 = energi. I Bitwig: lägg en *Note Receiver* på ett annat spår och välj STÄLLVERKET som källa, så kan du byta scener eller mönster i BETONG/KOLV när låten byter del.
- **Controller**: BREAK NU, BYGG NU, DROP NU, HÅLL och SLÄPP finns som parametrar. Mappa dem till knappar på din controller i Bitwig.
- Liten ratt per kanal: hur mycket motorn får styra just den kanalen.
- **BREAK NU / BYGG NU / DROP NU**: ta över från nästa takt. **HÅLL**: loopa delen som spelar nu. **SLÄPP**: motorn kör vidare själv.
- Tidslinjen visar delarna och vad varje kanal gör de kommande 56 takterna.

## Flikarna LÅTEN, KURVA, MALL och TRICK

- **TRICK**: roliga effekter som motorn kastar in själv. Tänd = får hända, **HUR OFTA** styr hur ofta.
  - *I breaket*: FRYS, HALVTID, BAKLÄNGES, TOMRUM, RADIOBYTE, DUBKEDJA (ekot matar sig självt mot rundgång och klipps tvärt i droppet)
  - *In i droppet*: VINYLBROMS, KORT EKO, BASSVEP (basen sveper upp under droppets första takt), MULLER (kicken dov och mullrande första halvan av bygget, klarnar mot droppet)
  - *I groovet*: STAMNING, BANDGLAPP, KANALBYTE, SPÖKEKO, STOPP-SLAG, FASFÖRSKJUTNING (en kanal glider en 1/16 var åttonde takt, som hos Reich), FILTERBÅGE (långsam filterbåge genom delen), KICKPAUS (kicken andas ut i 16-taktsfrasens slut), BASDYK (basen lyfts ur med högpass sista slaget i frasen och smäller tillbaka på ettan)
- **Nya mallar**: SPEGEL (bygger upp och samma väg tillbaka), STAPLA (en kanal i taget, sedan ras), FALSKT DROP (uppbyggnad, sedan break, droppet efter), DUELL (fråga och svar mellan kanaler), TELEFON (allt i telefonlur tills droppet), SVÄRM (element tänds och släcks hela tiden), SKRIFTEN (följer texten du skriver under TRICK, t.ex. `I16 G32 B16 U8 D16 K32 O16`), PLASTIK (en perc-kanal står fast hela låten, kicken hålls borta länge och smäller sedan in, claps och rides kommer och går), HOOD (temat får spela länge, ett nytt element ungefär varannan minut, inget hoppar ur).

- **Mallar efter labelsound** (hur låtarna byggs, inte hur de låter): **STOCKHOLM 96** (hårt, slagverksdrivet och loopbaserat som tidiga Drumcode, Planet Rhythm och Svek: få och korta breaks, hårda klipp, mycket stök), **RUMÄNSK** (långt och hypnotiskt som Rhadoo, Raresh och Petre Inspirescu: loopar som får ta tid, nästan inga breaks), **PERLON** (mikroförändringar, filter och dub, sällan stora drop), **KANAL** (dubtechno som Basic Channel och Chain Reaction: ett mönster som står, ekon och filter i stället för drop) och **MÖRKER** (spänning, långa byggen med muller och ett stort drop som Dubfire och SCI+TEC). PLASTIK och HOOD finns sedan förut för M_nus och M-Plant.
- **LÄS REFERENS** (under KURVA): välj en låt du gillar. Ställverket mäter energin och var basen försvinner per 8 takter i DAW:ens tempo, och gör KURVAN och SKRIFTEN av det (t.ex. `I16 G32 B16 U8 D32 G16 O16`). Mallen byts till SKRIFTEN, så nästa låt byggs som referensen. Inget ljud sparas, bara formen.
- **KURVA**: rita energin över hela setet med åtta punkter (128 till 2048 takter, 512 takter är ungefär en timme i 130 BPM). Tänd **KURVA PÅ**, så ersätter kurvan ENERGI-ratten: lågt = fler breaks och färre element, högt = fler klimax.
- **MALL**: bygg en egen mall. Längd per del, chanser (lyft, break, direkt bygg, direkt drop, klimax, utandning) och karaktär (hårda klipp, stök, dubkast, filter, atmo först). Välj **EGEN** i mallmenyn för att höra den. **KOPIERA MALL** tar mallen som spelar som startpunkt. Egna mallar sparas som `.stallmall` i `Dokument/Phunkshunz/STALLVERKET/Mallar`.
- **SPARA MIDI**: sparar låten som MIDI-fil i `Dokument/Phunkshunz/STALLVERKET/Export` och visar den i Finder. Ett spår med delarna (markörer + noter C1 och uppåt) och ett spår per kanal med en not när kanalen spelar, plus nivå (CC 11), lågpass (CC 74) och högpass (CC 75). Dra in filen i Bitwig.

## GLITCH

Effekter i stil med Effectrix, Glitch, Stutter Edit och Looperator, som motorn kastar in själv på rätt ställen, så det låter som ett grepp och inte som ett fel:

- **Frasslut**: sista ett eller två slagen i en 8-taktsfras får en gest.
- **Uppbyggnad**: sista två takterna växer (repeat som tätnar och stiger, kross som ökar, filter som svänger allt snabbare).
- **Groove**: ibland hackar ett enda 1/16-steg till, som en sekvenserad effekt.
- **Breakets sista slag**: ekokast, tape stop eller tonhopp ner.
- Aldrig i droppets första takt. Kick och bas glitchar sällan och får aldrig kross, tonhopp, baklänges eller svep.

Effekterna: **REPEAT** (1/8, 1/16, 1/32 allt tätare), **BAK** (baklänges), **TAPE** (tape stop), **GATE** (trance-gate), **KROSS** (bitkross och lägre samplingsfrekvens), **TON** (repeat en oktav ner eller upp), **EKO** (kanalen kastas in i ett skallrande eko) och **SVEP** (resonant filtersvep som går allt snabbare).

Varje kanal väljer i kanalvyn vilka effekter den får använda och hur mycket (**GLITCH**-ratten på kanalen). **GLITCH** under TRICK på CENTRAL styr alla kanaler.

## LOOPDELAREN

Ligger det en trumloop på en kanal (ROLL = LOOP eller PERC, AUTO hittar trumloopar själv) delar STÄLLVERKET den i fyra delar: **KICK**, **CLAP**, **HH** (sluten hihat) och **OH** (öppen hihat). Motorn behandlar dem som om de låg på egna kanaler: kicken kan försvinna i breaket medan hihaten står kvar, delarna kommer in en i taget i introt och byter i turordning i groovet. Det som blir över (basgång, perc, synt i loopen) följer kanalen som vanligt.

- Ingen AI, inget som drar processor: loopen delas i fyra band som alltid summerar till originalet, och varje slag hörs 30 ms innan det spelas så grindarna öppnar precis före slaget.
- Delaren lär sig loopen: vilka steg som är öppen och sluten hihat, och hur lång kicken är. Svagare basslag räknas som basgång och får vara kvar.
- **DELA LOOP** i kanalvyn slår på och av. **KICK / CLAP / HH / OH**: klicka för AUTO (motorn bestämmer), PÅ (alltid kvar) eller AV (alltid borta). Lampan lyser när delen slår.
- Med alla delar på är ljudet exakt som originalet.

## BETONG-KASSETT

Gör en STÄLLVERKET-kanal till en BETONG. Lägg STÄLLVERKET på ett tomt spår och tänd **KASSETT** i kanalvyn. Kanalen spelar ett BETONG-kit (44 kit) med ett BETONG-mönster (189 mönster), och motorn leker med det ihop med alla andra kanaler:

- Varje spår i mönstret behandlas som en egen kanal: kick, hats, clap, perc, bas och stab kommer in en i taget, försvinner i breaket och byter i turordning.
- **LEK** styr hur mycket motorn leker: byter till grannmönstret ur samma mapp (B-mönstret) när låten byter del, spelar kick 2/rumble i byggen och breaks, byter perc A och B, lägger fills i frasslut och tätar hihatsen i uppbyggnaden.
- Ljudet går sedan genom ställverket som vanligt: filter, stutter, rum, pump och TRYCK.
- Ljuden läses från **Musik/MASKINHALLEN/Ljudbank**. Ljudbanken följer med i zippen och installeras av installationsskriptet. Den är tänkt att delas av alla MASKINHALLEN-maskiner. Saknas den spelar kassetten egna syntade trumljud.
- **MAPP**: välj en egen mapp med ljud. De 16 första ljudfilerna (bokstavsordning) blir pad 1-16: 1 kick, 2 kick 2/rumble, 3 sluten hat, 4 öppen hat, 5 clap, 6 snare, 7-12 perc, 13 bas, 14 stab, 15 lead/textur, 16 röst/FX. Avbryt i dialogen = tillbaka till BETONG-kitet.

## REC

**REC** på CENTRAL spelar in varje kanal som en egen WAV-fil (24 bit) i `Skrivbord/STÄLLVERKET inspelningar/<datum och tid>/`, till exempel `01 KICK.wav`, `02 BAS.wav`. Alla börjar och slutar på samma hela takt, så filerna ligger exakt i linje när du drar in dem i en DAW. Trycker du REC när transporten står börjar inspelningen när du trycker play. När du stoppar lägger CENTRAL ihop alla kanalerna till **`00 HELA LÅTEN.wav`** i samma mapp (24 bit, toppen högst -1 dB, inget klipps). Det som ligger efter STÄLLVERKET i kanalerna eller på mastern kommer inte med; vill du ha exakt masterljudet, lägg CENTRAL sist på mastern med ROLL = ORÖRD, då blir den `MIX (CENTRAL).wav`.

## Licens

Fullversionen låses upp med en nyckel från Payhip. Klistra in den en gång, så hittar alla MASKINHALLEN-maskiner på datorn den. En bundle-nyckel låser upp allt. Utan nyckel går ljudet rakt igenom.

## Presets

68 fabrikspresets (motor-presets), varav 16 efter labelsound (STOCKHOLM 96, RUMÄNSK, PERLON, KANAL och MÖRKER). Egna sparas med **SPARA** som `.stallverket`-filer i
`Dokument/Phunkshunz/STALLVERKET/Presets`. Lägg en mapp med `.stallverket`-filer där (ett preset-pack), så syns de i listan under EGNA.

Filformatet är vanlig XML:

```xml
<STALLVERKET_PRESET namn="Mitt ställverk" version="1" mall="4" fro="96" form="0"
  energi="0.75" variation="0.45" brytdjup="0.5" bygglangd="0.3" stok="0.7"
  filter="0.4" andas="0.6" dub="0.05"/>
```

## Bra att veta

- Motorn följer DAW:ens takt och position. Står transporten still går allt rakt igenom.
- Alla kanaler räknar ut samma plan själva, så de byter del exakt samtidigt.
- Hittar en KANAL ingen CENTRAL kör den vidare på de senaste inställningarna den fick (de sparas i projektet).
- Allt som spelas spelas in i en buffert per kanal; det är den stutter och baklänges läser ur.

## Bygga själv

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
c++ -std=c++17 -Isrc tests/motor_test.cpp -o motor_test && ./motor_test
c++ -std=c++17 -O2 -Isrc tests/delare_test.cpp -o delare_test && ./delare_test
c++ -std=c++17 -O2 -Isrc tests/kassett_test.cpp -o kassett_test && ./kassett_test
c++ -std=c++17 -O2 -Isrc tests/glitch_test.cpp -o glitch_test && ./glitch_test
c++ -std=c++17 -O2 -Isrc tests/referens_test.cpp -o referens_test && ./referens_test
```
