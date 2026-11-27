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

#include <algorithm>
#include <array>
#include <initializer_list>
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
enum Sarskild { S_INGEN, S_SPEGEL, S_STAPLA, S_FALSKT, S_DUELL, S_TELEFON, S_SVARM, S_SKRIFT, S_PLASTIK, S_HOOD };
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
    { "PLASTIK",     { 64,64,32,16,32,32,64,32,32 }, 0.40f,0.30f,0.20f,0.20f,0.45f,0.30f, 0.3f,0.2f,0.4f,0.9f, true,  S_PLASTIK },
    { "HOOD",        { 32,64,32, 8, 8,64,64,32,32 }, 0.20f,0.10f,0.05f,0.60f,0.30f,0.25f, 0.6f,0.1f,0.1f,0.6f, false, S_HOOD },
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
    float    mjuk      = 0.0f;  // MJUKHET: 0 = gamla tvära byten, 1 = långa, glidande övergångar (se mjukgor nedan)
    float    tryck     = 0.0f;  // TRYCK: hur hårt maskinen trycker (mättnad och punch), motorn höjer i droppet

    // EGEN mall (mall == kAntalMallar) från mallredigeraren
    Mall     egen = kMallar[0];

    // energikurva över hela setet: åtta punkter utspridda över kurvTakter
    bool     kurvaPa   = false;
    int      kurvTakter = 512;
    float    kurva[8]  { 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 1.0f, 0.7f, 0.4f };

    // TRICK: vilka effekter som är på (bitar, se Trick) och hur ofta de händer
    uint32_t trick      = 0x1FFFFu;
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
            && andas == o.andas && dub == o.dub && overgang == o.overgang && mjuk == o.mjuk && tryck == o.tryck
            && extra == o.extra;
    }
    bool operator!= (const Installning& o) const { return ! (*this == o); }
};

enum Trick : uint32_t
{
    T_FRYS = 1u << 0, T_HALVTID = 1u << 1, T_BAKBREAK = 1u << 2, T_VINYL = 1u << 3, T_RADIO = 1u << 4, T_EKO = 1u << 5,
    T_TOMRUM = 1u << 6, T_STAM = 1u << 7, T_GLAPP = 1u << 8, T_BYT = 1u << 9, T_SPOKE = 1u << 10, T_STOPP = 1u << 11,
    T_FAS = 1u << 12, T_DUBKEDJA = 1u << 13, T_BAGE = 1u << 14, T_BASSVEP = 1u << 15, T_KICKPAUS = 1u << 16
};
inline constexpr int kAntalTrick = 17;

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
        // MJUKHET: uppbyggnader och breaks får aldrig bli så korta att de låter som ett misstag
        if (in.mjuk > 0.5f && ! p.form && (typ == BYGG || typ == BREAK || typ == LYFT)) return tvapotens (l, 8, 64);
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
    int      fas     = 0;       // FASFÖRSKJUTNING: kanalen ligger så här många 1/16-delar efter
    float    dubFb   = 0.0f;    // DUBKEDJA: ekots återkoppling (0 = vanlig)
    bool     ekoKlipp = false;  // klipper ekot tvärt (droppet efter en dubkedja)
    float    tryck   = 0.0f;    // TRYCK: mättnad och täthet, 0..1 (motorn höjer i droppet, sänker i breaket)
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

// Grupper för MJUKHET: bara en grupp får byta i varje 8-taktsfras, så att det
// aldrig händer fyra saker samtidigt. Kick/bas, hats/ride, clap/perc, resten.
inline int rollGrupp (int roll)
{
    static const int g[] = { 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 3 };   // KICK BAS HATS PERC SYNT ATMO FX LOOP CLAP RIDE VOX
    return (roll >= 0 && roll < NUM_ROLL) ? g[roll] : 3;
}
inline bool melodisk (int roll) { return roll == SYNT || roll == ATMO || roll == BAS || roll == LOOP || roll == VOX || roll == FX; }

// Åtgärden för en kanal vid en viss tidpunkt, rå (utan MJUKHET).
//  slag    : position i slag (fjärdedelar) från låtens början
//  slagPerTakt : taktartens täljare, oftast 4
inline Atgard atgardRa (const Plan& p, const Installning& in, const Overstyr& o,
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
            // MJUKHET: rollerna byter i turordning, en grupp per 8-taktsfras, i stället för alla på en gång
            int block = fras;
            if (in.mjuk > 0.3f)
            {
                const int K = in.variation > 0.75f ? 2 : 4;
                block = (fras + rollGrupp (roll) % K) / K;
            }
            const uint32_t hf = blanda (hk, (uint32_t) block, 77);
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
                case LOOP: a.lp = 1.0f - 0.6f * filt; a.niva = 0.7f; break;
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
                auto paFor = [&] (bool tur) { return (grA && ! tur) || (grB && tur) ? 0.0f : 1.0f; };
                float v = paFor (aTur);
                if (in.mjuk > 0.001f)
                {
                    // MJUKHET: svaret tonar in under sista slaget (eller två) i stället för att klippas in
                    const double block = 2.0 * slagPerTakt, posB = std::fmod (posIDel, block);
                    const double tona = std::min (block * 0.5, 0.5 + 1.5 * in.mjuk);
                    const float w = (float) std::clamp ((posB - (block - tona)) / tona, 0.0, 1.0);
                    v += (paFor (! aTur) - v) * w * w * (3.0f - 2.0f * w);
                    if (v < 1.0f && melodisk (roll)) a.lp = std::fmin (a.lp, 0.35f + 0.65f * v);
                }
                a.niva *= v;
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
                if (in.mjuk <= 0.001f)
                {
                    a.niva = slump01 (blanda (hk, (uint32_t) tid, 0x5A)) < pa ? 1.0f : 0.0f;
                    a.hard = true;
                }
                else
                {
                    // MJUKHET: svärmen byter varannan takt (mjukt läge) och tonar mellan lägena
                    const int steg = in.mjuk > 0.5f ? 2 : 1;
                    const int64_t b = tid / steg;
                    auto paFor = [&] (int64_t bb) { return slump01 (blanda (hk, (uint32_t) bb, 0x5A)) < pa ? 1.0f : 0.0f; };
                    const double block = (double) steg * slagPerTakt, posB = std::fmod (posIDel, block);
                    const double tona = std::min (block * 0.5, 0.5 + 1.5 * in.mjuk);
                    const float w = (float) std::clamp ((posB - (block - tona)) / tona, 0.0, 1.0);
                    const float v0 = paFor (b);
                    const float v = (roll == KICK) ? v0 : v0 + (paFor (b + 1) - v0) * w * w * (3.0f - 2.0f * w);
                    a.niva = v;
                    if (melodisk (roll)) a.lp = std::fmin (a.lp, 0.35f + 0.65f * v);
                }
            }
            break;

        case S_PLASTIK:
        {
            // en kanal står fast hela låten, kicken hålls borta länge och smäller in efter första breaket/bygget
            const bool fast = roll == PERC;
            if (fast && s.typ != OUTRO) { a.niva = 1.0f; a.stutter = s.typ == BYGG ? a.stutter : 0.0f; }
            if (roll == KICK && s.index <= 1 && (s.typ == INTRO || s.typ == GROOVE || s.typ == LYFT)) a.niva = 0.0f;
            if ((roll == CLAP || roll == RIDE) && (s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX))
                a.niva = slump01 (blanda (hk, (uint32_t) fras, 0x91A)) < 0.55f ? 1.0f : 0.0f;
            break;
        }

        case S_HOOD:
            if (s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX || s.typ == DROP)
            {
                // ett nytt element ungefär varannan minut (var 64:e takt), inget hoppar ur
                int64_t lat = takt; if (p.takter > 0) lat %= p.takter;
                const int ord = intrADning (roll, m.atmoForst);
                a.niva = lat >= (int64_t) ord * 32 ? 1.0f : 0.0f;
                if (roll == KICK) a.niva = 1.0f;
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
    // bara i fjärde takten av en fras (inte fras-slutet där fillsen ligger), så det låter avsiktligt
    if ((s.typ == GROOVE || s.typ == LYFT || s.typ == KLIMAX || s.typ == UTANDNING || (s.typ == DROP && tid >= 1))
        && (tid % 8) == 3 && slagKvar > sptD)
    {
        const uint32_t hb = blanda (s.h, (uint32_t) tid, 0x7A1C);
        if (slump01 (hb) < in.trickMangd * 0.8f)
        {
            const uint32_t ef = valjTrick (hb + 1, { T_STAM, T_GLAPP, T_BYT, T_SPOKE, T_STOPP });
            if (ef == T_STAM)
            {
                const double w = sptD - 0.5;   // sista åttondelen i takten
                const bool med = (roll == HATS || roll == PERC || roll == LOOP || roll == SYNT || roll == CLAP || roll == VOX)
                                 && slump01 (blanda (hb + 3, (uint32_t) roll)) < 0.6f;
                if (med && iTakt >= w && iTakt < w + 0.5) a.stutter = 0.5f;
            }
            else if (ef == T_GLAPP && roll != KICK)
            {
                if (iTakt >= sptD - 1.0) a.glapp = true;   // sista slaget
            }
            else if (ef == T_BYT) a.byt = true;
            else if (ef == T_SPOKE)
            {
                static const int vemLista[] = { SYNT, PERC, LOOP, VOX, CLAP, ATMO };
                // bara takens två sista slag: hela takten utan kanalen lät som ett fel
                if (roll == vemLista[blanda (hb + 5) % 6] && iTakt >= sptD - 2.0) { a.niva = 0.0f; a.dub = 1.0f; }
            }
            else if (ef == T_STOPP)
            {
                if (iTakt >= sptD - 0.5) { a.niva = 0.0f; a.hard = true; }   // andra halvan av sista slaget
            }
        }
    }

    // ---- DUBKEDJA: i breaket matar ekot sig självt allt mer, och klipps tvärt i droppet
    if ((in.trick & T_DUBKEDJA) && slump01 (s.h + 91) < in.trickMangd)
    {
        if (s.typ == BREAK && roll != KICK && roll != BAS)
        {
            a.dubFb = 0.55f + 0.37f * t;
            a.dub = std::fmax (a.dub, (roll == SYNT || roll == VOX || roll == PERC || roll == CLAP || roll == LOOP) ? 0.45f : 0.2f);
        }
    }
    if ((s.typ == DROP || s.typ == KLIMAX) && tid == 0 && iTakt < 0.25 && (in.trick & T_DUBKEDJA))
    {
        int64_t ti = 0;
        const auto forra = aktuell (p, o, s.start - 1, ti);
        if (forra.typ == BREAK || forra.typ == BYGG) a.ekoKlipp = true;
    }

    // ---- FILTERBÅGE: en kanal per del får en långsam båge: ner, lite upp, ner igen före slutet
    if ((in.trick & T_BAGE) && (s.typ == GROOVE || s.typ == LYFT || s.typ == UTANDNING || s.typ == KLIMAX)
        && slump01 (s.h + 101) < in.trickMangd * 1.4f)
    {
        static const int bageRoll[] = { SYNT, LOOP, PERC, HATS, ATMO, BAS };
        if (roll == bageRoll[blanda (s.h + 102) % 6])
        {
            const float bage = 0.42f + 0.38f * std::sin (3.14159265f * t);   // öppnast i mitten av delen
            a.lp = std::fmin (a.lp, bage);
        }
    }

    // ---- FASFÖRSKJUTNING: en kanal glider en 1/16 var åttonde takt (Reich), tillbaka efter fyra steg
    if ((in.trick & T_FAS) && (s.typ == GROOVE || s.typ == LYFT || s.typ == UTANDNING || s.typ == KLIMAX)
        && slump01 (s.h + 111) < in.trickMangd * 1.4f)
    {
        static const int fasRoll[] = { SYNT, PERC, LOOP, CLAP };
        if (roll == fasRoll[blanda (s.h + 112) % 4]) a.fas = (int) (fras % 4);
    }

    // ---- BASSVEP: basen börjar stängd och öppnar sig under droppets första takt
    if ((in.trick & T_BASSVEP) && roll == BAS && (s.typ == DROP || s.typ == KLIMAX) && tid == 0
        && slump01 (s.h + 121) < in.trickMangd * 1.6f)
        a.lp = std::fmin (a.lp, (float) (0.25 + 0.75 * (iTakt / sptD)));

    // ---- KICKPAUS: kicken andas ut de två sista slagen i varje 16-taktsfras
    if ((in.trick & T_KICKPAUS) && roll == KICK && (s.typ == GROOVE || s.typ == KLIMAX || s.typ == DROP)
        && (tid % 16) == 15 && slagKvar > sptD && kvarITakt <= 2.0 && slump01 (blanda (s.h, (uint32_t) (tid / 16), 0xC1)) < in.trickMangd * 1.4f)
    {
        a.niva = 0.0f; a.hard = true;
    }

    // dubkast i slutet av breaket
    if (s.typ == BREAK && slagKvar <= 1.0 && (roll == SYNT || roll == ATMO) && slump01 (s.h + 21) < dub)
        a.dub = 1.0f;

    // ---- TRYCK: hårdast i droppet och klimax, växer genom bygget, lättare i breaket
    {
        float d = 0.8f;
        if (s.typ == DROP || s.typ == KLIMAX) d = 1.0f;
        else if (s.typ == BYGG) d = 0.55f + 0.45f * t;
        else if (s.typ == BREAK) d = 0.45f;
        else if (s.typ == INTRO || s.typ == OUTRO) d = 0.65f;
        a.tryck = in.tryck * d;
    }

    return a;
}

//==============================================================================
// MJUKHET: glider in i nästa fras i stället för att byta tvärt.
//
// Motorn tittar fram till nästa 8-taktsgräns (eller delens slut). Är kanalen på
// väg ut eller in där, eller ska filtret någon annanstans, börjar förändringen
// redan några slag (upp till två takter) innan, på det sätt en DJ eller producent
// hade gjort det:
//  - kick och bas på väg ut: högpass som lyfter ur basen, sedan ut på ettan
//  - synt, atmo, loop på väg ut: lågpass som stänger och tonar ner
//  - trummor på väg ut: tonar ner med lite stängt filter
//  - på väg in: en filtrerad försmak som växer, och fullt på ettan (kicken kommer
//    rakt in på ettan, det är så det ska låta)
// In i DROP och KLIMAX glider ingenting: där ska det smälla.
inline Atgard atgard (const Plan& p, const Installning& in, const Overstyr& o,
                      int kanal, int roll, double slag, int slagPerTakt = 4)
{
    Atgard a = atgardRa (p, in, o, kanal, roll, slag, slagPerTakt);
    if (in.mjuk <= 0.001f || roll < 0 || roll >= NUM_ROLL) return a;
    if (slagPerTakt < 1) slagPerTakt = 4;
    if (slag < 0) slag = 0;

    const Mall& m = mallFor (in);
    const int64_t takt = (int64_t) std::floor (slag / slagPerTakt);
    int64_t tid = 0;
    const Sektion s = aktuell (p, o, takt, tid);
    if (s.typ == BYGG || s.typ == INTRO || s.typ == OUTRO) return a;   // de glider redan på egen hand

    const int64_t frasSlutITid = std::min ((int64_t) s.langd, (tid / 8 + 1) * 8);
    const int64_t grans = takt + (frasSlutITid - tid);
    const bool delByte = frasSlutITid == (int64_t) s.langd;
    const double slagTill = (double) grans * slagPerTakt - slag;

    int64_t ti = 0;
    const Sektion n = aktuell (p, o, grans, ti);
    if (delByte && (n.typ == DROP || n.typ == KLIMAX)) return a;

    double W = delByte ? (n.typ == BREAK || n.typ == OUTRO ? 4.0 + 12.0 * in.mjuk : 2.0 + 6.0 * in.mjuk)
                       : 1.0 + 7.0 * in.mjuk;
    W *= 1.0 - 0.45 * m.hard;                                    // hårda mallar glider kortare
    if (! melodisk (roll)) W *= 0.6;                              // trummor kortare än synt och atmo
    W = std::min (W, (double) frasSlutITid * slagPerTakt * 0.5);  // aldrig mer än halva frasen
    if (W < 0.25 || slagTill > W) return a;

    const Atgard mal = atgardRa (p, in, o, kanal, roll, (double) grans * slagPerTakt + 0.02, slagPerTakt);
    const float x  = (float) (1.0 - slagTill / W);               // 0 -> 1 fram till gränsen
    const float sx = x * x * (3.0f - 2.0f * x);
    auto lerp = [] (float a0, float b0, float t0) { return a0 + (b0 - a0) * t0; };

    if (mal.niva < a.niva - 0.05f)                                // på väg ut
    {
        if (roll == KICK || roll == BAS)
            a.hp = std::fmax (a.hp, lerp (a.hp, std::fmax (mal.hp, 0.38f), sx));
        else if (melodisk (roll))
        {
            a.lp = std::fmin (a.lp, lerp (a.lp, std::fmin (mal.lp, 0.28f), sx));
            a.niva = lerp (a.niva, mal.niva, sx * sx);
        }
        else
        {
            a.niva = lerp (a.niva, mal.niva, sx);
            a.lp = std::fmin (a.lp, lerp (1.0f, 0.55f, sx));
        }
        a.hard = false;
    }
    else if (mal.niva > a.niva + 0.05f)                           // på väg in
    {
        if (roll != KICK)
        {
            const bool mel = melodisk (roll);
            const float forsmak = mal.niva * (mel ? 0.45f : 0.35f) * (mel ? sx : sx * sx);
            if (forsmak > a.niva)
            {
                a.niva = forsmak;
                a.lp = lerp (mel ? 0.22f : 0.25f, mal.lp * (mel ? 0.7f : 0.7f), sx);
                a.hp = mel ? a.hp : std::fmax (mal.hp, 0.25f * (1.0f - sx));
                a.grind = mal.grind;
                a.hard = false;
            }
        }
    }
    else                                                          // samma nivå: filtret glider dit det ska
    {
        if (std::abs (mal.lp - a.lp) > 0.02f) a.lp = lerp (a.lp, mal.lp, sx);
        if (std::abs (mal.hp - a.hp) > 0.02f) a.hp = lerp (a.hp, mal.hp, sx);
    }
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
