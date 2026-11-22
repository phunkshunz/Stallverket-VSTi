// Fristående test av STÄLLVERKETs motor:  c++ -std=c++17 -Isrc tests/motor_test.cpp && ./a.out
#include "Motor.h"
#include <cstdio>
#include <cstdlib>
using namespace motor;
static int fel = 0;
#define KOLL(x) do { if (!(x)) { if (fel < 20) std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fel; } } while (0)

int main()
{
    for (int mall = 0; mall < kAntalMallar; ++mall)
        for (int form = 0; form < kAntalFormer; ++form)
            for (uint32_t fro = 1; fro < 40; ++fro)
            {
                Installning in; in.mall = mall; in.form = form; in.fro = fro;
                in.energi = (float) (fro % 5) / 4.0f; in.variation = (float) (fro % 3) / 2.0f; in.byggLangd = (float) (fro % 7) / 6.0f;
                const Plan p = byggPlan (in);
                KOLL (p.n > 1);
                KOLL (p.s[0].typ == INTRO);
                int64_t sum = 0;
                for (int i = 0; i < p.n; ++i)
                {
                    KOLL (p.s[(size_t) i].start == sum);
                    KOLL (p.s[(size_t) i].langd >= 4);
                    sum += p.s[(size_t) i].langd;
                    if (p.s[(size_t) i].typ == BYGG)
                        KOLL (i + 1 < p.n && (p.s[(size_t) i + 1].typ == DROP || p.s[(size_t) i + 1].typ == KLIMAX));
                }
                KOLL (sum == p.takter);
                if (form > 0)
                {
                    KOLL (p.s[(size_t) p.n - 1].typ == OUTRO);
                    KOLL (p.takter <= kFormTakter[form] + 16);
                    KOLL (p.takter >= kFormTakter[form] - 8);
                }
                else KOLL (p.takter > 2000);
                const Plan q = byggPlan (in);
                KOLL (q.n == p.n && q.takter == p.takter);
                Overstyr o;
                for (double sl = 0; sl < 4 * 300; sl += 0.25)
                    for (int r = 0; r < NUM_ROLL; ++r)
                    {
                        const auto a = atgard (p, in, o, r, r, sl);
                        KOLL (a.niva >= 0 && a.niva <= 1.0001f && a.lp >= 0 && a.lp <= 1.0001f && a.hp >= 0 && a.hp <= 1.0001f);
                        KOLL (a.stutter == 0 || a.stutter == 0.5f || a.stutter == 1 || a.stutter == 2 || a.stutter == 4);
                    }
            }

    for (int mall = 0; mall < kAntalMallar; ++mall)
    {
        Installning in; in.mall = mall; in.fro = 7; in.form = 2;
        const Plan p = byggPlan (in);
        std::printf ("%-11s %3lld takter:", kMallar[mall].namn, (long long) p.takter);
        for (int i = 0; i < p.n; ++i) std::printf (" %s%d", delNamn (p.s[(size_t) i].typ), p.s[(size_t) i].langd);
        std::printf ("\n");
    }

    Installning in; const Plan p = byggPlan (in);
    Overstyr o; o.typ = BREAK; o.start = 40; o.langd = 8;
    int64_t tid = 0;
    KOLL (aktuell (p, o, 41, tid).typ == BREAK && tid == 1);
    KOLL (atgard (p, in, o, 0, KICK, 41 * 4 + 1).niva == 0.0f);
    Overstyr h; h.hall = true; h.hallStart = 16; h.hallLangd = 8; h.hallTyp = KLIMAX;
    KOLL (aktuell (p, h, 100, tid).typ == KLIMAX && tid == (100 - 16) % 8);

    // EGEN mall + energikurva
    {
        Installning e; e.mall = kAntalMallar; e.egen = kMallar[2]; e.egen.langd[GROOVE] = 8; e.kurvaPa = true; e.kurvTakter = 256;
        e.extra = 1;
        const Plan pe = byggPlan (e);
        KOLL (pe.n > 10);
        KOLL (energiVid (e, 0) == e.kurva[0]);
        KOLL (std::abs (energiVid (e, 255) - e.kurva[7]) < 0.05f);
        KOLL (e != in);
    }

    std::printf (fel ? "\n%d FEL\n" : "\nMotorn OK\n", fel);
    return fel ? 1 : 0;
}
