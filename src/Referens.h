#pragma once

// STÄLLVERKET · REFERENSLÄSAREN
//
// Läs in en låt du gillar, så gör ställverket en byggmall av den: energin
// per 8 takter blir KURVAN, och var basen försvinner, var det byggs upp och var
// det smäller blir SKRIFTEN (t.ex. "I16 G32 B16 U8 D16 K32 O16").
// Ingen musik sparas eller kopieras: bara hur låten är byggd.
//
// Ren C++: tar ljudet som mono och tempot (från DAW:en), ger kurva och skrift.

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace referens
{
struct Resultat
{
    bool ok = false;
    float kurva[8] {};
    int kurvTakter = 256;      // närmaste av 128, 256, 512, 1024, 2048
    std::string skrift;        // för mallen SKRIFTEN
    int takter = 0;
    std::vector<float> energi, bas;   // per 8-taktsblock, 0..1 (för panelen)
};

inline Resultat analysera (const std::vector<float>& mono, double sr, double bpm)
{
    Resultat r;
    if (mono.empty() || sr <= 0 || bpm <= 0) return r;
    const double sekPerBlock = 8.0 * 4.0 * 60.0 / bpm;
    const size_t perBlock = (size_t) (sekPerBlock * sr);
    const size_t antal = mono.size() / std::max<size_t> (1, perBlock);
    if (antal < 4) return r;
    r.takter = (int) (antal * 8);

    // energi totalt och i basen (under ~150 Hz) per block
    const float aLag = 1.0f - std::exp (-6.2832f * 150.0f / (float) sr);
    float lag = 0, lag2 = 0;
    for (size_t b = 0; b < antal; ++b)
    {
        double e = 0, eb = 0;
        for (size_t i = b * perBlock; i < (b + 1) * perBlock; ++i)
        {
            const float x = mono[i];
            lag += aLag * (x - lag); lag2 += aLag * (lag - lag2);
            e += (double) x * x; eb += (double) lag2 * lag2;
        }
        r.energi.push_back ((float) std::sqrt (e / perBlock));
        r.bas.push_back ((float) std::sqrt (eb / perBlock));
    }
    auto normera = [] (std::vector<float>& v)
    {
        std::vector<float> s = v; std::sort (s.begin(), s.end());
        const float lo = s[s.size() / 20], hi = s[s.size() - 1 - s.size() / 20];   // 5 % av och på för att tåla toppar
        for (auto& x : v) x = std::clamp ((x - lo) / std::max (1e-6f, hi - lo), 0.0f, 1.0f);
    };
    normera (r.energi); normera (r.bas);

    // ---- kurvan: åtta punkter jämnt över låten (lite utjämnad)
    for (int k = 0; k < 8; ++k)
    {
        const double pos = (double) k / 7.0 * (double) (antal - 1);
        const int i = (int) pos;
        float s = 0, w = 0;
        for (int j = i - 1; j <= i + 1; ++j) if (j >= 0 && j < (int) antal) { s += r.energi[(size_t) j]; w += 1; }
        r.kurva[k] = std::clamp (0.15f + 0.85f * (s / std::max (1.0f, w)), 0.0f, 1.0f);
    }
    {
        const int alt[] = { 128, 256, 512, 1024, 2048 };
        int basta = 256; double avst = 1e9;
        for (int a : alt) { const double d = std::abs (std::log ((double) a / r.takter)); if (d < avst) { avst = d; basta = a; } }
        r.kurvTakter = basta;
    }

    // ---- delarna, ett 8-taktsblock i taget
    enum { I, G, B, U, D, K, O };
    std::vector<int> typ (antal, G);
    size_t introSlut = 0;
    while (introSlut < antal / 3 && (r.bas[introSlut] < 0.55f || r.energi[introSlut] < 0.5f)) typ[introSlut++] = I;
    size_t outroStart = antal;
    while (outroStart > introSlut + 2 && (r.bas[outroStart - 1] < 0.55f || r.energi[outroStart - 1] < 0.45f)
           && outroStart > antal - antal / 4) typ[--outroStart] = O;
    for (size_t b = introSlut; b < outroStart; ++b)
    {
        const float e = r.energi[b], ba = r.bas[b];
        if (ba < 0.35f) typ[b] = B;                       // basen borta = break
        else if (e > 0.85f) typ[b] = K;
        else typ[b] = G;
    }
    // efter ett break: det sista breakblocket som stiger blir BYGG, blocket efter blir DROP
    for (size_t b = introSlut + 1; b + 1 < outroStart; ++b)
        if (typ[b] == B && typ[b + 1] != B)
        {
            if (b > introSlut && typ[b - 1] == B) typ[b] = U;   // långt break: sista biten byggs upp
            // det som smäller in efter breaket är droppet (upp till 32 takter), sedan klimax
            for (size_t j = b + 1; j < outroStart && j < b + 5 && typ[j] != B && r.energi[j] > 0.5f; ++j) typ[j] = D;
        }
    // en topp efter ett vanligt groove utan break: det blir ett drop också
    for (size_t b = introSlut + 1; b < outroStart; ++b)
        if (typ[b] == K && typ[b - 1] == G && r.energi[b] - r.energi[b - 1] > 0.3f) typ[b] = D;

    // ---- slå ihop till skrift (längder i takter, högst 128 per del)
    static const char bokstav[] = { 'I', 'G', 'B', 'U', 'D', 'K', 'O' };
    for (size_t b = 0; b < antal;)
    {
        size_t e = b;
        while (e < antal && typ[e] == typ[b] && (e - b) < 16) ++e;
        if (! r.skrift.empty()) r.skrift += ' ';
        r.skrift += bokstav[typ[b]];
        r.skrift += std::to_string ((int) (e - b) * 8);
        b = e;
    }
    r.ok = true;
    return r;
}
} // namespace referens
