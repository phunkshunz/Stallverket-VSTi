#pragma once

// STÄLLVERKET · motorn
//
// Bygger en låt av allt som går genom maskinen. Ingen JUCE här, bara ren C++,
// så motorn kan testas fristående (tests/motor_test.cpp).
//
// Hur det hänger ihop:
//  - Inställningarna (mall, frö, energi ...) blir en PLAN: en lista med delar
//    (INTRO, GROOVE, BREAK, BYGG, DROP, KLIMAX ...) och hur många takter de är.
//  - Planen är helt bestämd av inställningarna. Därför kan varje kanal räkna ut
//    samma plan själv, och alla kanaler hamnar alltid i samma del samtidigt.
//  - För varje kanal och varje 16-del ger motorn en ÅTGÄRD: nivå, filter,
//    grind, stutter, baklänges och dubkast, beroende på kanalens ROLL
//    (KICK, BAS, HATS ...) och var i låten vi är.

#include <array>
#include <cmath>
#include <cstdint>

namespace motor
{
enum Roll { KICK, BAS, HATS, PERC, SYNT, ATMO, FX, LOOP, CLAP, RIDE, VOX, NUM_ROLL };
enum Del  { INTRO, GROOVE, LYFT, BREAK, BYGG, DROP, KLIMAX, UTANDNING, OUTRO, NUM_DEL };

inline const char* rollNamn (int r)
{
    static const char* n[] = { "KICK", "BAS", "HATS", "PERC", "SYNT", "ATMO", "FX", "LOOP", "CLAP", "RIDE", "VOX" };
    return (r >= 0 && r < NUM_ROLL) ? n[r] : "?";
}

inline const char* delNamn (int d)
{
    static const char* n[] = { "INTRO", "GROOVE", "LYFT", "BREAK", "BYGG", "DROP", "KLIMAX", "UTANDNING", "OUTRO" };
    return (d >= 0 && d < NUM_DEL) ? n[d] : "?";
}

//==============================================================================
// Slump som alltid ger samma svar för samma tal.
inline uint32_t blanda (uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
inline uint32_t blanda (uint32_t a, uint32_t b)                         { return blanda (a * 0x9E3779B9U ^ blanda (b)); }
inline uint32_t blanda (uint32_t a, uint32_t b, uint32_t c)             { return blanda (blanda (a, b), c); }
inline uint32_t blanda (uint32_t a, uint32_t b, uint32_t c, uint32_t d) { return blanda (blanda (a, b, c), d); }
inline float slump01 (uint32_t h) { return (float) (blanda (h) & 0xffffff) / 16777216.0f; }

//==============================================================================
// MALLAR: olika sätt att bygga en låt.
enum Sarskild { S_INGEN, S_SPEGEL, S_STAPLA, S_FALSKT, S_DUELL, S_TELEFON, S_SVARM, S_SKRIFT };
struct Mall
{
    const char* namn;
    uint8_t langd[NUM_DEL];   // grundlängd i takter per del
    float pLyft, pBreak, pByggDirekt, pDropDirekt, pKlimax, pUtandning;
    float hard;     // 0 = mjuka filterövergångar, 1 = hårda klipp
    float stok;     // extra stutter och fills
    float dub;      // extra dubkast
    float filter;   // extra filterrörelse
    bool  atmoForst;// atmo/synt kommer in först i intro i stället för kicken
    int   sarskild = 0; // mallar med eget beteende, se Sarskild
};

//                      namn          IN GR LY BR BY DR KL UT OU   lyft  brk   byggD dropD klmx  utand  hård stök dub  filt atmo
inline constexpr Mall kMallar[] =
{
    { "KLASSISK",    { 16,32,16,16, 8,16,32,16,16 }, 0.35f,0.40f,0.15f,0.15f,0.50f,0.30f, 0.5f,0.3f,0.2f,0.5f, false },
    { "MINIMAL",     { 32,64,32,16, 8,32,32,32,32 }, 0.45f,0.15f,0.10f,0.30f,0.25f,0.40f, 0.2f,0.2f,0.3f,0.8f, false },
    { "PEAK",        {  8,16, 8, 8, 8,16,16, 8, 8 }, 0.20f,0.45f,0.30f,0.10f,0.70f,0.15f, 0.8f,0.6f,0.1f,0.4f, false },
    { "HYPNOS",      { 32,64,32, 8, 8,32,64,32,32 }, 0.60f,0.05f,0.10f,0.40f,0.30f,0.30f, 0.1f,0.1f,0.2f,1.0f, true  },
    { "DOCKLANDS",   { 16,32, 8, 8, 4,16,32, 8,16 }, 0.25f,0.45f,0.20f,0.50f,0.45f,0.15f, 1.0f,0.8f,0.0f,0.3f, false },
    { "STÖK",        {  8, 8, 8, 8, 4, 8, 8, 8, 8 }, 0.30f,0.40f,0.30f,0.30f,0.55f,0.20f, 0.9f,1.0f,0.2f,0.5f, false },
    { "DUB",         { 32,32,16,32,16,32,32,32,32 }, 0.30f,0.50f,0.05f,0.35f,0.25f,0.45f, 0.1f,0.1f,1.0f,0.7f, true  },
    { "BERG-O-DAL",  { 16,16,16,32,32,16,32,16,16 }, 0.15f,0.60f,0.05f,0.05f,0.75f,0.20f, 0.6f,0.5f,0.3f,0.9f, true  },
    // mallar med eget beteende
    { "SPEGEL",      { 16,32,16,16, 8,16,32,16,16 }, 0.35f,0.40f,0.15f,0.15f,0.50f,0.30f, 0.5f,0.3f,0.2f,0.5f, false, S_SPEGEL },
    { "STAPLA",      { 16,64,16, 4, 8,16,32,16,16 }, 0.05f,0.90f,0.03f,0.70f,0.30f,0.10f, 1.0f,0.3f,0.1f,0.3f, false, S_STAPLA },
    { "FALSKT DROP", { 16,32,16,16, 8,16,32,16,16 }, 0.20f,0.40f,0.35f,0.10f,0.40f,0.20f, 0.8f,0.5f,0.2f,0.5f, false, S_FALSKT },
    { "DUELL",       { 16,32,16,16, 8,16,32,16,16 }, 0.35f,0.35f,0.15f,0.20f,0.50f,0.30f, 0.6f,0.3f,0.3f,0.5f, false, S_DUELL },
    { "TELEFON",     { 16,32,16,16,16,32,32,16,16 }, 0.30f,0.35f,0.25f,0.10f,0.60f,0.25f, 0.3f,0.2f,0.4f,0.6f, true,  S_TELEFON },
    { "SVÄRM",       {  8,16, 8, 8, 8,16,16, 8, 8 }, 0.30f,0.35f,0.25f,0.25f,0.50f,0.20f, 0.9f,0.6f,0.2f,0.4f, false, S_SVARM },
    { "SKRIFTEN",    { 16,32,16,16, 8,16,32,16,16 }, 0.35f,0.40f,0.15f,0.15f,0.50f,0.30f, 0.5f,0.3f,0.2f,0.5f, false, S_SKRIFT },
};
inline constexpr int kAntalMallar = (int) (sizeof (kMallar) / sizeof (kMallar[0]));

//==============================================================================
struct Installning
{
    int      mall      = 0;
    uint32_t fro       = 1;
    int      form      = 0;     // 0 = EVIG, annars antal takter innan låten börjar om
    float    energi    = 0.5f;
    float    variation = 0.5f;
    float    brytDjup  = 0.6f;
    float    byggLangd = 0.5f;
    float    stok      = 0.4f;
    float    filter    = 0.6f;
    float    andas     = 0.4f;
    float    dub       = 0.2f;
    float    overgang  = 0.5f;  // övergångseffekter: tape stop, pitch-stutter, rum

    // EGEN mall (mall == kAntalMallar) från mallredigeraren
    Mall     egen = kMallar[0];

    // energikurva över hela setet: åtta punkter utspridda över kurvTakter
    bool     kurvaPa   = false;
    int      kurvTakter = 512;
    float    kurva[8]  { 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 1.0f, 0.7f, 0.4f };

    // TRICK: vilka effekter som är på (bitar, se Trick) och hur ofta de händer
    uint32_t trick      = 0xFFFu;
    float    trickMangd = 0.5f;
    uint32_t levande    = 0;    // vilka kanaler som har ljud (för TOMRUM), styr inte planen

    // SKRIFTEN: följd av delar, typ << 8 | längd (0 = mallens längd)
    int      skrift[32] {};
    int      skriftN = 0;

    uint32_t extra = 0;         // hash av egen mall, kurva, trick och skrift, så att jämförelsen nedan märker ändringar

    bool operator== (const Installning& o) const
    {
        return mall == o.mall && fro == o.fro && form == o.form && energi == o.energi && variation == o.variation
            && brytDjup == o.brytDjup && byggLangd == o.byggLangd && stok == o.stok && filter == o.filter
            && andas == o.andas && dub == o.dub && overgang == o.overgang && extra == o.extra;
    }
    bool operator!= (const Installning& o) const { return ! (*this == o); }
};

enum Trick : uint32_t
{
    T_FRYS = 1u << 0, T_HALVTID = 1u << 1, T_BAKBREAK = 1u << 2, T_VINYL = 1u << 3, T_RADIO = 1u << 4, T_EKO = 1u << 5,
    T_TOMRUM = 1u << 6, T_STAM = 1u << 7, T_GLAPP = 1u << 8, T_BYT = 1u << 9, T_SPOKE = 1u << 10, T_STOPP = 1u << 11
};
inline constexpr int kAntalTrick = 12;

// SKRIFTEN: "I16 G32 B U D K" -> delar. I=INTRO G=GROOVE L=LYFT B=BREAK U=BYGG (upp) D=DROP K=KLIMAX A=UTANDNING O=OUTRO
inline int lasSkrift (const char* txt, int* ut, int max)
{
    int n = 0;
    for (const char* c = txt; *c && n < max; ++c)
    {
        int typ = -1;
        switch (*c | 0x20)
        {
            case 'i': typ = INTRO; break;  case 'g': typ = GROOVE; break; case 'l': typ = LYFT; break;
            case 'b': typ = BREAK; break;  case 'u': typ = BYGG; break;   case 'd': typ = DROP; break;
            case 'k': typ = KLIMAX; break; case 'a': typ = UTANDNING; break; case 'o': typ = OUTRO; break;
            default: break;
        }
        if (typ < 0) continue;
        int l = 0;
        while (c[1] >= '0' && c[1] <= '9') { l = l * 10 + (c[1] - '0'); ++c; }
        if (l > 0) l = l < 4 ? 4 : (l > 128 ? 128 : l);
        ut[n++] = (typ << 8) | l;
    }
    return n;
}

inline const Mall& mallFor (const Installning& in)
{
    if (in.mall == kAntalMallar) return in.egen;
    return kMallar[(in.mall >= 0 && in.mall < kAntalMallar) ? in.mall : 0];
}

// Energin i en viss takt: ENERGI-ratten, eller energikurvan om den är på.
inline float energiVid (const Installning& in, int64_t takt)
{
    if (! in.kurvaPa || in.kurvTakter <= 0) return in.energi;
    if (takt < 0) takt = 0;
    const double pos = (double) (takt % in.kurvTakter) / (double) in.kurvTakter * 7.0;
    const int i = (int) pos;
    const float f = (float) (pos - i);
    const float a = in.kurva[i], b = in.kurva[i < 7 ? i + 1 : 7];
    // mjuk kurva mellan punkterna
    const float s = f * f * (3.0f - 2.0f * f);
    return a + (b - a) * s;
}

inline constexpr int kFormTakter[] = { 0, 64, 128, 192, 256 };
inline constexpr const char* kFormNamn[] = { "EVIG", "64 TAKTER", "128 TAKTER", "192 TAKTER", "256 TAKTER" };
inline constexpr int kAntalFormer = 5;

struct Sektion
{
    int     typ   = GROOVE;
    int64_t start = 0;      // första takten
    int     langd = 16;     // takter
    uint32_t h    = 0;      // egen slump för delen
    int     index = 0;
};

struct Plan
{
    static constexpr int kMax = 600;
    std::array<Sektion, kMax> s {};
    int     n = 0;
    int64_t takter = 0;     // summan av alla delar
    bool    form = false;   // fast längd: börjar om efter OUTRO

    // delen som spelar i takt 'takt' (takt räknas från 0)
    const Sektion& hitta (int64_t takt) const
    {
        if (n == 0) return s[0];
        if (takt < 0) takt = 0;
        if (takter > 0) takt %= takter;
        int lo = 0, hi = n - 1;
        while (lo < hi)
        {
            const int mid = (lo + hi + 1) / 2;
            if (s[(size_t) mid].start <= takt) lo = mid; else hi = mid - 1;
        }
        return s[(size_t) lo];
    }
};

inline int tvapotens (float x, int lo, int hi)
{
    int v = lo;
    while (v * 2 <= hi && (float) (v * 2) <= x * 1.41f) v *= 2;
    return v;
}

inline Plan byggPlan (const Installning& in)
{
    Plan p;
    const Mall& m = mallFor (in);
    const int formTakter = kFormTakter[(in.form >= 0 && in.form < kAntalFormer) ? in.form : 0];
    p.form = formTakter > 0;

    uint32_t r = blanda (in.fro + 17U, (uint32_t) in.mall + 101U);
    auto nasta = [&r] { r = blanda (r + 0x6d2b79f5U); return (float) (r & 0xffffff) / 16777216.0f; };

    float e = in.energi, pBreak = 0, pKlimax = 0, pUtand = 0;
    auto uppdateraEnergi = [&] (int64_t takt)
    {
        e = energiVid (in, takt);
        pBreak  = m.pBreak  * (1.5f - e);
        pKlimax = m.pKlimax * (0.5f + e);
        pUtand  = m.pUtandning * (1.5f - e);
    };
    uppdateraEnergi (0);

    // korta former: kortare delar, så att låten hinner gå igenom flera delar
    const float skala = p.form ? std::fmax (0.25f, std::fmin (1.0f, (float) formTakter / 256.0f)) : 1.0f;

    auto langdFor = [&] (int typ)
    {
        float l = (float) m.langd[typ] * skala;
        if (typ == BYGG)  l *= std::pow (2.0f, (in.byggLangd - 0.5f) * 2.0f);
        if (typ == BREAK) l *= 0.75f + in.brytDjup * 0.5f;
        const float v = nasta();
        if (v < in.variation * 0.25f)      l *= 0.5f;
        else if (v > 1.0f - in.variation * 0.25f) l *= 2.0f;
        return tvapotens (l, 4, 64);
    };

    auto lagg = [&] (int typ, int langd)
    {
        auto& s = p.s[(size_t) p.n];
        s.typ = typ; s.start = p.takter; s.langd = langd;
        s.h = blanda (in.fro, (uint32_t) p.n, 0xA11CE);
        s.index = p.n;
        p.takter += langd;
        ++p.n;
    };

    int grooveIRad = 0;
    auto valj = [&] (int cur)
    {
        uppdateraEnergi (p.takter);
        int nx = GROOVE;
        const float x = nasta();
        switch (cur)
        {
            case INTRO: nx = GROOVE; break;
            case GROOVE:
                if      (x < m.pLyft)                                nx = LYFT;
                else if (x < m.pLyft + pBreak)                       nx = BREAK;
                else if (x < m.pLyft + pBreak + m.pByggDirekt)       nx = BYGG;
                else                                                 nx = GROOVE;
                if (nx == GROOVE && grooveIRad >= 2) nx = (nasta() < 0.5f) ? LYFT : BREAK;
                break;
            case LYFT:      nx = x < 0.5f ? BREAK : (x < 0.75f ? BYGG : GROOVE); break;
            case BREAK:     nx = x < m.pDropDirekt ? DROP : BYGG; break;
            case BYGG:      nx = x < pKlimax ? KLIMAX : DROP; break;
            case DROP:      nx = x < pKlimax * 0.6f ? KLIMAX : (x < pKlimax * 0.6f + pUtand ? UTANDNING : GROOVE); break;
            case KLIMAX:    nx = x < pUtand ? UTANDNING : (x < pUtand + 0.3f ? BREAK : GROOVE); break;
            case UTANDNING: nx = x < 0.5f ? BREAK : GROOVE; break;
            default:        nx = GROOVE; break;
        }
        grooveIRad = (nx == GROOVE) ? grooveIRad + 1 : 0;
        return nx;
    };

    // ---- SKRIFTEN: följer den skrivna följden, om och om igen
    if (m.sarskild == S_SKRIFT && in.skriftN > 0)
    {
        int i = 0;
        while (p.n < Plan::kMax - 1 && (! p.form || p.takter < formTakter))
        {
            const int typ = in.skrift[i] >> 8, l = in.skrift[i] & 255;
            lagg (typ, l > 0 ? l : langdFor (typ));
            if (++i >= in.skriftN) i = 0;
        }
        return p;
    }

    // ---- SPEGEL: bygger upp, sedan samma väg tillbaka baklänges
    if (m.sarskild == S_SPEGEL)
    {
        const int halv = p.form ? formTakter / 2 : 64;
        bool forsta = true;
        while (p.n < Plan::kMax - 60)
        {
            const int start = p.n;
            const int64_t startTakt = p.takter;
            int c = forsta ? INTRO : GROOVE;
            lagg (c, p.form ? tvapotens ((float) m.langd[c] * skala, 4, 16) : langdFor (c));
            while (p.takter - startTakt < halv && p.n < Plan::kMax - 60)
            {
                c = valj (c);
                int l = langdFor (c);
                if (p.form)
                {
                    const int kvar = halv - (int) (p.takter - startTakt);
                    if (kvar < 4) break;
                    if (l > kvar) l = tvapotens ((float) kvar, 4, kvar);
                    if (c == BYGG && kvar - l < 4) c = GROOVE;
                }
                lagg (c, l);
            }
            if (p.s[(size_t) p.n - 1].typ == BYGG) p.s[(size_t) p.n - 1].typ = GROOVE;
            const int slut = p.n;
            for (int i = slut - 1; i >= start; --i)
            {
                int t = p.s[(size_t) i].typ;
                if (t == INTRO) t = OUTRO; else if (t == BYGG) t = UTANDNING;
                lagg (t, p.s[(size_t) i].langd);
            }
            forsta = false;
            if (p.form) break;
        }
        return p;
    }

    const int outro = p.form ? tvapotens ((float) m.langd[OUTRO] * skala, 4, 16) : m.langd[OUTRO];
    lagg (INTRO, p.form ? tvapotens ((float) m.langd[INTRO] * skala, 4, 16) : langdFor (INTRO));
    int cur = INTRO;

    while (p.n < Plan::kMax - 4)
    {
        if (p.form && p.takter >= formTakter - outro) break;
        int nx = valj (cur);

        int l = langdFor (nx);
        if (p.form)
        {
            const int kvar = formTakter - outro - (int) p.takter;
            if (kvar < 4) break;
            if (l > kvar) l = tvapotens ((float) kvar, 4, kvar);
            // en BYGG ska alltid följas av något: hinns det inte, blir det GROOVE
            if (nx == BYGG && kvar - l < 8) nx = GROOVE;
        }
        // FALSKT DROP: efter uppbyggnaden kommer ett break i stället, droppet sen
        if (m.sarskild == S_FALSKT && cur == BYGG && (nx == DROP || nx == KLIMAX)
            && (! p.form || formTakter - outro - (int) p.takter - l >= 8))
            lagg (BREAK, p.form ? 4 : 8);
        lagg (nx, l);
        cur = nx;
    }

    if (cur == BYGG) lagg (DROP, p.form ? 4 : langdFor (DROP));
    if (p.form) lagg (OUTRO, outro);
    return p;
}

//==============================================================================
// Manuell styrning från CENTRAL: BREAK NU, BYGG NU, DROP NU och HÅLL.
struct Overstyr
{
    int     typ   = -1;     // -1 = ingen
    int64_t start = 0;      // takt
    int     langd = 0;
    int     typ2  = -1;     // del som följer direkt efter (t.ex. DROP efter BYGG NU)
    int     langd2 = 0;
    bool    hall  = false;  // håller kvar delen som spelade när HÅLL trycktes
    int64_t hallStart = 0;  // delens start
    int     hallLangd = 0;
    int     hallTyp   = GROOVE;
    uint32_t hallH    = 0;
};

// Ger delen för en takt, med eventuell överstyrning, och takten omräknad
// till platsen i delen.
inline Sektion aktuell (const Plan& p, const Overstyr& o, int64_t takt, int64_t& taktIDel)
{
    if (o.typ >= 0 && takt >= o.start && takt < o.start + o.langd)
    {
        Sektion s;
        s.typ = o.typ; s.start = o.start; s.langd = o.langd;
        s.h = blanda ((uint32_t) o.start, (uint32_t) o.typ, 0xBEEF);
        s.index = 100000 + (int) (o.start & 0xffff);
        taktIDel = takt - o.start;
        return s;
    }
    if (o.typ >= 0 && o.typ2 >= 0 && takt >= o.start + o.langd && takt < o.start + o.langd + o.langd2)
    {
        Sektion s;
        s.typ = o.typ2; s.start = o.start + o.langd; s.langd = o.langd2;
        s.h = blanda ((uint32_t) s.start, (uint32_t) o.typ2, 0xBEEF);
        s.index = 100001 + (int) (o.start & 0xffff);
        taktIDel = takt - s.start;
        return s;
    }
    if (o.hall && o.hallLangd > 0 && takt >= o.hallStart)
    {
        Sektion s;
        s.typ = o.hallTyp; s.start = o.hallStart; s.langd = o.hallLangd; s.h = o.hallH;
        s.index = 200000;
        taktIDel = (takt - o.hallStart) % o.hallLangd;
        return s;
    }
    const auto& s = p.hitta (takt);
    int64_t t = takt;
    if (p.takter > 0) t %= p.takter;
    taktIDel = t - s.start;
    return s;
}

//==============================================================================
struct Atgard
{
    float    niva    = 1.0f;    // 0..1
    float    lp      = 1.0f;    // 1 = öppet
    float    hp      = 0.0f;    // 0 = öppet
    uint16_t grind   = 0xFFFF;  // vilka 16-delar som släpps igenom
    float    stutter = 0.0f;    // längd i 16-delar (4 = 1/4, 2 = 1/8, 1 = 1/16, 0.5 = 1/32), 0 = av
    bool     bak     = false;   // spelar förra slaget baklänges
    float    dub     = 0.0f;    // skick till ekot
    bool     hard    = false;   // hårt klipp i stället för mjuk övergång
    float    pump    = 0.0f;    // sidechain-pump från kicken, 0..1
    float    pumpTid = 0.4f;    // pumpens längd i slag
    float    ton     = 1.0f;    // stutterns uppspelningshastighet (2 = en oktav upp)
    bool     tapeStop = false;  // bandet saktar in och stannar
    float    rum     = 0.0f;    // skick till rummet (reverb-svall)
    float    frys    = 0.0f;    // FRYS: biten precis innan låses och ringer vidare (nivå)
    bool     halvtid = false;   // HALVTID: spelar i halv hastighet
    bool     bakLang = false;   // BAKLÄNGES-BREAK: allt spelas baklänges från breakets start
    float    tapeLangd = 1.0f;  // tape stop: hur många slag inbromsningen tar (VINYLBROMS = en takt)
    bool     glapp   = false;   // BANDGLAPP: tonhöjden dippar som ett slirande band
    bool     byt     = false;   // KANALBYTE: vänster och höger byter plats
};

inline uint16_t grindMonster (uint32_t h, float tathet)
{
    // euklidiskt mönster: k slag jämnt utspridda på 16, roterat
    int k = 5 + (int) (tathet * 7.0f + slump01 (h) * 3.0f);
    if (k > 15) k = 15;
    const int rot = (int) (blanda (h + 3) % 16);
    uint16_t g = 0;
    for (int i = 0; i < 16; ++i)
        if (((i * k) % 16) < k) g |= (uint16_t) (1u << ((i + rot) % 16));
    return g;
}

// I vilken ordning rollerna kommer in i introt (0 = först).
inline int intrADning (int roll, bool atmoForst)
{
    static const int a[] = { 0, 2, 1, 3, 5, 6, 7, 4, 3, 6, 7 };   // KICK BAS HATS PERC SYNT ATMO FX LOOP CLAP RIDE VOX
    static const int b[] = { 3, 4, 2, 5, 1, 0, 6, 2, 5, 6, 3 };
    return (atmoForst ? b : a)[roll];
}

// Åtgärden för en kanal vid en viss tidpunkt.
//  slag    : position i slag (fjärdedelar) från låtens början
//  slagPerTakt : taktartens täljare, oftast 4
inline Atgard atgard (const Plan& p, const Installning& in, const Overstyr& o,
                      int kanal, int roll, double slag, int slagPerTakt = 4)
{
    Atgard a;
    if (roll < 0 || roll >= NUM_ROLL) return a;
    if (slagPerTakt < 1) slagPerTakt = 4;
    if (slag < 0) slag = 0;

    const Mall& m = mallFor (in);
    const int64_t takt = (int64_t) std::floor (slag / slagPerTakt);
    int64_t tid = 0;
    const Sektion s = aktuell (p, o, takt, tid);

    const double iTakt = slag - (double) takt * slagPerTakt;       // 0..slagPerTakt
    const double posIDel = (double) tid * slagPerTakt + iTakt;     // slag in i delen
    const double delSlag = (double) s.langd * slagPerTakt;
    const float  t = (float) (posIDel / delSlag);                  // 0..1 genom delen
    const double slagKvar = delSlag - posIDel;                     // slag kvar av delen
    const int    fras = (int) (tid / 8);                           // 8-taktsfras
    const bool   frasSlut = (tid % 8) == 7 && slagKvar > slagPerTakt;   // sista takten i en fras, inte delens slut
    const double kvarITakt = slagPerTakt - iTakt;

    const bool hard = m.hard > 0.5f;
    a.hard = hard;
    const float filt = std::fmin (1.0f, in.filter * (0.6f + m.filter * 0.6f));
    const float stok = std::fmin (1.0f, in.stok * 0.8f + m.stok * 0.3f);
    const float dub  = std::fmin (1.0f, in.dub * 0.8f + m.dub * 0.4f);
    const uint32_t hk = blanda (s.h, (uint32_t) kanal, (uint32_t) roll);

    // ---------------------------------------------------------------- grund per del
    switch (s.typ)
    {
        case INTRO:
        case OUTRO:
        {
            // rollerna kommer in (eller går ut) en i taget
            const int ord = intrADning (roll, m.atmoForst);
            float in01 = (float) ord / 8.0f;                         // när rollen kommer in, 0..1
            in01 += (slump01 (hk) - 0.5f) * 0.15f * in.variation;
            const float pos = s.typ == INTRO ? t : 1.0f - t;
            const float fade = hard ? (pos >= in01 ? 1.0f : 0.0f)
                                    : std::fmax (0.0f, std::fmin (1.0f, (pos - in01) * 6.0f + 0.15f));
            a.niva = (roll == KICK && s.typ == INTRO && ! m.atmoForst) ? 1.0f : fade;
            if (! hard) a.lp = 0.35f + 0.65f * fade;
            if (s.typ == OUTRO && roll == ATMO) a.niva = 1.0f;
            break;
        }

        case GROOVE:
        case UTANDNING:
        case LYFT:
        {
            static const float bas[] = { 0.97f, 0.90f, 0.85f, 0.70f, 0.60f, 0.55f, 0.40f, 0.75f, 0.80f, 0.45f, 0.25f };
            float pa = bas[roll] + (energiVid (in, takt) - 0.5f) * 0.4f;
            pa = 1.0f - in.variation * (1.0f - pa);
            if (s.typ == LYFT) pa = std::fmax (pa, 0.85f);
            const uint32_t hf = blanda (hk, (uint32_t) fras, 77);
            const float x = slump01 (hf);
            if (x > pa)
            {
                // ut ur mixen den här frasen: tyst eller bortfiltrerad
                if (slump01 (hf + 9) < 0.5f * filt) { a.lp = 0.3f + 0.3f * slump01 (hf + 11); a.niva = 0.8f; }
                else a.niva = 0.0f;
            }
            if (s.typ == UTANDNING)
            {
                if (roll == BAS)  a.lp = std::fmin (a.lp, 1.0f - 0.5f * filt);
                if (roll == SYNT) { a.lp = std::fmin (a.lp, 1.0f - 0.6f * filt); a.niva *= 0.7f; }
                if (roll == FX)   a.niva *= 0.4f;
            }
            if (s.typ == LYFT)
            {
                if (roll == SYNT || roll == FX) a.lp = 0.4f + 0.6f * t;
                if (roll == HATS && in.variation > 0.3f) a.grind = grindMonster (hk, 0.6f);
                if (roll == BAS) a.hp = 0.15f * filt * t;
            }
            break;
        }

        case BREAK:
        {
            const float d = in.brytDjup;
            switch (roll)
            {
                case KICK: if (d > 0.15f) a.niva = 0.0f; else a.hp = 0.6f; break;
                case BAS:  if (d > 0.6f) a.niva = 0.0f; else a.hp = 0.35f + 0.4f * d * filt; break;
                case HATS: if (d > 0.8f) a.niva = 0.0f; else { a.lp = 1.0f - 0.65f * filt; a.niva = 0.6f; } break;
                case PERC: a.lp = 1.0f - 0.5f * filt; a.niva = d > 0.9f ? 0.0f : 0.8f; break;
                case SYNT: a.niva = 1.0f; a.dub = dub * 0.25f; break;
                case ATMO: a.niva = 1.0f; a.dub = dub * 0.2f; break;
                case FX:   a.niva = 0.8f; break;
                case LOOP: a.lp = 1.0f - 0.6f * filt; a.grind = grindMonster (hk, 0.3f); break;
                case CLAP: if (d > 0.5f) a.niva = 0.0f; else { a.lp = 1.0f - 0.4f * filt; a.dub = dub * 0.3f; } break;
                case RIDE: a.niva = 0.0f; break;
                case VOX:  a.niva = 1.0f; a.dub = dub * 0.35f; a.rum = in.overgang * 0.3f; break;
                default: break;
            }
            break;
        }

        case BYGG:
        {
            const float t2 = t * t;
            switch (roll)
            {
                case KICK:
                    a.hp = filt * 0.7f * t2;
                    if (stok > 0.25f && t > 0.75f) a.stutter = t > 0.97f ? 0.5f : (t > 0.94f ? 1.0f : (t > 0.87f ? 2.0f : 4.0f));
                    if (in.brytDjup > 0.85f && t < 0.5f) a.niva = 0.0f;
                    break;
                case BAS:
                    if (in.brytDjup > 0.6f) a.niva = t > 0.5f ? 0.6f : 0.0f;
                    a.hp = filt * 0.85f * t;
                    break;
                case HATS:  a.niva = 0.5f + 0.5f * t; a.lp = 0.35f + 0.65f * t; break;
                case PERC:
                case LOOP:
                    a.lp = 0.4f + 0.6f * t;
                    if (stok > 0.15f && t > 0.5f)
                        a.stutter = t > 0.94f ? 0.5f : (t > 0.75f ? 1.0f : 2.0f);
                    break;
                case SYNT:  a.lp = 0.3f + 0.7f * t; a.niva = 0.6f + 0.4f * t; break;
                case ATMO:  a.niva = 0.5f + 0.5f * t; break;
                case FX:    a.niva = t; break;
                case CLAP:  // virvelrulle: 1/4 -> 1/8 -> 1/16 -> 1/32
                    a.lp = 0.5f + 0.5f * t;
                    if (stok > 0.1f && t > 0.4f)
                        a.stutter = t > 0.94f ? 0.5f : (t > 0.8f ? 1.0f : (t > 0.6f ? 2.0f : 4.0f));
                    break;
                case RIDE:  a.niva = t > 0.5f ? (t - 0.5f) * 2.0f : 0.0f; break;
                case VOX:   a.niva = 0.7f; a.lp = 0.5f + 0.5f * t; break;
                default: break;
            }
            // pitch-stutter: bitarna stiger i tonhöjd sista kvarten
            if (a.stutter > 0.0f && t > 0.75f && roll != KICK)
                a.ton = 1.0f + in.overgang * (t - 0.75f) * 4.0f;
            // rum-svall in i droppet
            if (slagKvar <= 2.0 && roll != KICK && roll != BAS)
                a.rum = in.overgang * (float) (1.0 - slagKvar / 2.0);
            // andas: allt tyst sista slaget (eller två) innan droppet
            const bool andas = slump01 (s.h + 5) < in.andas;
            const double tystSlag = in.andas > 0.7f ? 2.0 : 1.0;
            if (andas && slagKvar <= tystSlag)
            {
                a.stutter = 0.0f;
                if (roll == ATMO || roll == FX) { a.bak = true; a.niva = 1.0f; }
                else a.niva = 0.0f;
            }
            else if (slagKvar <= 1.0 && (roll == ATMO || roll == FX))
                a.bak = true;
            break;
        }

        case DROP:
            if (roll == ATMO) a.niva = 0.7f;
            if (roll == FX && tid >= 1) a.niva = 0.5f;
            if (roll == VOX && tid >= 2) a.niva = 0.0f;
            break;

        case KLIMAX:
            if (roll == FX)   a.grind = grindMonster (blanda (hk, (uint32_t) fras), 0.5f + energiVid (in, takt) * 0.4f);
            if (roll == LOOP && in.variation > 0.5f && (fras % 2) == 1) a.grind = grindMonster (blanda (hk, (uint32_t) fras, 3), 0.8f);
            if (roll == VOX && (fras % 2) == 0) a.niva = 0.0f;
            break;

        default: break;
    }

    // ---------------------------------------------------------------- fills i frasslut
    if (s.typ != BREAK && s.typ != BYGG && s.typ != INTRO && s.typ != OUTRO && frasSlut)
    {
        const uint32_t hf = blanda (s.h, (uint32_t) fras, 0xF111);
        if (slump01 (hf) < stok * 0.7f)
        {
            static const int fillRoll[] = { HATS, PERC, LOOP, SYNT, CLAP };
            const int vem = fillRoll[blanda (hf + 1) % 5];
            if (vem == roll && kvarITakt <= 1.0) a.stutter = slump01 (hf + 2) < 0.5f ? 1.0f : 0.5f;
        }
        if (roll == KICK && slump01 (hf + 4) < stok * 0.35f && kvarITakt <= 0.5) a.niva = 0.0f;

        // dubkast: en roll skickas till ekot och tystnar, ekot svarar
        if (slump01 (hf + 6) < dub * 0.8f)
        {
            static const int dubRoll[] = { SYNT, PERC, LOOP, ATMO, VOX, CLAP };
            const int vem = dubRoll[blanda (hf + 7) % 6];
            if (vem == roll && kvarITakt <= 1.0)
            {
                a.dub = 1.0f;
                if (kvarITakt <= 0.5) a.niva = 0.0f;
            }
        }
    }

    // tape stop: sista slaget innan ett break (eller outro) saktar bandet in, med rum efter
    if ((s.typ == GROOVE || s.typ == KLIMAX || s.typ == DROP || s.typ == LYFT || s.typ == UTANDNING) && slagKvar <= 1.0 && in.overgang > 0.05f)
    {
        int64_t ti = 0;
        const auto nasta = aktuell (p, o, s.start + s.langd, ti);
        if ((nasta.typ == BREAK || nasta.typ == OUTRO) && slump01 (s.h + 31) < in.overgang * (0.35f + m.hard * 0.4f))
        {
            if (roll != ATMO && roll != FX) a.tapeStop = true;
            a.rum = std::fmax (a.rum, in.overgang * 0.6f);
        }
    }

    // ================================================================ mallar med eget beteende
    switch (m.sarskild)
    {
        case S_STAPLA:
            if (s.typ == GROOVE)
            {
                // en kanal i taget kommer in, sedan raseras allt i breaket
                const int ord = intrADning (roll, m.atmoForst);
                const int steg = s.langd / 8 > 0 ? s.langd / 8 : 1;
                a.niva = tid >= (int64_t) ord * steg ? 1.0f : 0.0f;
                a.lp = 1.0f; a.hard = true;
            }
            if (s.typ == BREAK) { a.niva = (roll == ATMO || roll == VOX) ? 1.0f : 0.0f; a.hard = true; }
            break;

        case S_DUELL:
            if (s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX || s.typ == UTANDNING)
            {
                // fråga och svar: grupp A har två takter, grupp B nästa två
                const bool aTur = ((tid / 2) % 2) == 0;
                const bool grA = roll == SYNT || roll == VOX || roll == FX || roll == ATMO;
                const bool grB = roll == PERC || roll == LOOP || roll == CLAP;
                if ((grA && ! aTur) || (grB && aTur)) a.niva = 0.0f;
                if (roll == HATS && aTur) a.niva *= 0.5f;
            }
            break;

        case S_TELEFON:
            if (s.typ == BYGG)    { a.lp = std::fmin (a.lp, 0.45f + 0.55f * t); a.hp = std::fmax (a.hp, 0.4f * (1.0f - t)); }
            else if (s.typ != DROP && s.typ != KLIMAX) { a.lp = std::fmin (a.lp, 0.45f); a.hp = std::fmax (a.hp, 0.4f); }
            break;

        case S_SVARM:
            if (s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX || s.typ == UTANDNING)
            {
                const float pa = roll == KICK ? 0.92f : 0.5f + 0.3f * energiVid (in, takt);
                a.niva = slump01 (blanda (hk, (uint32_t) tid, 0x5A)) < pa ? 1.0f : 0.0f;
                a.hard = true;
            }
            break;

        default: break;
    }

    // ================================================================ TRICK
    auto valjTrick = [&] (uint32_t h, std::initializer_list<uint32_t> lista) -> uint32_t
    {
        uint32_t ok[8]; int n = 0;
        for (auto b : lista) if ((in.trick & b) && n < 8) ok[n++] = b;
        return n > 0 ? ok[blanda (h) % (uint32_t) n] : 0u;
    };
    const double sptD = (double) slagPerTakt;

    // ---- breakeffekter: en per break
    if (s.typ == BREAK && slump01 (s.h + 71) < in.trickMangd)
    {
        const uint32_t ef = valjTrick (s.h + 72, { T_FRYS, T_HALVTID, T_BAKBREAK, T_TOMRUM, T_RADIO });
        const bool tung = roll == KICK || roll == BAS;
        if (ef == T_FRYS && ! tung && posIDel < 2.0 * sptD)
        {
            a.frys = (float) (1.0 - posIDel / (2.0 * sptD));
            a.niva *= 0.4f;
        }
        else if (ef == T_HALVTID && ! tung)
        {
            a.halvtid = true;
            if (roll == LOOP || roll == PERC) a.niva = std::fmax (a.niva, 0.8f);
        }
        else if (ef == T_BAKBREAK && roll != KICK)
        {
            a.bakLang = true;
            a.niva = std::fmax (a.niva, roll == BAS ? 0.5f : 0.7f);
        }
        else if (ef == T_TOMRUM)
        {
            // bara en kanal får vara kvar, ensam i rummet
            int antal = 0, val = -1;
            for (int i = 0; i < 32; ++i) if (in.levande & (1u << i)) ++antal;
            if (antal > 0)
            {
                int k = (int) (blanda (s.h + 73) % (uint32_t) antal);
                for (int i = 0; i < 32; ++i) if (in.levande & (1u << i)) { if (k-- == 0) { val = i; break; } }
                if (kanal == val) { a.niva = 1.0f; a.lp = 1.0f; a.hp = 0.0f; a.rum = std::fmax (a.rum, 0.35f); }
                else { a.niva = 0.0f; a.dub = 0.0f; }
            }
        }
        else if (ef == T_RADIO && posIDel < 2.0 * sptD)
        {
            const uint32_t hr = blanda (hk, (uint32_t) std::floor (posIDel * 2.0), 0xAD10);
            if (slump01 (hr) < 0.5f) { a.lp = 0.35f; a.hp = 0.45f; } else { a.lp = 1.0f; a.hp = 0.0f; }
            if (roll != KICK) a.niva = std::fmax (a.niva, 0.7f);
            a.hard = true;
        }
    }

    // ---- slutet av uppbyggnaden: VINYLBROMS eller KORT EKO
    if (s.typ == BYGG && slump01 (s.h + 81) < in.trickMangd)
    {
        const uint32_t ef = valjTrick (s.h + 82, { T_VINYL, T_EKO });
        if (ef == T_VINYL && slagKvar <= sptD)
        {
            a.tapeStop = true; a.tapeLangd = (float) sptD;
            a.stutter = 0.0f; a.bak = false;
            a.niva = std::fmax (a.niva, 0.8f);
        }
        else if (ef == T_EKO && slagKvar <= 2.0 && roll != KICK && roll != BAS)
        {
            a.stutter = slagKvar > 1.0 ? 1.0f : (slagKvar > 0.5 ? 0.5f : 0.25f);
            a.ton = 1.0f; a.bak = false;
            a.niva = (float) (0.5 + 0.5 * (1.0 - slagKvar / 2.0));
        }
    }

    // ---- grooveknep: högst ett per takt, aldrig i frasens sista takt
    if ((s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX || s.typ == UTANDNING || (s.typ == DROP && tid >= 1))
        && (tid % 8) != 7)
    {
        const uint32_t hb = blanda (s.h, (uint32_t) tid, 0x7A1C);
        if (slump01 (hb) < in.trickMangd * 0.35f)
        {
            const uint32_t ef = valjTrick (hb + 1, { T_STAM, T_GLAPP, T_BYT, T_SPOKE, T_STOPP });
            if (ef == T_STAM)
            {
                const double w = (double) (blanda (hb + 2) % (uint32_t) (slagPerTakt * 2)) * 0.5;
                const bool med = (roll == HATS || roll == PERC || roll == LOOP || roll == SYNT || roll == CLAP || roll == VOX)
                                 && slump01 (blanda (hb + 3, (uint32_t) roll)) < 0.6f;
                if (med && iTakt >= w && iTakt < w + 0.5) a.stutter = 0.5f;
            }
            else if (ef == T_GLAPP && roll != KICK)
            {
                const int b = (int) (blanda (hb + 4) % (uint32_t) slagPerTakt);
                if ((int) std::floor (iTakt) == b) a.glapp = true;
            }
            else if (ef == T_BYT) a.byt = true;
            else if (ef == T_SPOKE)
            {
                static const int vemLista[] = { SYNT, PERC, LOOP, VOX, CLAP, ATMO };
                if (roll == vemLista[blanda (hb + 5) % 6]) { a.niva = 0.0f; a.dub = 1.0f; }
            }
            else if (ef == T_STOPP)
            {
                const double b2 = 1.0 + (double) (blanda (hb + 6) % (uint32_t) (slagPerTakt > 1 ? slagPerTakt - 1 : 1));
                if (iTakt >= b2 && iTakt < b2 + 0.5) { a.niva = 0.0f; a.hard = true; }
            }
        }
    }

    // dubkast i slutet av breaket
    if (s.typ == BREAK && slagKvar <= 1.0 && (roll == SYNT || roll == ATMO) && slump01 (s.h + 21) < dub)
        a.dub = 1.0f;

    return a;
}

// Ungefärlig nivå (0..1) för en kanal i en viss takt. Används av displayen.
inline float visning (const Plan& p, const Installning& in, const Overstyr& o, int kanal, int roll, int64_t takt, int spt = 4)
{
    const auto a = atgard (p, in, o, kanal, roll, (double) takt * spt + spt * 0.5, spt);
    float v = a.niva * (0.25f + 0.75f * a.lp) * (1.0f - 0.6f * a.hp);
    if (a.grind != 0xFFFF) v *= 0.75f;
    return v;
}
} // namespace motor
