// Test av LOOPDELAREN:  c++ -std=c++17 -O2 -Isrc tests/delare_test.cpp -o delare_test && ./delare_test
// Syntar en technoloop (kick, clap på 2 och 4, sluten hihat, öppen hihat på offbeat, basgång)
// och kollar att:
//  1. allt på = exakt originalet (bara fördröjt med framåtblicken)
//  2. varje del som tas bort försvinner, och resten står kvar
#include "Fack.h"
#include "Delare.h"
#include <cstdio>
#include <cstdlib>
#include <random>

static int fel = 0;
#define KOLL(x) do { if (!(x)) { std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fel; } } while (0)

constexpr float SR = 48000.0f;
constexpr double BPM = 130.0;

// Filter för testljuden: clap = bandpassat brus runt 1,5 kHz, hihats = högpassat brus
struct Bp { float ic1 = 0, ic2 = 0; float kor (float x, float fc, float q) { const float g = std::tan (3.14159f * fc / 48000.0f), k = 1.0f / q;
    const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2; const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return fc < 3000 ? v1 * k : x - k * v1 - v2; } };
static Bp bpClap, hpHat[2];

// En del av loopen åt gången, så vi vet facit
static float del (int vilken, double slag, std::mt19937& rng)
{
    std::uniform_real_distribution<float> brus (-1.0f, 1.0f);
    const double sek = 60.0 / BPM;
    const double fasSlag = slag - std::floor (slag);
    const double t = fasSlag * sek;
    const int slagNr = (int) std::floor (slag) % 4;
    const double s16 = std::fmod (slag * 4.0, 1.0) * sek / 4.0;
    const int steg16 = (int) std::floor (slag * 4.0) % 4;
    switch (vilken)
    {
        case 0: // kick
            return (float) (std::sin (6.2832 * (45.0 * t + 90.0 * (1.0 - std::exp (-t * 30.0)) / 30.0)) * std::exp (-t * 9.0) * 0.8);
        case 1: // clap på 2 och 4
            {
                // tre snabba smällar och en svans, som en riktig clap
                const double env = t < 0.03 ? std::exp (-std::fmod (t, 0.01) * 300.0) : std::exp (-(t - 0.03) * 25.0);
                const float x = bpClap.kor (brus (rng), 1500.0f, 1.2f);
                return slagNr % 2 == 1 ? (float) (x * env * 0.9) : 0.0f;
            }
        case 2: // sluten hihat på 16-delar utom offbeat
            { const float h = hpHat[0].kor (brus (rng), 7000.0f, 0.7f); return steg16 != 2 ? (float) (h * std::exp (-s16 * 160.0) * 0.3) : 0.0f; }
        case 3: // öppen hihat på offbeat
            { const float h = hpHat[1].kor (brus (rng), 7000.0f, 0.7f); return steg16 == 2 ? (float) (h * std::exp (-s16 * 14.0) * 0.25) : 0.0f; }
        default: // basgång på 16-delar, offbeat-betonad
        {
            static const int not_[16] = { 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0 };
            const int st = (int) std::floor (slag * 4.0) % 16;
            return not_[st] ? (float) (std::tanh (3.0 * std::sin (6.2832 * 55.0 * s16)) * std::exp (-s16 * 18.0) * 0.25) : 0.0f;
        }
    }
}

// Kör fyra takter genom ett fack. mal[4] = nivå per del. Returnerar energin i ut och facit.
static void kor (const float mal[4], bool aktiv, std::vector<float>& ut, std::vector<float> facit[5], int bara = -1)
{
    fack::Fack f;
    f.forbered (SR);
    f.sattDela (aktiv);
    const int lat = f.latens();
    const double sps = BPM / 60.0 / SR;
    const int takter = 8, n = (int) (takter * 4 / sps);
    std::vector<float> L (n), R (n);
    std::mt19937 rng[5] = { std::mt19937 (1), std::mt19937 (2), std::mt19937 (3), std::mt19937 (4), std::mt19937 (5) };
    for (int k = 0; k < 5; ++k) facit[k].assign (n, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        float x = 0;
        for (int k = 0; k < 5; ++k) { const float v = del (k, i * sps, rng[k]); facit[k][i] = v; if (bara < 0 || bara == k) x += v; }
        L[i] = R[i] = x;
    }
    const int blk = 512;
    for (int i0 = 0; i0 < n; i0 += blk)
    {
        const int m = std::min (blk, n - i0);
        const double slag0 = i0 * sps - lat * sps;
        f.kor (L.data() + i0, R.data() + i0, m, slag0, sps, true, 1.0f, 1.0f, [&] (double)
        {
            motor::Atgard a;
            a.dela = aktiv;
            for (int i = 0; i < 4; ++i) a.delar[i] = mal[i];
            return a;
        });
    }
    ut.assign (n, 0.0f);
    for (int i = 0; i + lat < n; ++i) ut[i] = L[i + lat];   // tillbaka i linje med facit
}

int main()
{
    std::vector<float> ut, facit[5], ref[5], tmp[5];
    const float alla[4] = { 1, 1, 1, 1 };
    // facit genom samma kanal utan delare (kanalens filter är inte helt genomskinliga)
    for (int k = 0; k < 5; ++k) kor (alla, false, ref[k], tmp, k);
    std::vector<float> refSum (ref[0].size(), 0.0f);
    for (int k = 0; k < 5; ++k) for (size_t i = 0; i < refSum.size(); ++i) refSum[i] += ref[k][i];
    const size_t start = (size_t) (SR * 1.0), slut = refSum.size() - (size_t) (SR * 0.1);
    auto maxAvv = [&] (const std::vector<float>& a, const std::vector<float>& b)
    { double m = 0; for (size_t i = start; i < slut; ++i) m = std::max (m, (double) std::abs (a[i] - b[i])); return m; };

    // 1. allt på: samma som utan delare
    kor (alla, true, ut, facit);
    std::printf ("allt på: största avvikelse %.6f\n", maxAvv (ut, refSum));
    KOLL (maxAvv (ut, refSum) < 1e-3);

    // 2. en del i taget bort. Varje ljud körs genom en egen kopia av delaren som hör hela
    //    loopen (samma grindar), så vi ser exakt vad som hände med varje ljud.
    const char* namn[4] = { "KICK", "CLAP", "HH", "OH" };
    for (int d = 0; d < 4; ++d)
    {
        float mal[4] = { 1, 1, 1, 1 };
        mal[d] = 0.0f;
        const int framLen = std::max (64, (int) std::lround (SR * 0.030));
        delare::Delare kopia[5];
        for (auto& k : kopia) k.forbered (SR, framLen);
        const double sps = BPM / 60.0 / SR;
        const size_t n = facit[0].size();
        std::vector<float> mix (n, 0.0f);
        for (int k = 0; k < 5; ++k) for (size_t i = 0; i < n; ++i) mix[i] += facit[k][i];
        double inE[5] {}, utE[5] {};
        for (size_t i = 0; i < n; ++i)
        {
            const int pos = (int) (i % (size_t) (framLen + 1));
            int las = pos + 1; if (las > framLen) las = 0;
            const long spel = (long) i - framLen;                 // samplet som spelas nu
            const double slagSpel = spel * sps;
            for (int k = 0; k < 5; ++k)
            {
                kopia[k].lyssna (mix[i], pos, i * sps);
                float l = spel >= 0 ? facit[k][(size_t) spel] : 0.0f, r = l;
                const float fore = l;
                kopia[k].dela (l, r, las, slagSpel, mal, 1.0f, true);
                if (spel >= (long) start && spel < (long) slut) { inE[k] += (double) fore * fore; utE[k] += (double) l * l; }
            }
        }
        double restIn = 0, restUt = 0;
        for (int k = 0; k < 5; ++k) if (k != d) { restIn += inE[k]; restUt += utE[k]; }
        const double borta = 1.0 - utE[d] / inE[d];
        const double restKvar = restUt / restIn;
        std::printf ("%-4s bort: %5.1f %% av delen försvann, resten kvar %5.1f %%  (", namn[d], 100.0 * borta, 100.0 * restKvar);
        for (int k = 0; k < 5; ++k) if (k != d) std::printf (" %s %.0f%%", k < 4 ? namn[k] : "BAS", 100.0 * utE[k] / inE[k]);
        std::printf (" )\n");
        KOLL (borta > 0.6);
        KOLL (restKvar > 0.75);
    }

    // 3. allt av: bara resten (basgången) kvar till största delen
    {
        const float inga[4] = { 0, 0, 0, 0 };
        kor (inga, true, ut, facit);
        double utE = 0, totE = 0, basE = 0;
        for (size_t i = start; i < slut; ++i) { utE += ut[i] * ut[i]; totE += refSum[i] * refSum[i]; basE += ref[4][i] * ref[4][i]; }
        std::printf ("allt av: %.1f %% av energin kvar (basgången är %.1f %%)\n", 100.0 * utE / totE, 100.0 * basE / totE);
        KOLL (utE < totE * 0.4);
    }

    // 4. delaren avstängd: orört
    {
        const float inga[4] = { 0, 0, 0, 0 };
        kor (inga, false, ut, facit);
        KOLL (maxAvv (ut, refSum) < 1e-3);
    }

    std::printf (fel ? "\n%d FEL\n" : "\nDelaren OK\n", fel);
    return fel ? 1 : 0;
}
