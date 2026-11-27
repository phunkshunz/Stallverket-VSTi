// Test av GLITCH:  c++ -std=c++17 -O2 -Isrc tests/glitch_test.cpp -o glitch_test && ./glitch_test
// Kollar var motorn lägger glitcharna: i frasslut, i slutet av bygget och på enstaka steg,
// aldrig i droppets första takt och aldrig när kanalen inte tillåter dem.
#include "Glitch.h"
#include <cstdio>
static int fel = 0;
#define KOLL(x) do { if (!(x)) { std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fel; } } while (0)
using namespace motor;

static bool aktiv (const Atgard& a, const Atgard& r)
{
    return a.stutter != r.stutter || a.bak != r.bak || a.tapeStop != r.tapeStop || a.kross > 0 || a.svep
        || (a.niva == 0 && r.niva > 0) || a.ton != r.ton || a.dubFb != r.dubFb;
}

int main()
{
    Installning in; in.mall = 0; in.fro = 11; in.mjuk = 0.65f; in.trick = 0; in.trickMangd = 0;
    const Plan p = byggPlan (in);
    Overstyr o;
    long iFrasslut = 0, iBygg = 0, iGroove = 0, iDrop1 = 0, totalt = 0, ovrigt = 0;
    long per[8] {};
    for (double sl = 0; sl < 4 * 512; sl += 1.0 / 16)
    {
        for (int roll : { HATS, SYNT, PERC, CLAP })
        {
            const Atgard r = atgard (p, in, o, roll, roll, sl);
            Atgard a = r;
            glitch::glitcha (a, p, in, o, roll, roll, sl, 4, 0xFF, 0.8f);
            if (! aktiv (a, r)) continue;
            ++totalt;
            int64_t tid; const auto s = aktuell (p, o, (int64_t) (sl / 4), tid);
            const double iT = sl - std::floor (sl / 4) * 4;
            if (s.typ == DROP && tid == 0) ++iDrop1;
            else if (s.typ == BYGG) ++iBygg;
            else if (tid % 8 == 7) ++iFrasslut;
            else if (tid % 8 == 3) ++iGroove;
            else if (s.typ == BREAK && tid == s.langd - 1 && iT >= 3.0) {}
            else ++ovrigt;
            if (a.stutter != r.stutter && a.ton == r.ton) ++per[0];
            if (a.bak) ++per[1];
            if (a.tapeStop) ++per[2];
            if (a.kross > 0) ++per[4];
            if (a.svep) ++per[7];
            if (a.dubFb > 0.7f) ++per[6];
        }
    }
    std::printf ("glitch-steg (1/64): totalt %ld, frasslut %ld, bygg %ld, groove-steg %ld, droppets första takt %ld, annat %ld\n",
                 totalt, iFrasslut, iBygg, iGroove, iDrop1, ovrigt);
    std::printf ("  repeat %ld  bak %ld  tape %ld  kross %ld  eko %ld  svep %ld\n", per[0], per[1], per[2], per[4], per[6], per[7]);
    KOLL (totalt > 200);
    KOLL (iDrop1 == 0);
    KOLL (ovrigt == 0);
    KOLL (iFrasslut > 0 && iBygg > 0);

    // ingen effekt tillåten = inget händer
    for (double sl = 0; sl < 4 * 256; sl += 0.25)
    {
        const Atgard r = atgard (p, in, o, 1, HATS, sl);
        Atgard a = r;
        glitch::glitcha (a, p, in, o, 1, HATS, sl, 4, 0u, 1.0f);
        KOLL (! aktiv (a, r));
        a = r;
        glitch::glitcha (a, p, in, o, 1, HATS, sl, 4, 0xFF, 0.0f);
        KOLL (! aktiv (a, r));
    }
    // kicken får aldrig kross eller tonhopp
    for (double sl = 0; sl < 4 * 512; sl += 0.25)
    {
        const Atgard r = atgard (p, in, o, 0, KICK, sl);
        Atgard a = r;
        glitch::glitcha (a, p, in, o, 0, KICK, sl, 4, 0xFF, 1.0f);
        KOLL (a.kross == 0.0f && a.ton == r.ton);
    }
    std::printf (fel ? "\n%d FEL\n" : "\nGlitch OK\n", fel);
    return fel ? 1 : 0;
}
