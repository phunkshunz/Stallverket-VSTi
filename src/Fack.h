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
                    int las = ankare + fas;
                    las = ((las % ringLen) + ringLen) % ringLen;
                    // korta övertoningar i bitarnas kanter, så det inte klickar
                    const int kant = std::min (bitLen / 4, (int) (sr * 0.002f));
                    float env = 1.0f;
                    if (fas < kant) env = (float) fas / (float) kant;
                    else if (fas > bitLen - kant) env = (float) (bitLen - fas) / (float) kant;
                    xL = ring[(size_t) las * 2] * env;
                    xR = ring[(size_t) las * 2 + 1] * env;
                }
            }
            else stutterAktiv = false;

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

            float oL = xL * gain + eL;
            float oR = xR * gain + eR;

            // mix: hur mycket av maskinen som hörs mot originalet
            L[i] = inL + (oL - inL) * mix;
            R[i] = inR + (oR - inR) * mix;

            topp = std::max (topp * 0.99995f, std::max (std::abs (L[i]), std::abs (R[i])));
            if (++skriv >= ringLen) skriv = 0;
        }
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
    float gain = 1, lpN = 1, hpN = 0, ekoLp[2] {};
    bool stutterAktiv = false, bakAktiv = false;
    float stutterLangd = 0;
    double stutterBit = 0;
    int ankare = 0, bakAnkare = 0, bakN = 0;
    unsigned raknare = 0;
    float topp = 0;
};
} // namespace fack
