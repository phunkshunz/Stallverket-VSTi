#pragma once

// STÄLLVERKET · GLITCH
//
// Effekter i stil med Effectrix, Glitch, Stutter Edit och Looperator, som motorn
// kastar in själv, och på rätt ställen, så det låter som ett grepp och inte som ett fel:
//  - frasslut: sista ett-två slagen i en 8-taktsfras får en gest (repeat som blir
//    tätare, baklänges, tape stop, gate, kross, tonhopp, ekokast eller filtersvep)
//  - uppbyggnad: sista två takterna växer: repeat som tätnar och stiger, kross som
//    ökar, filter som svänger allt snabbare
//  - groove: ibland ett enda 1/16-steg som hackar till, som en sekvenserad effekt
//  - breakets sista slag: ekokast, tape stop eller tonhopp ner
//  - aldrig i droppets första takt: där ska det smälla rent
// Varje kanal väljer själv vilka effekter som får användas och hur mycket.
// Ren C++, bestämd av planen: alla kanaler hör samma sak varje gång.

#include "Motor.h"

namespace glitch
{
enum Effekt : uint32_t
{
    G_REPEAT = 1u << 0, G_BAK = 1u << 1, G_TAPE = 1u << 2, G_GATE = 1u << 3,
    G_KROSS = 1u << 4, G_TON = 1u << 5, G_EKO = 1u << 6, G_SVEP = 1u << 7
};
constexpr int kAntal = 8;
inline const char* namn (int i)
{
    static const char* n[] = { "REPEAT", "BAKLÄNGES", "TAPE", "GATE", "KROSS", "TONHOPP", "EKOKAST", "SVEP" };
    return (i >= 0 && i < kAntal) ? n[i] : "?";
}

inline uint32_t valj (uint32_t mask, uint32_t h)
{
    uint32_t ok[kAntal]; int n = 0;
    for (int i = 0; i < kAntal; ++i) if (mask & (1u << i)) ok[n++] = 1u << i;
    return n > 0 ? ok[motor::blanda (h) % (uint32_t) n] : 0u;
}

// Gör en gest på åtgärden.  x = 0..1 genom gesten, langd = gestens längd i slag,
// iGest = slag in i gesten, vaxer = uppbyggnad (gesten blir intensivare mot slutet).
inline void gest (motor::Atgard& a, uint32_t ef, double iGest, double langd, int roll, bool vaxer, uint32_t h)
{
    using namespace motor;
    const float x = (float) (iGest / langd);
    switch (ef)
    {
        case G_REPEAT:
            // tätnar mot slutet: 1/8, 1/16, 1/32
            a.stutter = x < 0.5f ? 2.0f : (x < 0.8f ? 1.0f : 0.5f);
            if (vaxer && roll != KICK && roll != BAS) a.ton = 1.0f + 0.9f * x * x;   // botten stiger inte
            break;
        case G_BAK:
            if (roll != KICK) a.bak = true;
            break;
        case G_TAPE:
            a.tapeStop = true; a.tapeLangd = (float) langd;
            break;
        case G_GATE:
        {
            // trance-gate på 1/16 (eller 1/32 sista biten): varannan sextondel tyst
            const double sext = iGest * 4.0;
            const bool pa = ((int) std::floor (x > 0.75f ? sext * 2.0 : sext) & 1) == 0;
            if (! pa) { a.niva = 0.0f; a.hard = true; }
            break;
        }
        case G_KROSS:
            a.kross = vaxer ? 0.25f + 0.65f * x : 0.75f;
            break;
        case G_TON:
            a.stutter = 2.0f;
            a.ton = (blanda (h + 9) & 1) ? 0.5f : 2.0f;   // en oktav ner eller upp
            if (vaxer) a.ton = 1.0f + x;
            break;
        case G_EKO:
            a.dub = 1.0f; a.dubFb = 0.78f;
            if (x > 0.5f) a.niva = 0.0f;                 // kanalen tystnar, ekot skallrar vidare
            break;
        case G_SVEP:
        {
            // resonant filtersvep som går allt snabbare
            const double fart = vaxer ? 1.0 + 3.0 * x : 2.0;
            const double fas = iGest * fart;
            a.lp = std::fmin (a.lp, 0.3f + 0.7f * (float) (0.5 + 0.5 * std::cos (6.2831853 * fas)));
            a.svep = true;
            break;
        }
        default: break;
    }
}

// Lägg GLITCH på kanalens åtgärd.
//  mask  = vilka effekter kanalen tillåter, mangd = hur mycket (kanalens ratt × CENTRALs GLITCH)
inline void glitcha (motor::Atgard& a, const motor::Plan& p, const motor::Installning& in, const motor::Overstyr& o,
                     int kanal, int roll, double slag, int spt, uint32_t mask, float mangd)
{
    using namespace motor;
    if (mask == 0 || mangd <= 0.001f || roll < 0 || roll >= NUM_ROLL) return;
    if (spt < 1) spt = 4;
    const int64_t takt = (int64_t) std::floor (slag / spt);
    int64_t tid = 0;
    const Sektion s = aktuell (p, o, takt, tid);
    const double iTakt = slag - (double) takt * spt;
    const double slagKvarDel = (double) (s.langd - tid) * spt - iTakt;
    const uint32_t hk = blanda (s.h, (uint32_t) kanal, 0x611C);
    const bool tung = roll == KICK || roll == BAS;
    // kick och bas glitchar sällan, och aldrig med kross eller tonhopp (botten ska stå stadigt)
    uint32_t m = mask;
    if (tung) m &= ~(G_KROSS | G_TON | G_BAK | G_SVEP);
    if (m == 0) return;
    const float vikt = tung ? 0.3f : 1.0f;

    if (s.typ == DROP && tid == 0) return;   // droppets första takt: rent

    // ---- uppbyggnad: sista två takterna växer
    if (s.typ == BYGG && slagKvarDel <= 2.0 * spt)
    {
        if (slump01 (hk + 1) < mangd * 1.2f * vikt)
        {
            const double langd = 2.0 * spt;
            gest (a, valj (m, hk + 2), langd - slagKvarDel, langd, roll, true, hk);
        }
        return;
    }

    // ---- breakets sista slag
    if (s.typ == BREAK && slagKvarDel <= 1.0)
    {
        const uint32_t brk = m & (G_EKO | G_TAPE | G_TON);
        if (brk && slump01 (hk + 3) < mangd * 0.6f * vikt)
            gest (a, valj (brk, hk + 4), 1.0 - slagKvarDel, 1.0, roll, false, hk);
        return;
    }

    if (s.typ == INTRO || s.typ == OUTRO) return;

    // ---- frasslut: sista ett eller två slagen i en 8-taktsfras
    const int frasTakt = (int) (tid % 8);
    const uint32_t hf = blanda (hk, (uint32_t) (tid / 8), 0xF5);
    if (frasTakt == 7)
    {
        const double langd = (blanda (hf + 1) & 1) ? 2.0 : 1.0;
        const double kvarTakt = spt - iTakt;
        if (kvarTakt <= langd && slump01 (hf) < mangd * 0.75f * vikt)
            gest (a, valj (m, hf + 2), langd - kvarTakt, langd, roll, false, hf);
        return;
    }

    // ---- groove: ett enda 1/16-steg hackar till (fjärde takten i frasen), som en sekvenserad effekt
    if (frasTakt == 3 && ! tung && slump01 (hf + 5) < mangd * 0.35f)
    {
        static const int steg[] = { 6, 10, 13, 14, 15 };
        const int st = steg[blanda (hf + 6) % 5];
        const double start = st * 0.25;
        if (iTakt >= start && iTakt < start + 0.25)
        {
            uint32_t litet = m & (G_REPEAT | G_BAK | G_GATE | G_KROSS | G_TON);
            if (litet) gest (a, valj (litet, hf + 7), iTakt - start, 0.25, roll, false, hf);
        }
    }
}
} // namespace glitch
