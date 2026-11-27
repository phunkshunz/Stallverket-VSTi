// Test av REFERENSLÄSAREN:  c++ -std=c++17 -O2 -Isrc tests/referens_test.cpp -o referens_test && ./referens_test
// Syntar en låt med känd form (intro, groove, break, bygg, drop, outro) och kollar att skriften blir rätt.
#include "Referens.h"
#include <cstdio>
#include <random>
static int fel = 0;
#define KOLL(x) do { if (!(x)) { std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fel; } } while (0)

int main()
{
    const double sr = 8000.0, bpm = 128.0;
    // del: 0 intro (hats), 1 groove (kick+bas+hats), 2 break (pad), 3 bygg (pad + stigande hats), 4 drop (allt, starkare), 5 outro (hats)
    const int form[][2] = { { 0, 16 }, { 1, 32 }, { 2, 16 }, { 3, 8 }, { 4, 32 }, { 1, 16 }, { 5, 16 } };
    std::vector<float> ljud;
    std::mt19937 rng (7); std::uniform_real_distribution<float> br (-1, 1);
    const double sps = bpm / 60.0 / sr;
    double slag = 0;
    for (auto& d : form)
    {
        const int n = (int) (d[1] * 4 / sps);
        for (int i = 0; i < n; ++i, slag += sps)
        {
            const double t = (slag - std::floor (slag)) * 60.0 / bpm;
            const double s8 = std::fmod (slag * 2, 1.0) * 30.0 / bpm;
            const float kick = (float) (std::sin (6.2832 * 50 * t) * std::exp (-t * 8));
            const float bas = (float) (std::sin (6.2832 * 55 * std::fmod (slag + 0.5, 1.0) * 60 / bpm) * 0.5);
            const float hat = br (rng) * (float) std::exp (-s8 * 60) * 0.15f;
            const float pad = (float) (std::sin (6.2832 * 440 * slag * 60 / bpm) * 0.12);
            const float upp = (float) i / n;
            float x = 0;
            switch (d[0])
            {
                case 0: x = hat; break;
                case 1: x = kick * 0.8f + bas + hat; break;
                case 2: x = pad; break;
                case 3: x = pad + hat * 2.0f * upp; break;
                case 4: x = kick + bas * 1.2f + hat * 1.5f + pad; break;
                default: x = hat * 0.8f; break;
            }
            ljud.push_back (x);
        }
    }
    const auto r = referens::analysera (ljud, sr, bpm);
    std::printf ("takter %d  kurva-längd %d\nskrift: %s\nkurva:", r.takter, r.kurvTakter, r.skrift.c_str());
    for (float k : r.kurva) std::printf (" %.2f", k);
    std::printf ("\n");
    KOLL (r.ok);
    KOLL (r.takter == 136);
    KOLL (r.kurvTakter == 128);
    KOLL (r.skrift.rfind ("I16", 0) == 0);            // börjar med 16 takters intro
    KOLL (r.skrift.find ("B") != std::string::npos);  // ett break
    KOLL (r.skrift.find ("D") != std::string::npos || r.skrift.find ("K") != std::string::npos);   // ett drop
    KOLL (r.skrift.find ("O16") != std::string::npos);   // slutar med outro
    KOLL (! referens::analysera ({}, sr, bpm).ok);
    std::printf (fel ? "\n%d FEL\n" : "\nReferensläsaren OK\n", fel);
    return fel ? 1 : 0;
}
