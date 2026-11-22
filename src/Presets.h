#pragma once

// STÄLLVERKET · fabrikspresets (motor-presets)
//
// Ett preset ställer bara in motorn: hur låten byggs. Ljudet kommer från dina
// egna kanaler. Egna presets sparas som .stallverket-filer (vanlig XML) i
//   Dokument/Phunkshunz/STALLVERKET/Presets
// Lägg en mapp med .stallverket-filer där, så dyker de upp i listan (preset-pack).

namespace presets
{
struct Fabrik
{
    const char* namn;
    int mall, form;
    float energi, variation, brytDjup, byggLangd, stok, filter, andas, dub;
    int fro;
};

// mall: 0 KLASSISK 1 MINIMAL 2 PEAK 3 HYPNOS 4 DOCKLANDS 5 STÖK 6 DUB 7 BERG-O-DAL
// form: 0 EVIG 1 64 2 128 3 192 4 256
inline constexpr Fabrik kFabrik[] =
{
    //  namn                       mall form energi var  bryt bygg stök filt andas dub  frö
    { "Grundkoppling",              0, 0, 0.55f,0.50f,0.60f,0.50f,0.35f,0.60f,0.40f,0.20f,  1 },
    { "Båtvarvet 96",               4, 0, 0.75f,0.45f,0.50f,0.30f,0.70f,0.40f,0.60f,0.05f, 96 },
    { "Fuktigt golv",               1, 0, 0.45f,0.60f,0.40f,0.50f,0.20f,0.85f,0.20f,0.30f, 12 },
    { "Hypnos i timmar",            3, 0, 0.50f,0.35f,0.20f,0.40f,0.10f,1.00f,0.10f,0.20f,  7 },
    { "Topptid",                    2, 0, 0.90f,0.55f,0.70f,0.45f,0.60f,0.55f,0.70f,0.10f, 33 },
    { "Stök i ställverket",         5, 0, 0.70f,0.80f,0.55f,0.30f,0.95f,0.60f,0.50f,0.25f, 66 },
    { "Dubkällaren",                6, 0, 0.40f,0.50f,0.70f,0.70f,0.15f,0.70f,0.30f,0.90f,  5 },
    { "Berg och dal",               7, 0, 0.65f,0.50f,0.90f,0.80f,0.45f,0.90f,0.80f,0.30f, 21 },
    { "Kort sak 64",                0, 1, 0.60f,0.40f,0.60f,0.40f,0.40f,0.60f,0.50f,0.20f,  3 },
    { "Radioklipp 128",             2, 2, 0.80f,0.40f,0.60f,0.50f,0.50f,0.50f,0.60f,0.10f, 14 },
    { "Klubbversion 192",           0, 3, 0.65f,0.50f,0.65f,0.60f,0.40f,0.65f,0.50f,0.20f, 19 },
    { "Lång resa 256",              3, 4, 0.55f,0.45f,0.40f,0.70f,0.20f,0.90f,0.30f,0.30f, 41 },
    { "Hårda klipp",                4, 0, 0.85f,0.30f,0.40f,0.20f,0.50f,0.20f,0.70f,0.00f, 88 },
    { "Mjuka svep",                 3, 0, 0.45f,0.40f,0.50f,0.80f,0.05f,1.00f,0.10f,0.20f,  9 },
    { "Andas innan",                0, 0, 0.70f,0.50f,0.70f,0.50f,0.30f,0.60f,1.00f,0.10f, 27 },
    { "Lång uppbyggnad",            7, 0, 0.60f,0.40f,0.80f,1.00f,0.40f,0.80f,0.60f,0.20f, 55 },
    { "Rullande trummor",           5, 0, 0.80f,0.60f,0.40f,0.40f,1.00f,0.40f,0.30f,0.00f, 72 },
    { "Ekon i tunneln",             6, 0, 0.50f,0.60f,0.60f,0.50f,0.30f,0.60f,0.40f,1.00f, 13 },
    { "Natriumljus",                1, 0, 0.55f,0.70f,0.50f,0.50f,0.30f,0.70f,0.30f,0.40f, 58 },
    { "Betongpelare",               0, 0, 0.75f,0.30f,0.50f,0.40f,0.30f,0.40f,0.50f,0.10f, 61 },
    { "Kompressorn startar",        2, 0, 0.95f,0.50f,0.30f,0.30f,0.70f,0.50f,0.80f,0.05f, 44 },
    { "Nattskift",                  1, 0, 0.35f,0.80f,0.60f,0.50f,0.20f,0.80f,0.20f,0.50f, 23 },
    { "Gruskross",                  4, 0, 0.85f,0.70f,0.60f,0.30f,0.80f,0.30f,0.60f,0.10f, 77 },
    { "Tomrummet",                  1, 0, 0.30f,0.90f,0.80f,0.60f,0.10f,0.90f,0.30f,0.60f, 31 },
    { "Låst groove",                3, 0, 0.60f,0.10f,0.10f,0.50f,0.10f,0.70f,0.00f,0.10f,  2 },
    { "Allt eller inget",           7, 0, 0.80f,0.40f,1.00f,0.60f,0.50f,0.70f,1.00f,0.10f, 90 },
    { "Fredag 15:30",               5, 0, 0.65f,1.00f,0.70f,0.50f,0.90f,0.80f,0.70f,0.50f, 15 },
    { "Kat 1-tekniker",             0, 0, 0.50f,0.25f,0.50f,0.50f,0.15f,0.50f,0.30f,0.10f,  4 },
    { "Ventilen hackar",            5, 0, 0.75f,0.70f,0.50f,0.40f,0.85f,0.50f,0.40f,0.20f, 47 },
    { "Köldbärare",                 6, 0, 0.45f,0.40f,0.70f,0.60f,0.20f,0.80f,0.40f,0.70f, 38 },
    { "Pumpande klimax",            2, 0, 1.00f,0.60f,0.40f,0.40f,0.60f,0.40f,0.60f,0.10f, 99 },
    { "Minimal timme",              1, 4, 0.45f,0.55f,0.40f,0.50f,0.20f,0.80f,0.20f,0.30f, 17 },
    { "Peak 128",                   2, 2, 0.95f,0.50f,0.60f,0.40f,0.60f,0.50f,0.70f,0.10f, 51 },
    { "Dub 192",                    6, 3, 0.45f,0.50f,0.70f,0.60f,0.20f,0.70f,0.30f,0.90f, 29 },
    { "Docklands 192",              4, 3, 0.80f,0.50f,0.50f,0.30f,0.70f,0.40f,0.60f,0.05f, 93 },
    { "Djupt break",                0, 0, 0.55f,0.50f,1.00f,0.70f,0.30f,0.80f,0.60f,0.40f, 36 },
    { "Inga breaks",                3, 0, 0.75f,0.50f,0.00f,0.30f,0.30f,0.60f,0.00f,0.10f, 64 },
    { "Ständig förändring",         0, 0, 0.60f,1.00f,0.60f,0.50f,0.50f,0.70f,0.50f,0.30f, 81 },
    { "Stilla vatten",              6, 0, 0.30f,0.30f,0.50f,0.80f,0.05f,0.90f,0.20f,0.60f, 10 },
    { "Ställverket brinner",        5, 0, 1.00f,0.90f,0.50f,0.30f,1.00f,0.50f,0.90f,0.30f,666 },
};
inline constexpr int kAntal = (int) (sizeof (kFabrik) / sizeof (kFabrik[0]));
} // namespace presets
