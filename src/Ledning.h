#pragma once

// STÄLLVERKET · ledningen mellan instanserna
//
// Alla STÄLLVERKET i samma projekt delar det här minnet (de laddas av samma
// plugin-fil i samma process). CENTRAL skriver inställningarna hit, varje KANAL
// läser dem och rapporterar tillbaka sin roll och nivå så att CENTRAL kan visa dem.
//
// Bitwig: låt alla STÄLLVERKET ligga i samma process. Inställningar > Plug-ins >
// Plug-in hosting mode: "Together" eller "By manufacturer" (inte "Individually").
// Kör de ändå var för sig fungerar varje KANAL vidare på de senaste
// inställningarna den fick (de sparas i projektet).

#include <atomic>
#include <cstdint>

namespace ledning
{
inline constexpr int kKanaler = 16;
inline constexpr int kInst = 32;     // antal värden som skickas från CENTRAL

struct Kanal
{
    std::atomic<int>      roll { -1 };
    std::atomic<float>    niva { 0.0f };
    std::atomic<int64_t>  senast { 0 };      // ms, när kanalen senast hördes av
    std::atomic<int>      del { -1 };
    std::atomic<int64_t>  ljud { 0 };        // ms, när det senast kom ljud in på kanalen
};

struct Nav
{
    std::atomic<int64_t>  centralSenast { 0 };   // ms
    std::atomic<uint32_t> centralId { 0 };
    std::atomic<uint32_t> version { 0 };
    std::atomic<float>    inst[kInst];

    // överstyrning från CENTRAL
    std::atomic<int>      ovTyp { -1 };
    std::atomic<int64_t>  ovStart { 0 };
    std::atomic<int>      ovLangd { 0 };
    std::atomic<bool>     hall { false };
    std::atomic<int64_t>  hallStart { 0 };
    std::atomic<int>      hallLangd { 0 };
    std::atomic<int>      hallTyp { 0 };
    std::atomic<uint32_t> hallH { 0 };

    // var CENTRAL är i låten (för KANAL-displayen när den inte spelar själv)
    std::atomic<double>   slag { 0.0 };

    Kanal kanal[kKanaler];

    Nav() { for (auto& v : inst) v.store (0.0f); }
};

inline Nav& nav()
{
    static Nav n;
    return n;
}

inline bool levande (int64_t senast, int64_t nu, int64_t grans = 1500) { return senast > 0 && nu - senast < grans; }
} // namespace ledning
