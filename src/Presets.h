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
    float mjuk = 0.65f, tryck = 0.3f;   // MJUKHET och TRYCK (äldre presets får standardvärdena)
};

// mall: 0 KLASSISK 1 MINIMAL 2 PEAK 3 HYPNOS 4 DOCKLANDS 5 STÖK 6 DUB 7 BERG-O-DAL
//       8 SPEGEL 9 STAPLA 10 FALSKT DROP 11 DUELL 12 TELEFON 13 SVÄRM 14 SKRIFTEN 15 PLASTIK 16 HOOD
//       17 STOCKHOLM 96 18 RUMÄNSK 19 PERLON 20 KANAL 21 MÖRKER   (motorns nummer, inte menyns)
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
    { "Spegelsalen",                8, 0, 0.60f,0.40f,0.60f,0.50f,0.30f,0.70f,0.40f,0.20f, 18 },
    { "Stapla lådor",               9, 0, 0.70f,0.20f,0.80f,0.40f,0.30f,0.40f,0.60f,0.10f, 24 },
    { "Lurad igen",                10, 0, 0.75f,0.50f,0.70f,0.50f,0.50f,0.60f,1.00f,0.20f, 32 },
    { "Duell i kulverten",         11, 0, 0.65f,0.40f,0.50f,0.50f,0.40f,0.50f,0.40f,0.40f, 43 },
    { "Ring mig",                  12, 0, 0.55f,0.40f,0.60f,0.60f,0.30f,0.70f,0.50f,0.40f, 46 },
    { "Bisvärmen",                 13, 0, 0.75f,0.80f,0.50f,0.30f,0.70f,0.50f,0.50f,0.20f, 53 },
    { "Enligt skriften",           14, 0, 0.60f,0.50f,0.60f,0.50f,0.40f,0.60f,0.50f,0.20f, 70 },
    { "Spastisk väntan",           15, 0, 0.60f,0.30f,0.60f,0.80f,0.30f,0.90f,0.50f,0.40f, 93 },
    { "Plastikmannen",             15, 0, 0.70f,0.40f,0.50f,0.60f,0.40f,0.80f,0.40f,0.50f, 98 },
    { "Minus en timme",            16, 4, 0.50f,0.10f,0.30f,0.50f,0.10f,0.60f,0.20f,0.20f, 94 },
    { "Hood i maskinhallen",       16, 0, 0.60f,0.20f,0.30f,0.40f,0.20f,0.50f,0.30f,0.10f, 13 },
    { "Dubkedjan",                  6, 0, 0.50f,0.50f,0.80f,0.60f,0.20f,0.70f,0.40f,1.00f, 27 },
    //  efter labelsound                mall form energi var  bryt bygg stök filt andas dub  frö   mjuk  tryck
    // STOCKHOLM 96: hårt, slagverk, loopar (tidiga Drumcode, Planet Rhythm, Svek)
    { "Planet Rytm",               17, 0, 0.85f,0.40f,0.45f,0.30f,0.75f,0.35f,0.50f,0.05f, 95, 0.45f,0.60f },
    { "Söder 03.00",               17, 0, 0.80f,0.55f,0.55f,0.40f,0.65f,0.45f,0.60f,0.10f, 303,0.55f,0.55f },
    { "Loopverkstan",              17, 0, 0.75f,0.30f,0.30f,0.30f,0.85f,0.30f,0.30f,0.00f, 46, 0.40f,0.65f },
    { "Hammarby hamn",             17, 3, 0.80f,0.45f,0.50f,0.35f,0.70f,0.40f,0.60f,0.05f, 97, 0.50f,0.60f },
    // RUMÄNSK: långt, hypnotiskt, tålamod (Rhadoo, Raresh, Petre Inspirescu)
    { "Bukarest soluppgång",       18, 0, 0.50f,0.35f,0.20f,0.40f,0.15f,0.95f,0.10f,0.30f, 7,  0.90f,0.20f },
    { "Tålamod",                   18, 0, 0.45f,0.25f,0.15f,0.50f,0.10f,0.90f,0.00f,0.20f, 71, 0.95f,0.15f },
    { "Mikrogung",                 18, 4, 0.55f,0.45f,0.25f,0.40f,0.25f,0.85f,0.10f,0.35f, 108,0.85f,0.25f },
    // PERLON: mikroförändringar, filter och dub (Villalobos, Zip)
    { "Frankfurt efterfest",       19, 0, 0.50f,0.60f,0.40f,0.40f,0.35f,0.85f,0.20f,0.50f, 69, 0.80f,0.25f },
    { "Glapp i gunget",            19, 0, 0.55f,0.70f,0.35f,0.35f,0.45f,0.80f,0.30f,0.45f, 113,0.75f,0.30f },
    { "Perlonsnöre",               19, 4, 0.45f,0.55f,0.45f,0.50f,0.30f,0.90f,0.20f,0.60f, 35, 0.85f,0.20f },
    // KANAL: dubtechno, ekon i stället för drop (Basic Channel, Chain Reaction)
    { "Kedjereaktion",             20, 0, 0.45f,0.30f,0.50f,0.60f,0.05f,0.85f,0.20f,1.00f, 4,  0.90f,0.25f },
    { "Dubkanalen",                20, 0, 0.40f,0.40f,0.60f,0.60f,0.05f,0.90f,0.30f,0.95f, 8,  0.90f,0.20f },
    { "Källaren på Hardwax",       20, 3, 0.50f,0.35f,0.55f,0.50f,0.10f,0.80f,0.20f,0.90f, 93, 0.85f,0.30f },
    // MÖRKER: spänning, muller, stora drop (Dubfire, SCI+TEC)
    { "Mörkrets tryck",            21, 0, 0.80f,0.45f,0.80f,0.85f,0.45f,0.75f,0.80f,0.25f, 21, 0.70f,0.65f },
    { "Muller i tunneln",          21, 0, 0.75f,0.40f,0.85f,0.90f,0.40f,0.80f,0.70f,0.30f, 77, 0.75f,0.60f },
    { "Revben",                    21, 3, 0.85f,0.50f,0.75f,0.80f,0.50f,0.70f,0.90f,0.20f, 2007,0.65f,0.70f },
};
inline constexpr int kAntal = (int) (sizeof (kFabrik) / sizeof (kFabrik[0]));
} // namespace presets
