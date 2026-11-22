#pragma once

// STÄLLVERKET · lyssnaren
//
// Lyssnar på det som kommer in på kanalen i fyra takter och gissar vilken ROLL
// det är: hur energin fördelar sig mellan bas, mellanregister och diskant,
// hur många anslag det är per slag och var i takten de ligger.
// Gissningen byts bara om samma svar kommer två gånger i rad.

#include <cmath>
#include <algorithm>
#include "Motor.h"

namespace lyssna
{
class Lyssnare
{
public:
    void forbered (double sampleRate)
    {
        sr = (float) sampleRate;
        aLag  = 1.0f - std::exp (-6.2832f * 150.0f / sr);
        aHog  = 1.0f - std::exp (-6.2832f * 5000.0f / sr);
        aSnabb = 1.0f - std::exp (-1.0f / (0.008f * sr));
        aLangsam = 1.0f - std::exp (-1.0f / (0.120f * sr));
        nollstall();
        gissning = -1; kandidat = -1;
    }

    void nollstall()
    {
        eL = eM = eH = 0.0; anslag = paSlag = baktakt = 0; startSlag = -1.0; ljudSampel = 0; sampel = 0;
    }

    // x = mono in, slag = position i slag för samplet
    inline void sampla (float x, double slag)
    {
        if (startSlag < 0.0) startSlag = std::floor (slag);
        senasteSlag = slag;
        lag += aLag * (x - lag);
        hogLp += aHog * (x - hogLp);
        const float hog = x - hogLp;
        const float mitt = hogLp - lag;
        eL += (double) lag * lag; eM += (double) mitt * mitt; eH += (double) hog * hog;
        ++sampel;

        const float ax = std::abs (x);
        if (ax > 0.003f) ++ljudSampel;
        snabb += aSnabb * (ax - snabb);
        langsam += aLangsam * (ax - langsam);
        if (vila > 0) --vila;
        else if (snabb > langsam * 2.2f && snabb > 0.02f)
        {
            ++anslag;
            const double fas = slag - std::floor (slag + 0.5);
            if (std::abs (fas) < 0.07)
            {
                ++paSlag;
                if (((int64_t) std::floor (slag + 0.5)) % 2 == 1) ++baktakt;
            }
            vila = (int) (sr * 0.09f);
        }

        if (slag - startSlag >= (gissning < 0 ? 8.0 : 16.0)) { klassa(); nollstall(); }
    }

    int roll() const { return gissning; }

private:
    void klassa()
    {
        const double tot = eL + eM + eH;
        if (tot < 1e-6 || ljudSampel < sampel / 10) return;   // för tyst för att säga något
        const double fL = eL / tot, fM = eM / tot, fH = eH / tot;
        const double perSlag = anslag / std::max (1.0, senasteSlag - startSlag);
#ifdef LYSSNA_DEBUG
        std::printf ("fL %.2f fM %.2f fH %.2f perSlag %.2f paSlag %.2f bak %.2f\n", fL, fM, fH, perSlag, anslag ? (double) paSlag / anslag : 0.0, paSlag ? (double) baktakt / paSlag : 0.0);
#endif
        const double paSlagAndel = anslag > 0 ? (double) paSlag / anslag : 0.0;
        const double bakAndel = paSlag > 0 ? (double) baktakt / paSlag : 0.0;

        int r = motor::LOOP;
        if (fL > 0.55)
            r = (perSlag > 0.6 && perSlag < 2.3 && paSlagAndel > 0.55) ? motor::KICK : motor::BAS;
        else if (perSlag >= 0.3 && perSlag <= 1.2 && paSlagAndel > 0.6 && bakAndel > 0.7)
            r = motor::CLAP;
        else if (fH > 0.5)
            r = perSlag >= 1.5 ? motor::HATS : (perSlag >= 0.6 ? motor::RIDE : motor::ATMO);
        else if (fL > 0.2 && fM > 0.2 && fH > 0.15 && perSlag > 1.0)
            r = motor::LOOP;
        else if (perSlag > 1.0)
            r = motor::PERC;
        else if (perSlag < 0.15)
            r = fM > 0.5 ? motor::SYNT : motor::ATMO;
        else
            r = motor::SYNT;

        if (r == kandidat) gissning = r;
        kandidat = r;
        if (gissning < 0) gissning = r;   // första gissningen direkt
    }

    float sr = 48000.0f, aLag = 0, aHog = 0, aSnabb = 0, aLangsam = 0;
    float lag = 0, hogLp = 0, snabb = 0, langsam = 0;
    double eL = 0, eM = 0, eH = 0, startSlag = -1.0, senasteSlag = 0.0;
    int anslag = 0, paSlag = 0, baktakt = 0, vila = 0;
    int64_t ljudSampel = 0, sampel = 0;
    int gissning = -1, kandidat = -1;
};
} // namespace lyssna
