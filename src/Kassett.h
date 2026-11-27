#pragma once

// STÄLLVERKET · BETONG-KASSETT
//
// Gör en STÄLLVERKET-kanal till en BETONG: den spelar ett BETONG-kit med ett
// BETONG-mönster, och STÄLLVERKETs motor leker med det tillsammans med alla
// andra kanaler:
//  - varje spår i mönstret behandlas som en egen kanal (kick, hats, clap, perc,
//    bas, stab ...), så de kommer in en i taget i introt, försvinner i breaket och
//    byter i turordning i groovet
//  - motorn byter mellan mönstret (A) och ett grannmönster ur samma mapp (B) när
//    delarna byts, byter till kick 2/rumble i byggen och breaks, byter perc A och B,
//    och lägger fills i frasslut
// Ljudet läggs till det som redan går på kanalen, och går sedan genom ställverket
// som vanligt (filter, stutter, rum, pump ...).
//
// Ren C++ utan JUCE, så den kan testas fristående. Ljudfilerna läses av processorn.

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "KassettData.h"
#include "Motor.h"

namespace kassett
{
constexpr int kPads = 16, kSpar = 8, kMaxSteg = 32, kRoster = 32;

struct Ljud
{
    std::vector<float> L, R;
    double sr = 48000.0;
    std::string namn;
};
using LjudPtr = std::shared_ptr<const Ljud>;

struct Steg { bool pa = false; float styrka = 0.78f; uint8_t chans = 100, ratchet = 1; int8_t semi = 0; };
struct Spar
{
    int pad = 0, stegPerSlag = 4, langd = 16, rotEvery = 0;
    std::array<Steg, kMaxSteg> steg {};
    bool anvands = false;
};
struct Monster { std::array<Spar, kSpar> spar {}; float swing = 0.0f; };

// Vilken roll ett pad har för motorn (0-baserat pad)
inline int rollForPad (int pad)
{
    static const int r[kPads] = { motor::KICK, motor::BAS, motor::HATS, motor::RIDE, motor::CLAP, motor::CLAP,
                                  motor::PERC, motor::PERC, motor::PERC, motor::PERC, motor::PERC, motor::HATS,
                                  motor::BAS, motor::SYNT, motor::ATMO, motor::FX };
    return (pad >= 0 && pad < kPads) ? r[pad] : motor::PERC;
}

inline int stegPerSlagFor (const char* kod)
{
    if (std::strcmp (kod, "8") == 0) return 2;
    if (std::strcmp (kod, "16T") == 0) return 6;
    if (std::strcmp (kod, "32") == 0) return 8;
    if (std::strcmp (kod, "5") == 0) return 5;
    if (std::strcmp (kod, "7") == 0) return 7;
    return 4;
}

// "pad:takt:steg[:n0,0,12...][:r4]" eller "pad:takt:e:träffar:längd:rotation"
inline Monster lasMonster (int index)
{
    Monster m;
    if (index < 0 || index >= kNumPatterns) index = 0;
    const auto& pr = kPatterns[index];
    m.swing = pr.swing;
    for (int l = 0; l < kSpar; ++l)
    {
        const char* spec = pr.lanes[l];
        if (spec == nullptr) break;
        std::vector<std::string> delar;
        { std::string s (spec), cur; for (char c : s) { if (c == ':') { delar.push_back (cur); cur.clear(); } else cur += c; } delar.push_back (cur); }
        if (delar.size() < 3) continue;
        auto& sp = m.spar[(size_t) l];
        sp.anvands = true;
        sp.pad = std::clamp (std::atoi (delar[0].c_str()) - 1, 0, kPads - 1);
        sp.stegPerSlag = stegPerSlagFor (delar[1].c_str());
        size_t extra = 3;
        if (delar[2] == "e" && delar.size() >= 6)
        {
            const int traff = std::atoi (delar[3].c_str()), len = std::clamp (std::atoi (delar[4].c_str()), 1, kMaxSteg);
            const int rot = std::atoi (delar[5].c_str());
            sp.langd = len;
            for (int i = 0; i < len; ++i)
            {
                const int idx = ((i - rot) % len + len) % len;
                sp.steg[(size_t) i].pa = traff > 0 && ((idx * std::min (traff, len)) % len) < std::min (traff, len);
            }
            extra = 6;
        }
        else
        {
            const std::string& st = delar[2];
            sp.langd = std::clamp ((int) st.size(), 1, kMaxSteg);
            for (int i = 0; i < sp.langd; ++i)
            {
                const char c = st[(size_t) i];
                auto& s = sp.steg[(size_t) i];
                s.pa = c != '.';
                s.styrka = c == 'X' ? 1.0f : (c == 'g' ? 0.42f : 0.78f);
                s.chans = c == 'o' ? 50 : (c == 'p' ? 25 : 100);
                s.ratchet = (c >= '2' && c <= '4') ? (uint8_t) (c - '0') : 1;
            }
        }
        for (size_t k = extra; k < delar.size(); ++k)
        {
            const auto& x = delar[k];
            if (x.empty()) continue;
            if (x[0] == 'n')
            {
                int i = 0; std::string cur;
                for (size_t j = 1; j <= x.size(); ++j)
                {
                    if (j == x.size() || x[j] == ',') { if (i < kMaxSteg) sp.steg[(size_t) i].semi = (int8_t) std::clamp (std::atoi (cur.c_str()), -24, 24); ++i; cur.clear(); }
                    else cur += x[j];
                }
            }
            else if (x[0] == 'r') sp.rotEvery = std::clamp (std::atoi (x.c_str() + 1), 0, 16);
        }
    }
    return m;
}

// Grannmönstret ur samma mapp (B-mönstret), eller nästa mönster om mappen är slut.
inline int grannMonster (int index, int steg)
{
    if (index < 0 || index >= kNumPatterns) return 0;
    const char* grupp = kPatterns[index].group;
    int i = index;
    for (int n = 0; n < kNumPatterns; ++n)
    {
        i = (i + (steg >= 0 ? 1 : -1) + kNumPatterns) % kNumPatterns;
        const char* g = kPatterns[i].group;
        if (grupp == nullptr || g == nullptr || std::strcmp (g, grupp) == 0) return i;
    }
    return (index + 1) % kNumPatterns;
}

//==============================================================================
// Motorns lek med kassetten i en viss stund (bestäms av planen, så det är samma varje gång)
struct Lek
{
    bool b = false;         // spela B-mönstret
    bool kick2 = false;     // kickspåret spelar pad 2 (kick 2/rumble)
    bool percByt = false;   // perc A och B byter plats
    bool fill = false;      // fill i den här takten
    float tathet = 0.0f;    // extra ratchets på hats (uppbyggnad)
};

inline Lek lekFor (const motor::Plan& p, const motor::Installning& in, const motor::Overstyr& o,
                   int kanal, double slag, int spt, float lek)
{
    using namespace motor;
    Lek l;
    if (spt < 1) spt = 4;
    const int64_t takt = (int64_t) std::floor (slag / spt);
    int64_t tid = 0;
    const Sektion s = aktuell (p, o, takt, tid);
    const uint32_t h = blanda (s.h, (uint32_t) kanal, 0xCA55);
    const float t = (float) (((double) tid * spt + (slag - (double) takt * spt)) / ((double) s.langd * spt));
    switch (s.typ)
    {
        case GROOVE: case LYFT: case UTANDNING:
            l.b = slump01 (blanda (h, (uint32_t) (tid / 16))) < 0.5f * lek;   // kan byta var 16:e takt i långa grooves
            break;
        case DROP: case KLIMAX:
            l.b = slump01 (h + 2) < 0.35f + 0.4f * lek;
            break;
        case BYGG:
            l.kick2 = t < 0.5f && slump01 (h + 3) < lek;
            l.tathet = t > 0.5f ? (t - 0.5f) * 2.0f * lek : 0.0f;
            break;
        case BREAK:
            l.kick2 = in.brytDjup < 0.4f && slump01 (h + 4) < lek;
            break;
        default: break;
    }
    l.percByt = slump01 (blanda (h + 5, (uint32_t) (tid / 8))) < 0.4f * lek;
    const bool frasSlut = (tid % 8) == 7 && tid + 1 < s.langd;
    const bool sistaIBygg = s.typ == BYGG && tid >= s.langd - 2;
    l.fill = (frasSlut && slump01 (blanda (h + 6, (uint32_t) (tid / 8))) < lek * (0.3f + in.stok * 0.7f)) || (sistaIBygg && lek > 0.2f);
    return l;
}

//==============================================================================
class Kassett
{
public:
    void forbered (double sampleRate)
    {
        sr = sampleRate;
        for (auto& r : roster) r.aktiv = false;
        if (! syntGjord) { for (int p = 0; p < kPads; ++p) synt[(size_t) p] = syntetisera (p, 48000.0); syntGjord = true; }
    }

    // Från meddelandetråden: ett ljud per pad (nullptr = det syntade reservljudet)
    void sattLjud (int pad, LjudPtr l)
    {
        if (pad < 0 || pad >= kPads) return;
        std::atomic_store (&ljud[(size_t) pad], std::move (l));
    }
    void sattMonster (int index)
    {
        if (index == monsterIndex.load()) return;
        auto a = std::make_shared<Monster> (lasMonster (index));
        auto b = std::make_shared<Monster> (lasMonster (grannMonster (index, 1)));
        std::atomic_store (&monA, std::shared_ptr<const Monster> (a));
        std::atomic_store (&monB, std::shared_ptr<const Monster> (b));
        monsterIndex.store (index);
    }
    bool harRiktigaLjud (int pad) const { return std::atomic_load (&ljud[(size_t) pad]) != nullptr; }
    float padAktivitet (int pad) const { return padLampa[(size_t) pad].load(); }

    // Kör ett block. Ljudet läggs till i L och R.
    //  slag0 = position i slag vid första samplet. nivaFor (roll, slag, lp&) = motorns nivå för rollen.
    //  lekFn (slag) = motorns lek just då.
    template <typename NivaFn, typename LekFn>
    void kor (float* L, float* R, int n, double slag0, double slagPerSampel, bool spelar, float volym,
              NivaFn&& nivaFor, LekFn&& lekFn)
    {
        const auto a = std::atomic_load (&monA), b = std::atomic_load (&monB);
        if (spelar && a != nullptr && slagPerSampel > 0.0)
        {
            const double slag1 = slag0 + n * slagPerSampel;
            const Monster& mA = *a;
            const Monster& mB = b != nullptr ? *b : *a;
            for (int sp = 0; sp < kSpar; ++sp)
            {
                const Spar& sA = mA.spar[(size_t) sp];
                if (! sA.anvands) continue;
                const double steg = 1.0 / sA.stegPerSlag;
                const int64_t k0 = (int64_t) std::floor (slag0 / steg) - 1, k1 = (int64_t) std::ceil (slag1 / steg) + 1;
                for (int64_t k = k0; k <= k1; ++k)
                {
                    double tk = (double) k * steg;
                    const Lek lk = lekFn (tk);
                    const Monster& m = lk.b ? mB : mA;
                    const Spar& s = m.spar[(size_t) sp].anvands ? m.spar[(size_t) sp] : sA;
                    const double swing = m.swing;
                    if ((k & 1) && (s.stegPerSlag == 4 || s.stegPerSlag == 2)) tk += swing * 0.5 * steg;
                    const int64_t takt = (int64_t) std::floor (tk / 4.0);
                    int rot = 0;
                    if (s.rotEvery > 0) rot = (int) ((takt / s.rotEvery) % s.langd);
                    const int i = (int) ((((k + rot) % s.langd) + s.langd) % s.langd);
                    const Steg& st = s.steg[(size_t) i];
                    int ratchet = st.ratchet;
                    bool pa = st.pa;
                    int pad = s.pad;
                    const int roll = rollForPad (s.pad);
                    // leken: kick 2, perc A/B, fills och täthet
                    if (pad == 0 && lk.kick2) pad = 1;
                    if (lk.percByt && (pad == 8 || pad == 9)) pad = pad == 8 ? 9 : 8;
                    const int stegITakt = (int) ((((int64_t) std::floor (tk * s.stegPerSlag + 1e-6)) % (4 * s.stegPerSlag) + 4 * s.stegPerSlag) % (4 * s.stegPerSlag));
                    const bool sistaSlaget = stegITakt >= 3 * s.stegPerSlag;
                    if (lk.fill && sistaSlaget && (roll == motor::HATS || roll == motor::PERC || roll == motor::CLAP))
                    {
                        if (! pa && roll != motor::CLAP && (motor::blanda ((uint32_t) k, (uint32_t) sp) & 3) == 0) pa = true;
                        if (pa) ratchet = std::max (ratchet, roll == motor::CLAP ? 2 : 3);
                    }
                    if (lk.tathet > 0.3f && roll == motor::HATS && pa) ratchet = std::max (ratchet, lk.tathet > 0.75f ? 4 : 2);
                    if (! pa) continue;
                    if (st.chans < 100 && (motor::blanda ((uint32_t) k, (uint32_t) sp, 0xC4A5) % 100) >= st.chans) continue;
                    for (int r = 0; r < ratchet; ++r)
                    {
                        const double t = tk + r * steg / ratchet;
                        if (t < slag0 || t >= slag1) continue;
                        float lp = 1.0f;
                        const float niva = nivaFor (roll, t, lp);
                        if (niva < 0.01f) continue;
                        const float styrka = st.styrka * (r > 0 ? 0.75f : 1.0f);
                        starta (pad, (int) ((t - slag0) / slagPerSampel), styrka * niva, st.semi, lp);
                    }
                }
            }
        }
        rendera (L, R, n, volym);
    }

private:
    struct Rost
    {
        bool aktiv = false;
        int pad = 0, fordrojning = 0;
        double pos = 0, fart = 1;
        float gain = 1, lpK = 1, lpL = 0, lpR = 0, ut = 1.0f;
        bool tonarUt = false;
        LjudPtr ljud;
        uint32_t alder = 0;
    };

    void starta (int pad, int offset, float gain, int semi, float lp)
    {
        // stängd hat tystar öppen hat (choke), som på en trummaskin
        if (pad == 2) for (auto& r : roster) if (r.aktiv && r.pad == 3) r.tonarUt = true;
        LjudPtr l = std::atomic_load (&ljud[(size_t) pad]);
        if (l == nullptr) l = synt[(size_t) pad];
        if (l == nullptr || l->L.empty()) return;
        Rost* fri = nullptr;
        for (auto& r : roster) if (! r.aktiv) { fri = &r; break; }
        if (fri == nullptr)   // stjäl den äldsta
        {
            fri = &roster[0];
            for (auto& r : roster) if (r.alder < fri->alder) fri = &r;
        }
        Rost& r = *fri;
        r.aktiv = true; r.pad = pad; r.fordrojning = std::max (0, offset);
        r.pos = 0.0; r.fart = (l->sr / sr) * std::pow (2.0, semi / 12.0);
        r.gain = gain; r.ljud = std::move (l); r.tonarUt = false; r.ut = 1.0f;
        r.lpK = lp >= 0.999f ? 1.0f : (float) (1.0 - std::exp (-6.2832 * (60.0 * std::pow (2.0, lp * 8.4)) / sr));
        r.lpL = r.lpR = 0.0f;
        r.alder = ++raknare;
        padLampa[(size_t) pad].store (std::min (1.0f, gain));
        ++slagRaknare[(size_t) pad];
    }

public:
    // för test och panel: hur många slag varje pad har spelat
    std::array<uint32_t, kPads> slagRaknare {};
private:

    void rendera (float* L, float* R, int n, float volym)
    {
        const float utTon = (float) (1.0 / (0.005 * sr));
        for (auto& r : roster)
        {
            if (! r.aktiv) continue;
            const Ljud& l = *r.ljud;
            const int len = (int) l.L.size();
            const bool stereo = ! l.R.empty();
            const float g = r.gain * volym;
            for (int i = r.fordrojning; i < n; ++i)
            {
                const int i0 = (int) r.pos;
                if (i0 + 1 >= len || r.ut <= 0.0f) { r.aktiv = false; break; }
                const float fr = (float) (r.pos - i0);
                float xl = l.L[(size_t) i0] + (l.L[(size_t) i0 + 1] - l.L[(size_t) i0]) * fr;
                float xr = stereo ? l.R[(size_t) i0] + (l.R[(size_t) i0 + 1] - l.R[(size_t) i0]) * fr : xl;
                if (r.lpK < 1.0f) { r.lpL += r.lpK * (xl - r.lpL); r.lpR += r.lpK * (xr - r.lpR); xl = r.lpL; xr = r.lpR; }
                // kort intoning mot klick, och uttoning när en hat tystas
                const float in = r.pos < 32.0 ? (float) (r.pos / 32.0) : 1.0f;
                if (r.tonarUt) r.ut -= utTon;
                const float e = g * in * std::max (0.0f, r.ut);
                L[i] += xl * e; R[i] += xr * e;
                r.pos += r.fart;
            }
            r.fordrojning = 0;
        }
        for (auto& p : padLampa) p.store (p.load() * 0.85f);
    }

    // Reservljud när ljudbanken inte finns: enkla syntade trumljud per pad-roll
    static LjudPtr syntetisera (int pad, double s)
    {
        auto l = std::make_shared<Ljud>();
        l->sr = s;
        uint32_t brus = 0x1234567u + (uint32_t) pad * 7919u;
        auto rnd = [&brus] { brus = brus * 1664525u + 1013904223u; return (float) ((brus >> 8) & 0xFFFF) / 32768.0f - 1.0f; };
        auto skapa = [&] (double sek) { l->L.assign ((size_t) (sek * s), 0.0f); };
        float hp = 0, bp1 = 0, bp2 = 0;
        const double pi2 = 6.283185307;
        switch (rollForPad (pad))
        {
            case motor::KICK:
            {
                skapa (0.45);
                double fas = 0;
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    fas += pi2 * (45.0 + 110.0 * std::exp (-t * 35.0)) / s;
                    l->L[i] = (float) (std::tanh (1.8 * std::sin (fas)) * std::exp (-t * 7.0) * 0.9);
                }
                break;
            }
            case motor::HATS: case motor::RIDE:
            {
                const bool oppen = pad == 3;
                skapa (oppen ? 0.5 : 0.08);
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    const float x = rnd(); hp += 0.5f * (x - hp);
                    l->L[i] = (x - hp) * (float) std::exp (-t * (oppen ? 8.0 : 60.0)) * 0.5f;
                }
                break;
            }
            case motor::CLAP:
            {
                skapa (0.3);
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    const float x = rnd();
                    bp1 += 0.25f * (x - bp1); bp2 += 0.08f * (bp1 - bp2);
                    const double env = t < 0.03 ? std::exp (-std::fmod (t, 0.01) * 300.0) : std::exp (-(t - 0.03) * 22.0);
                    l->L[i] = (bp1 - bp2) * (float) env * 1.6f;
                }
                break;
            }
            case motor::BAS:
            {
                skapa (0.35);
                double fas = 0;
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    fas += pi2 * (pad == 1 ? 40.0 : 55.0) / s;
                    l->L[i] = (float) (std::tanh (2.5 * std::sin (fas)) * std::exp (-t * (pad == 1 ? 3.0 : 9.0)) * 0.55);
                }
                break;
            }
            case motor::SYNT: case motor::ATMO: case motor::FX:
            {
                skapa (pad == 14 ? 1.2 : 0.35);
                double f1 = 0, f2 = 0, f3 = 0;
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    f1 += pi2 * 220.0 / s; f2 += pi2 * 261.6 / s; f3 += pi2 * 329.6 / s;
                    const double x = std::sin (f1) + std::sin (f2) * 0.8 + std::sin (f3) * 0.6;
                    l->L[i] = (float) (x * std::exp (-t * (pad == 14 ? 2.0 : 10.0)) * 0.22);
                }
                break;
            }
            default:   // perc: kort, tonad knack
            {
                skapa (0.12);
                const double f = 300.0 + 90.0 * pad;
                for (size_t i = 0; i < l->L.size(); ++i)
                {
                    const double t = (double) i / s;
                    l->L[i] = (float) (std::sin (pi2 * f * t) * std::exp (-t * 40.0) * 0.5);
                }
                break;
            }
        }
        l->namn = "synt";
        return l;
    }

    double sr = 48000.0;
    std::array<LjudPtr, kPads> ljud {}, synt {};
    bool syntGjord = false;
    std::shared_ptr<const Monster> monA, monB;
    std::atomic<int> monsterIndex { -1 };
    std::array<Rost, kRoster> roster {};
    std::array<std::atomic<float>, kPads> padLampa {};
    uint32_t raknare = 0;
};
} // namespace kassett
