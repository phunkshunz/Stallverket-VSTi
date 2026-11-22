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
3. Den första du lägger in blir **CENTRAL** av sig själv, de andra tar nästa lediga **KANAL** (1-8). En CENTRAL per projekt.
4. Välj **ROLL** på varje kanal: KICK, BAS, HATS, PERC, SYNT, ATMO, FX, LOOP (eller ORÖRD = går rakt igenom).
5. Låt dina klipp/mönster loopa och tryck play. Motorn tar över: tar bort och lägger till element, filtrerar, rullar trummor och bygger upp till droppen.

CENTRAL behandlar också ljudet på sin egen kanal, så den kan ligga på t.ex. kicken. Vill du ha den på mastern: välj ROLL = ORÖRD.

## CENTRAL

- **MALL**: hur låten byggs. KLASSISK, MINIMAL, PEAK, HYPNOS, DOCKLANDS, STÖK, DUB, BERG-O-DAL. Varje mall har egna längder, sannolikheter och övergångar (hårda klipp eller mjuka filtersvep).
- **FRÖ** + **SLUMPA**: samma mall och känsla, en helt ny låt. **SLUMPA ALLT** slumpar allt.
- **FORM**: EVIG (bygger vidare i timmar utan att upprepa sig) eller 64/128/192/256 takter som börjar om efter OUTRO.
- **ENERGI** (färre breaks, fler klimax) · **VARIATION** (hur ofta element kommer och går) · **BRYTDJUP** (hur mycket som försvinner i breaken) · **BYGGLÄNGD** · **STÖK** (stutter, trumrullar, fills) · **FILTER** (hur mycket filtren rör sig) · **ANDAS** (tystnad sista slaget innan droppet) · **DUBKAST** (eko som ringer vidare) · **DJUP** (hur mycket motorn styr totalt).
- **PUMP** + **PUMPTID**: kicken pumpar bas, synt, atmo och de andra kanalerna, utan någon sidechain-routing. Pumpen följer motorn: tystnar kicken i breaket, slutar det pumpa.
- **MIDI UT**: CENTRAL skickar MIDI på kanal 16 när en ny del börjar. Not C1 = INTRO, C#1 = GROOVE, D1 = LYFT, D#1 = BREAK, E1 = BYGG, F1 = DROP, F#1 = KLIMAX, G1 = UTANDNING, G#1 = OUTRO. CC 20 = delens nummer, CC 21 = energi. I Bitwig: lägg en *Note Receiver* på ett annat spår och välj STÄLLVERKET som källa, så kan du byta scener eller mönster i BETONG/KOLV när låten byter del.
- **Controller**: BREAK NU, BYGG NU, DROP NU, HÅLL och SLÄPP finns som parametrar. Mappa dem till knappar på din controller i Bitwig.
- Liten ratt per kanal: hur mycket motorn får styra just den kanalen.
- **BREAK NU / BYGG NU / DROP NU**: ta över från nästa takt. **HÅLL**: loopa delen som spelar nu. **SLÄPP**: motorn kör vidare själv.
- Tidslinjen visar delarna och vad varje kanal gör de kommande 56 takterna.

## Presets

40 fabrikspresets (motor-presets). Egna sparas med **SPARA** som `.stallverket`-filer i
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
```
