#pragma once

// STÄLLVERKET · ljudet i en kanal
//
// Tar motorns åtgärd och gör den på ljudet som strömmar igenom:
// nivå och grind, högpass och lågpass, stutter (upprepar en bit av det som
// just spelats), baklänges (förra slaget spelat bakåt in i droppet) och
// dubkast (ett eko som ringer vidare när kanalen tystnar).
// Allt som spelas spelas också in i en ringbuffert, det är den som stutter
// och baklänges läser ur.

#include <cmath>
#include <vector>
#include <algorithm>
#include "Motor.h"

namespace fack
{
// Tillståndsvariabelfilter (TPT), stereo.
struct Svf
{
    float ic1[2] {}, ic2[2] {};
    float g = 0, k = 1.2f, a1 = 0, a2 = 0, a3 = 0;

    void satt (float fc, float sr, float q = 0.75f)
    {
        fc = std::clamp (fc, 15.0f, sr * 0.45f);
        g = std::tan (3.14159265f * fc / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void nolla() { ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0; }

    // ger lågpass, och högpass via referens
    inline float kor (float x, int c, float& hp)
    {
        const float v3 = x - ic2[c];
        const float v1 = a1 * ic1[c] + a2 * v3;
        const float v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
        ic1[c] = 2.0f * v1 - ic1[c];
        ic2[c] = 2.0f * v2 - ic2[c];
        hp = x - k * v1 - v2;
        return v2;
    }
};

// Litet rum (Schroeder): fyra kamfilter och två allpass per sida. Inget brus, bara svans.
struct Rum
{
    std::vector<float> kam[8], ap[4];
    int kPos[8] {}, aPos[4] {};
    float damp[8] {};

    void forbered (float sr)
    {
        static const int kl[] = { 1116, 1188, 1277, 1356 };
        static const int al[] = { 556, 441 };
        const float f = sr / 44100.0f;
        for (int i = 0; i < 8; ++i) { kam[i].assign ((size_t) ((kl[i % 4] + (i >= 4 ? 23 : 0)) * f), 0.0f); kPos[i] = 0; damp[i] = 0; }
        for (int i = 0; i < 4; ++i) { ap[i].assign ((size_t) ((al[i % 2] + (i >= 2 ? 23 : 0)) * f), 0.0f); aPos[i] = 0; }
    }

    float hpLp[2] {};

    inline float sida (float x, int c)
    {
        // högpass in i rummet, så låga frekvenser inte byggs upp i svansen
        hpLp[c] += 0.03f * (x - hpLp[c]);
        x -= hpLp[c];
        float ut = 0.0f;
        for (int i = c * 4; i < c * 4 + 4; ++i)
        {
            auto& b = kam[i];
            const float y = b[(size_t) kPos[i]];
            damp[i] = y * 0.6f + damp[i] * 0.4f;
            b[(size_t) kPos[i]] = x + damp[i] * 0.8f;
            if (++kPos[i] >= (int) b.size()) kPos[i] = 0;
            ut += y;
        }
        for (int i = c * 2; i < c * 2 + 2; ++i)
        {
            auto& b = ap[i];
            const float bv = b[(size_t) aPos[i]];
            b[(size_t) aPos[i]] = ut + bv * 0.5f;
            ut = bv - ut;
            if (++aPos[i] >= (int) b.size()) aPos[i] = 0;
        }
        return ut * 0.1f;
    }
};

class Fack
{
public:
    void forbered (double sampleRate)
    {
        sr = (float) sampleRate;
        ring.assign ((size_t) (sr * 32.0f) * 2, 0.0f);
        ringLen = (int) (ring.size() / 2);
        eko.assign ((size_t) (sr * 4.0f) * 2, 0.0f);
        ekoLen = (int) (eko.size() / 2);
        skriv = 0; ekoSkriv = 0;
        lp.nolla(); hp.nolla();
        gain = 1; lpN = 1; hpN = 0; ekoLp[0] = ekoLp[1] = 0;
        stutterAktiv = false; bakAktiv = false;
        lp.satt (20000.0f, sr); hp.satt (20.0f, sr);
        raknare = 0;
        rum.forbered (sr);
        tapeAktiv = false; rumN = 0;
        halvAktiv = bakLangAktiv = frysAktiv = false; frysN = 0; bytN = 0; glappN = 0;
    }

    // Kör ett block. slag0 = position i slag vid första samplet, slagPerSampel = tempo.
    // spelar = false: ljudet går rakt igenom (mjukt tillbaka till neutralt).
    template <typename AtgardFn>
    void kor (float* L, float* R, int n, double slag0, double slagPerSampel, bool spelar,
              float djup, float mix, AtgardFn&& atgardVid)
    {
        if (ring.empty()) return;
        const float gTid = 1.0f - std::exp (-1.0f / (0.004f * sr));   // 4 ms
        const float gMjuk = 1.0f - std::exp (-1.0f / (0.030f * sr));  // 30 ms
        const float fTid = 1.0f - std::exp (-1.0f / (0.025f * sr));
        const double sampelPerSlag = slagPerSampel > 0 ? 1.0 / slagPerSampel : (double) sr * 0.5;

        motor::Atgard a;
        double nastaKoll = -1.0;

        for (int i = 0; i < n; ++i)
        {
            const double slag = slag0 + i * slagPerSampel;

            // ny åtgärd var 1/64-del (eller när transporten står)
            const double ruta = std::floor (slag * 16.0);
            if (! spelar) a = motor::Atgard();
            else if (ruta != nastaKoll) { a = atgardVid (slag); nastaKoll = ruta; }

            // djup: hur mycket motorn får styra den här kanalen
            const float d = spelar ? djup : 0.0f;
            const int steg = (int) std::fmod (std::floor (slag * 4.0), 16.0);
            const bool grindPa = ((a.grind >> steg) & 1u) != 0;
            const float malNiva = 1.0f + ((a.niva * (grindPa ? 1.0f : 0.0f)) - 1.0f) * d;
            const float malLp = 1.0f + (a.lp - 1.0f) * d;
            const float malHp = a.hp * d;
            const bool  stut = d > 0.5f && a.stutter > 0.0f;
            const bool  bak  = d > 0.5f && a.bak;
            const float dub  = a.dub * d;

            gain += ((a.grind != 0xFFFF || a.hard) ? gTid : gMjuk) * (malNiva - gain);
            lpN  += fTid * (malLp - lpN);
            hpN  += fTid * (malHp - hpN);

            if ((raknare++ & 15) == 0)
            {
                lp.satt (60.0f * std::pow (2.0f, lpN * 8.4f), sr, 0.8f);
                hp.satt (20.0f * std::pow (2.0f, hpN * 7.0f), sr, 0.75f);
            }

            const float inL = L[i], inR = R[i];
            ring[(size_t) skriv * 2] = inL;
            ring[(size_t) skriv * 2 + 1] = inR;

            float xL = inL, xR = inR;

            // ---- stutter: första biten spelas som vanligt, sedan upprepas den
            if (stut)
            {
                const double bit = a.stutter * 0.25;                   // slag per bit
                const double bitIndex = std::floor (slag / bit);
                const int bitLen = std::max (16, (int) (bit * sampelPerSlag));
                if (! stutterAktiv || a.stutter != stutterLangd)
                {
                    stutterAktiv = true;
                    stutterLangd = a.stutter;
                    stutterBit = bitIndex;
                    const double fas = (slag - bitIndex * bit) * sampelPerSlag;
                    ankare = skriv - (int) fas;
                }
                if (bitIndex > stutterBit)
                {
                    const int fas = (int) ((slag - bitIndex * bit) * sampelPerSlag) % bitLen;
                    // ton > 1: biten spelas snabbare och upprepas inom sig själv, så den stiger i tonhöjd
                    const double tonFas = std::fmod ((double) fas * std::max (1.0f, a.ton), (double) bitLen);
                    // korta övertoningar i bitarnas kanter, så det inte klickar
                    const int kant = std::min (bitLen / 4, (int) (sr * 0.002f));
                    float env = 1.0f;
                    if (fas < kant) env = (float) fas / (float) kant;
                    else if (fas > bitLen - kant) env = (float) (bitLen - fas) / (float) kant;
                    if (a.ton > 1.0f)
                    {
                        const double tf = tonFas;
                        if (tf < kant) env *= (float) (tf / kant);
                        else if (tf > bitLen - kant) env *= (float) ((bitLen - tf) / kant);
                    }
                    lasRing ((double) ankare + tonFas, xL, xR);
                    xL *= env; xR *= env;
                }
            }
            else stutterAktiv = false;

            // ---- BANDGLAPP: läser lite efter, med en fördröjning som sväller och krymper = tonhöjden dippar
            if (d > 0.5f && a.glapp && ! stut)
            {
                const double len = std::max (64.0, sampelPerSlag);
                const double fas = std::fmod ((double) glappN / len, 1.0);
                const double s = std::sin (3.14159265 * fas);
                lasRing ((double) skriv - s * s * sr * 0.012, xL, xR);
                ++glappN;
            }
            else glappN = 0;

            // ---- HALVTID: spelar från breakets start i halv hastighet (en oktav ner)
            if (d > 0.5f && a.halvtid && ! stut)
            {
                if (! halvAktiv) { halvAktiv = true; halvAnkare = skriv; halvN = 0; }
                lasRing ((double) halvAnkare + halvN * 0.5, xL, xR);
                ++halvN;
            }
            else halvAktiv = false;

            // ---- BAKLÄNGES-BREAK: allt som spelade innan breaket, baklänges
            if (d > 0.5f && a.bakLang && ! stut)
            {
                if (! bakLangAktiv) { bakLangAktiv = true; bakLangAnkare = skriv; bakLangN = 0; }
                const int k = bakLangN % (ringLen - 1);
                lasRing ((double) bakLangAnkare - 1 - k, xL, xR);
                const float env = std::min (1.0f, (float) bakLangN / (sr * 0.01f));
                xL *= env; xR *= env;
                ++bakLangN;
            }
            else bakLangAktiv = false;

            // ---- baklänges: förra slaget bakåt, tonar upp
            if (bak)
            {
                if (! bakAktiv) { bakAktiv = true; bakAnkare = skriv; bakN = 0; }
                const int len = std::max (64, (int) sampelPerSlag);
                const int k = bakN % len;
                int las = bakAnkare - 1 - k;
                las = ((las % ringLen) + ringLen) % ringLen;
                const float env = std::min (1.0f, (float) k / (float) len * 1.2f);
                xL = ring[(size_t) las * 2] * env;
                xR = ring[(size_t) las * 2 + 1] * env;
                ++bakN;
            }
            else bakAktiv = false;

            // ---- tape stop: bandet saktar in under slaget och stannar
            if (d > 0.5f && a.tapeStop)
            {
                if (! tapeAktiv) { tapeAktiv = true; tapeAnkare = skriv; tapePos = 0.0; tapeN = 0; }
                const double len = std::max (64.0, sampelPerSlag * std::max (0.25f, a.tapeLangd));
                const double fas = std::min (1.0, (double) tapeN / len);
                const double fart = 1.0 - fas;                       // 1 -> 0
                tapePos += fart * fart * 0.5 + fart * 0.5;           // tappar fart mjukt
                lasRing ((double) tapeAnkare + tapePos, xL, xR);
                const float env = (float) std::min (1.0, (1.0 - fas) * 8.0);   // tona ut sista biten
                xL *= env; xR *= env;
                ++tapeN;
            }
            else tapeAktiv = false;

            // ---- filter: lågpass, sedan högpass
            float skrap, hpL, hpR;
            xL = lp.kor (xL, 0, skrap);
            xR = lp.kor (xR, 1, skrap);
            hp.kor (xL, 0, hpL);
            hp.kor (xR, 1, hpR);
            xL = hpL; xR = hpR;

            // ---- dubkast: skickas före nivån, så ekot ringer vidare när kanalen tystnar
            const int ekoTid = std::clamp ((int) (sampelPerSlag * 0.75), 64, ekoLen - 1);
            int ekoLas = ekoSkriv - ekoTid; if (ekoLas < 0) ekoLas += ekoLen;
            const float eL = eko[(size_t) ekoLas * 2], eR = eko[(size_t) ekoLas * 2 + 1];
            ekoLp[0] += 0.25f * (eR - ekoLp[0]);       // korsat: ekot pingpongar
            ekoLp[1] += 0.25f * (eL - ekoLp[1]);
            eko[(size_t) ekoSkriv * 2]     = xL * dub + ekoLp[0] * 0.55f;
            eko[(size_t) ekoSkriv * 2 + 1] = xR * dub + ekoLp[1] * 0.55f;
            if (++ekoSkriv >= ekoLen) ekoSkriv = 0;

            // ---- pump: kicken trycker ner kanalen på varje slag (ingen routing, följer takten)
            float pumpG = 1.0f;
            if (a.pump > 0.0f && spelar)
            {
                const double fas = slag - std::floor (slag);
                const double x = fas / std::max (0.05, (double) a.pumpTid);
                double duck = x >= 1.0 ? 0.0 : (1.0 - x) * (1.0 - x);
                duck *= std::min (1.0, fas / 0.012);                 // snabb attack, inget klick
                pumpG = 1.0f - a.pump * d * (float) duck;
            }
            pumpN += 0.35f * (pumpG - pumpN);

            // ---- rum: skickas före nivån, svansen ringer vidare in i nästa del
            rumN += 0.002f * (a.rum * d - rumN);
            float rL = 0.0f, rR = 0.0f;
            if (rumN > 0.0005f || rumSvans > 0)
            {
                rL = rum.sida (xL * rumN, 0);
                rR = rum.sida (xR * rumN, 1);
                rumSvans = rumN > 0.0005f ? (int) (sr * 4.0f) : rumSvans - 1;
            }

            // ---- FRYS: 60 ms precis innan låses och loopas mjukt med två överlappande läshuvuden
            float fL = 0.0f, fR = 0.0f;
            if (d > 0.5f && a.frys > 0.001f)
            {
                const int L60 = std::max (256, (int) (sr * 0.06f));
                if (! frysAktiv) { frysAktiv = true; frysAnkare = skriv - L60 - 1; frysN = 0; }
                for (int h = 0; h < 2; ++h)
                {
                    const int fas = (frysN + h * (L60 / 2)) % L60;
                    const float w = 0.5f - 0.5f * std::cos (6.2831853f * (float) fas / (float) L60);
                    float l, r;
                    lasRing ((double) frysAnkare + fas, l, r);
                    fL += l * w; fR += r * w;
                }
                fL *= a.frys; fR *= a.frys;
                ++frysN;
            }
            else frysAktiv = false;

            float oL = xL * gain * pumpN + eL + rL + fL;
            float oR = xR * gain * pumpN + eR + rR + fR;

            // ---- KANALBYTE: vänster och höger byter plats, mjukt
            bytN += 0.001f * ((a.byt && d > 0.5f ? 1.0f : 0.0f) - bytN);
            if (bytN > 0.0005f)
            {
                const float b = bytN, l0 = oL, r0 = oR;
                oL = l0 * (1.0f - b) + r0 * b;
                oR = r0 * (1.0f - b) + l0 * b;
            }

            // mix: hur mycket av maskinen som hörs mot originalet
            L[i] = inL + (oL - inL) * mix;
            R[i] = inR + (oR - inR) * mix;

            topp = std::max (topp * 0.99995f, std::max (std::abs (L[i]), std::abs (R[i])));
            if (++skriv >= ringLen) skriv = 0;
        }
    }

    inline void lasRing (double pos, float& l, float& r) const
    {
        double p = std::fmod (pos, (double) ringLen);
        if (p < 0) p += ringLen;
        const int i0 = (int) p;
        const int i1 = (i0 + 1) % ringLen;
        const float fr = (float) (p - i0);
        l = ring[(size_t) i0 * 2] + (ring[(size_t) i1 * 2] - ring[(size_t) i0 * 2]) * fr;
        r = ring[(size_t) i0 * 2 + 1] + (ring[(size_t) i1 * 2 + 1] - ring[(size_t) i0 * 2 + 1]) * fr;
    }

    float nivaMatare() const { return topp; }
    float aktuellGain() const { return gain; }
    float aktuellLp() const { return lpN; }
    float aktuellHp() const { return hpN; }

private:
    float sr = 48000.0f;
    std::vector<float> ring, eko;
    int ringLen = 0, ekoLen = 0, skriv = 0, ekoSkriv = 0;
    Svf lp, hp;
    float gain = 1, lpN = 1, hpN = 0, pumpN = 1, ekoLp[2] {};
    Rum rum;
    float rumN = 0;
    int rumSvans = 0;
    bool tapeAktiv = false;
    int tapeAnkare = 0, tapeN = 0;
    double tapePos = 0;
    bool halvAktiv = false, bakLangAktiv = false, frysAktiv = false;
    int halvAnkare = 0, halvN = 0, bakLangAnkare = 0, bakLangN = 0, frysAnkare = 0, frysN = 0, glappN = 0;
    float bytN = 0;
    bool stutterAktiv = false, bakAktiv = false;
    float stutterLangd = 0;
    double stutterBit = 0;
    int ankare = 0, bakAnkare = 0, bakN = 0;
    unsigned raknare = 0;
    float topp = 0;
};
} // namespace fack
