#pragma once

// STÄLLVERKET · LOOPDELAREN
//
// Delar en trumloop i fyra delar som motorn kan ta bort och lägga till var för sig:
//   KICK  = basbandet runt varje kickslag, plus kickens punch (200-700 Hz) en kort stund
//   CLAP  = övre mellanbandet (700 Hz-5 kHz) när ett brett, brusigt slag kommer (clap, virvel)
//   HH    = diskanten runt korta hihatslag
//   OH    = diskanten runt långa (öppna) hihatslag
// Det som blir över (basgång, perc, synt i loopen) är RESTEN och rörs inte.
//
// Så funkar det, utan AI och utan att datorn går varm:
//  - Ljudet delas i fyra band med filter som alltid summerar till originalet:
//    bas (under 200 Hz), låg mitt (200-700 Hz), hög mitt (700 Hz-5 kHz) och diskant.
//  - Ställverket hör varje slag 30 ms innan det spelas (framåtblicken), så
//    grindarna öppnar en aning före slaget. Inget huggs av.
//  - Loopar upprepar sig, så delaren lär sig loopen: för varje 1/16-steg i två
//    takter kommer den ihåg om hihaten där brukar ringa länge (öppen) eller kort.
//  - Delarna dras bort från originalet. Är alla delar på blir ljudet exakt som
//    förut, bit för bit.

#include <cmath>
#include <algorithm>
#include <vector>
#include <atomic>
#include <cstdint>

namespace delare
{
// Enkelt tvåpolsfilter (TPT), en kanal.
struct Filt
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1.414f, a1 = 0, a2 = 0, a3 = 0;
    void satt (float fc, float sr, float q = 0.707f)
    {
        fc = std::clamp (fc, 15.0f, sr * 0.45f);
        g = std::tan (3.14159265f * fc / sr); k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void nolla() { ic1 = ic2 = 0; }
    inline void kor (float x, float& lp, float& hp)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        lp = v2; hp = x - k * v1 - v2;
    }
};

class Delare
{
public:
    static constexpr int kSteg = 32;   // 1/16-steg i två takter

    void forbered (float sampleRate, int framLangd)
    {
        sr = sampleRate;
        framLen = framLangd;
        flaggor.assign ((size_t) framLen + 1, 0);
        for (int c = 0; c < 2; ++c)
        {
            lag[c].satt (200.0f, sr); hog[c].satt (5000.0f, sr); mitt[c].satt (700.0f, sr);
            lag[c].nolla(); hog[c].nolla(); mitt[c].nolla();
        }
        // lyssnarens filter behöver inte summera till originalet, så de får vara branta
        dLag.satt (200.0f, sr); dLag2.satt (200.0f, sr);
        dHog.satt (6500.0f, sr); dHog2.satt (6500.0f, sr);
        dMitt.satt (800.0f, sr); dMitt2.satt (4000.0f, sr);
        for (auto* f : { &dLag, &dLag2, &dHog, &dHog2, &dMitt, &dMitt2 }) f->nolla();
        // Banden byggs av lågpass och exakt rest (inte av högpass), så att en clap på 1,5 kHz
        // inte läcker in i grannbanden när filtren vrider fasen. Summan är alltid originalet.
        for (int b = 0; b < 3; ++b) { snabb[b] = langsam[b] = 0; vila[b] = 0; }
        for (int i = 0; i < 4; ++i) { grind[i] = 0; hall[i] = 0; niva[i] = 1.0f; }
        restN = 1.0f; punch = 0; punchG = 0.0f; clapDisk = 0; clapDiskG = 0.0f;
        cN = 0; cMitt = cDisk = 0.0f;
        kN = kLangdN = 0; kMater = false; kTopp = basMax = 0.0f; kickLangd = 0.22f;
        basGlom = std::exp (-1.0f / (4.0f * sr));   // starkaste basslaget glöms bort på ett par sekunder
        for (int s = 0; s < kSteg; ++s) ohKarta[s] = (s % 4 == 2) ? 0.7f : 0.2f;   // offbeat = öppen, tills den lärt sig
        matN = 0;
        aSnabb   = 1.0f - std::exp (-1.0f / (0.0015f * sr));
        aLangsam = 1.0f - std::exp (-1.0f / (0.045f * sr));
        aUpp     = 1.0f - std::exp (-1.0f / (0.0006f * sr));
        const float ned[4] = { 0.015f, 0.025f, 0.012f, 0.040f };
        for (int i = 0; i < 4; ++i) aNed[i] = 1.0f - std::exp (-1.0f / (ned[i] * sr));
        aNiva    = 1.0f - std::exp (-1.0f / (0.008f * sr));
        fore     = std::min (framLen - 1, std::max (1, (int) (0.002f * sr)));   // grinden öppnar 2 ms före slaget
    }

    // Skriv-sidan: det som just kommit in (30 ms före det som spelas).
    //  pos = platsen i framåtblicksbufferten, slagSkriv = position i slag för samplet.
    inline void lyssna (float monoIn, int pos, double slagSkriv)
    {
        float lp, hp, x1, x2, skrap;
        dLag.kor (monoIn, x1, skrap);  dLag2.kor (x1, lp, skrap);      // bas, 4:e ordningen
        dHog.kor (monoIn, skrap, x2);  dHog2.kor (x2, skrap, hp);      // diskant, 4:e ordningen
        float m1, m2;
        dMitt.kor (monoIn, skrap, m1); dMitt2.kor (m1, m2, skrap);     // hög mitt: 800 Hz - 4 kHz
        const float band[3] = { std::abs (lp), std::abs (m2), std::abs (hp) };   // bas, hög mitt, diskant
        uint8_t f = 0;
        bool start[3] {};
        for (int b = 0; b < 3; ++b)
        {
            snabb[b]   += aSnabb   * (band[b] - snabb[b]);
            langsam[b] += aLangsam * (band[b] - langsam[b]);
            if (vila[b] > 0) { --vila[b]; continue; }
            const float troskel = b == 0 ? 0.02f : (b == 1 ? 0.012f : 0.006f);
            const float kvot = b == 2 ? 1.8f : 2.4f;          // hihats direkt efter en öppen hihat ska också höras
            if (snabb[b] > langsam[b] * kvot && snabb[b] > troskel)
            {
                start[b] = true;
                vila[b] = (int) (sr * (b == 2 ? 0.035f : 0.07f));
            }
        }
        flaggor[(size_t) pos] = 0;

        // ---- kick: ett basslag som hör till de starkaste. Svagare basslag är basgången och får vara kvar.
        if (start[0]) { kPos = pos; kTopp = 0.0f; kN = (int) (sr * 0.008f); kLangdN = 0; kMater = false; }
        if (kN > 0)
        {
            kTopp = std::max (kTopp, snabb[0]);
            if (--kN == 0)
            {
                basMax = std::max (basMax, kTopp);
                if (kTopp >= basMax * 0.55f)
                {
                    flaggor[(size_t) kPos] |= 1;             // i efterhand: 8 ms senare, men långt innan det spelas
                    kMater = true; kLangdN = (int) (sr * 0.008f);
                }
            }
        }
        basMax *= basGlom;
        // hur lång kicken är: tills basen fallit till en femtedel (eller nästa slag)
        if (kMater)
        {
            ++kLangdN;
            if (snabb[0] < kTopp * 0.2f || start[0] || kLangdN > (int) (sr * 0.35f))
            {
                kickLangd += 0.3f * ((float) kLangdN / sr - kickLangd);
                kMater = false;
            }
        }

        // ---- clap/virvel: hög mitt tänds och är stark mot diskanten (hihats har nästan allt ovanför 5 kHz, en clap har sin kropp i mitten)
        // Bestäms i efterhand: energin i hög mitt mot diskant under 10 ms efter slaget.
        // En clap har kroppen i mitten, en hihat nästan allt ovanför 5 kHz.
        if (start[1]) { cPos = pos; cN = (int) (sr * 0.010f); cMitt = cDisk = 0.0f; }
        if (cN > 0)
        {
            cMitt += band[1] * band[1]; cDisk += band[2] * band[2];
            if (--cN == 0 && cMitt > cDisk * 0.35f) flaggor[(size_t) cPos] |= 2;
        }
        if (start[2])
        {
            f |= 4;
            // mät hur länge hihaten ringer, för att lära sig öppen/sluten per steg
            matSteg = stegFor (slagSkriv);
            matTopp = snabb[2];
            matN = (int) (sr * 0.085f);
        }
        if (matN > 0 && --matN == 0)
        {
            const float lang = snabb[2] > matTopp * 0.33f ? 1.0f : 0.0f;
            ohKarta[matSteg] += 0.25f * (lang - ohKarta[matSteg]);
        }
        flaggor[(size_t) pos] |= f;
    }

    // Spel-sidan: dela samplet som spelas nu (x) och lägg tillbaka delarna
    // med sina nivåer. lasPos = platsen i bufferten för det som spelas.
    //  mal[4] = motorns nivåer för delarna (1 = kvar, 0 = borta), restMal = nivån för resten,
    //  pa = delaren aktiv. Är den inte aktiv går allt igenom orört.
    inline void dela (float& xL, float& xR, int lasPos, double slag, const float* mal, float restMal, bool pa)
    {
        // titta 2 ms fram: öppna grindarna precis innan slaget
        int fram = lasPos + fore; if (fram > framLen) fram -= framLen + 1;
        const uint8_t f = flaggor[(size_t) fram];
        if (f & 1) { hall[0] = (int) (sr * std::clamp (kickLangd * 1.1f, 0.05f, 0.35f)); punch = (int) (sr * 0.05f); }
        if (f & 2) { hall[1] = (int) (sr * 0.12f); clapDisk = (int) (sr * 0.025f); }
        if (f & 4)
        {
            const int s = stegFor (slag);
            if (ohKarta[s] > 0.5f) { hall[3] = (int) (sr * 0.45f); hall[2] = 0; }
            else                   { hall[2] = (int) (sr * 0.06f); hall[3] = std::min (hall[3], (int) (sr * 0.004f)); }
        }
        {
            const float m = punch > 0 ? 1.0f : 0.0f;
            if (punch > 0) --punch;
            punchG += (m > punchG ? aUpp : aNed[0]) * (m - punchG);
            const float c = clapDisk > 0 ? 1.0f : 0.0f;
            if (clapDisk > 0) --clapDisk;
            clapDiskG += (c > clapDiskG ? aUpp : aNed[1]) * (c - clapDiskG);
        }
        for (int i = 0; i < 4; ++i)
        {
            const float m = hall[i] > 0 ? 1.0f : 0.0f;
            if (hall[i] > 0) --hall[i];
            grind[i] += (m > grind[i] ? aUpp : aNed[i]) * (m - grind[i]);
            const float t = pa ? std::clamp (mal[i], 0.0f, 1.0f) : 1.0f;
            niva[i] += aNiva * (t - niva[i]);
        }
        restN += aNiva * ((pa ? std::clamp (restMal, 0.0f, 1.0f) : 1.0f) - restN);

        float ut[2] = { xL, xR };
        for (int c = 0; c < 2; ++c)
        {
            float b4[4];
            delaBand (ut[c], lag[c], hog[c], mitt[c], b4);
            const float lp = b4[0], lm = b4[1], hm = b4[2], h2 = b4[3];
            // resten på sin nivå, varje del på sin: x*rest + del*grind*(nivå - rest)
            ut[c] = ut[c] * restN
                  + (lp * grind[0] + lm * punchG) * (niva[0] - restN)
                  + hm * grind[1] * (niva[1] - restN)
                  + h2 * (clapDiskG * (niva[1] - restN)                                 // clappens smäll (25 ms) tar diskanten med sig
                          + (1.0f - clapDiskG) * (grind[2] * (niva[2] - restN) + grind[3] * (niva[3] - restN)));
        }
        xL = ut[0]; xR = ut[1];
        for (int i = 0; i < 4; ++i) aktiv[i].store (grind[i]), nivaVisning[i].store (niva[i]);
    }

    float aktivitet (int i) const { return aktiv[i].load(); }
    float nivaNu (int i) const    { return nivaVisning[i].load(); }
    float restNu() const          { return restN; }

private:
    // bas | låg mitt | hög mitt | diskant, där summan alltid är x
    static inline void delaBand (float x, Filt& fLag, Filt& fHog, Filt& fMitt, float* b)
    {
        float lp, hp;
        fLag.kor (x, lp, hp);
        const float ovan = x - lp;           // allt ovanför basen
        fHog.kor (ovan, lp, hp);
        const float mitten = lp;             // under 5 kHz
        b[0] = x - ovan;
        b[3] = ovan - mitten;                // diskant = rest
        fMitt.kor (mitten, lp, hp);
        b[1] = lp;                           // låg mitt
        b[2] = mitten - lp;                  // hög mitt = rest
    }

    static int stegFor (double slag)
    {
        const int64_t st = (int64_t) std::floor (slag * 4.0 + 0.25);   // avrundat till närmaste 1/16
        return (int) (((st % kSteg) + kSteg) % kSteg);
    }

    float sr = 48000.0f;
    int framLen = 64, fore = 96;
    std::vector<uint8_t> flaggor;
    Filt lag[2], hog[2], mitt[2], dLag, dHog, dMitt, dLag2, dHog2, dMitt2;
    int punch = 0, clapDisk = 0;
    float punchG = 0.0f, clapDiskG = 0.0f;
    int cPos = 0, cN = 0;
    float cMitt = 0.0f, cDisk = 0.0f;
    int kPos = 0, kN = 0, kLangdN = 0;
    bool kMater = false;
    float kTopp = 0.0f, basMax = 0.0f, basGlom = 0.99999f, kickLangd = 0.22f;
    float snabb[3] {}, langsam[3] {};
    int vila[3] {};
    float grind[4] {}, niva[4] { 1, 1, 1, 1 }, restN = 1.0f;
    int hall[4] {};
    float ohKarta[kSteg] {};
    int matN = 0, matSteg = 0;
    float matTopp = 0;
    float aSnabb = 0, aLangsam = 0, aUpp = 0, aNed[4] {}, aNiva = 0;
    std::atomic<float> aktiv[4] {}, nivaVisning[4] {};
};
} // namespace delare
