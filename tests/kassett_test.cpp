// Test av BETONG-KASSETTEN:  c++ -std=c++17 -O2 -Isrc tests/kassett_test.cpp -o kassett_test && ./kassett_test
#include "Kassett.h"
#include <cstdio>
static int fel = 0;
#define KOLL(x) do { if (!(x)) { std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fel; } } while (0)

int main()
{
    using namespace kassett;
    std::printf ("%d kit, %d mönster\n", kNumKits, kNumPatterns);
    KOLL (kNumKits >= 40 && kNumPatterns >= 150);

    // alla mönster går att läsa och har minst ett spår med slag
    for (int i = 0; i < kNumPatterns; ++i)
    {
        const Monster m = lasMonster (i);
        int slag = 0;
        for (auto& s : m.spar) if (s.anvands) for (int k = 0; k < s.langd; ++k) slag += s.steg[(size_t) k].pa;
        if (slag == 0) std::printf ("tomt mönster: %s\n", kPatterns[i].name);
        KOLL (slag > 0);
        KOLL (grannMonster (i, 1) != i);
    }

    // "Grund 130": fyra takter, alla nivåer på = 16 kickar och 8 clap
    const double sr = 48000.0, bpm = 130.0, sps = bpm / 60.0 / sr;
    auto kor = [&] (Kassett& k, double fran, double till, auto niva, auto lek)
    {
        std::vector<float> L (512), R (512);
        for (double sl = fran; sl + 512 * sps <= till; sl += 512 * sps)
        {
            std::fill (L.begin(), L.end(), 0.0f); std::fill (R.begin(), R.end(), 0.0f);
            k.kor (L.data(), R.data(), 512, sl, sps, true, 0.8f, niva, lek);
            for (float v : L) KOLL (std::isfinite (v) && std::abs (v) < 8.0f);
        }
    };
    {
        Kassett k; k.forbered (sr); k.sattMonster (0);
        kor (k, 0.0, 16.0, [] (int, double, float&) { return 1.0f; }, [] (double) { return Lek {}; });
        std::printf ("Grund 130, 4 takter: kick %u, clap %u, öppen hat %u\n", k.slagRaknare[0], k.slagRaknare[4], k.slagRaknare[3]);
        KOLL (k.slagRaknare[0] >= 15);
        KOLL (k.slagRaknare[4] == 8 || k.slagRaknare[4] == 7);
        KOLL (k.slagRaknare[3] >= 15);
    }
    // motorn tar bort kicken: inga kickslag
    {
        Kassett k; k.forbered (sr); k.sattMonster (0);
        kor (k, 0.0, 16.0, [] (int roll, double, float&) { return roll == motor::KICK ? 0.0f : 1.0f; }, [] (double) { return Lek {}; });
        KOLL (k.slagRaknare[0] == 0 && k.slagRaknare[4] >= 7);
    }
    // leken: kick 2 i stället för kick 1, och fills ger fler hatslag
    {
        Kassett k; k.forbered (sr); k.sattMonster (0);
        kor (k, 0.0, 16.0, [] (int, double, float&) { return 1.0f; }, [] (double) { Lek l; l.kick2 = true; l.fill = true; return l; });
        std::printf ("med kick 2 + fill: kick1 %u kick2 %u hats %u\n", k.slagRaknare[0], k.slagRaknare[1], k.slagRaknare[2]);
        KOLL (k.slagRaknare[0] == 0 && k.slagRaknare[1] >= 15);
    }
    // med motorn: en hel plan, kicken försvinner i breaken
    {
        motor::Installning in; in.mall = 0; in.fro = 5; in.mjuk = 0.65f; in.brytDjup = 0.8f;
        const motor::Plan p = motor::byggPlan (in);
        motor::Overstyr o;
        Kassett k; k.forbered (sr); k.sattMonster (3);
        int kickIBreak = 0;
        auto niva = [&] (int roll, double sl, float& lp)
        {
            const auto a = motor::atgard (p, in, o, 7 * 3 + 61 + roll, roll, sl);
            lp = a.lp;
            int64_t ti; const auto s = motor::aktuell (p, o, (int64_t) (sl / 4), ti);
            if (s.typ == motor::BREAK && roll == motor::KICK && a.niva > 0.01f) ++kickIBreak;
            return a.niva;
        };
        auto lek = [&] (double sl) { return lekFor (p, in, o, 3, sl, 4, 0.7f); };
        kor (k, 0.0, 4.0 * 256, niva, lek);
        std::printf ("256 takter med motorn: kick %u kick2 %u hats %u clap %u, kickslag i break: %d\n",
                     k.slagRaknare[0], k.slagRaknare[1], k.slagRaknare[2], k.slagRaknare[4], kickIBreak);
        KOLL (kickIBreak == 0);
        KOLL (k.slagRaknare[0] > 200);
    }
    std::printf (fel ? "\n%d FEL\n" : "\nKassetten OK\n", fel);
    return fel ? 1 : 0;
}
