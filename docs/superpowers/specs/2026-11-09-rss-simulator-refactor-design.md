# Návrh: refaktor RSS simulátora

Vetva: `refactor/rss-dma-simulator`
Stav: odsúhlasený autorom (2026-11-09)

## 1. Prečo

Projekt rástol postupne. Metodika sa viackrát menila (jednoduché tuple → flowy → váhy →
RETA → JSQ) a každá zmena sa „prilepila“ k existujúcemu kódu. Dnešný stav:

- parametre sú na štyroch miestach: `#define` v `config.hpp` (RETA_ON, IP_ONLY_ON),
  konštanty (okno, kľúče), argumenty programu (symetria, váha, `jsq`) a cykly v `run.sh`;
  zmena IP only alebo RETA vyžaduje úpravu kódu a novú kompiláciu,
- výsledné CSV nehovorí, s akými nastaveniami vzniklo,
- tok programu skáče cez lambdy a callbacky (`prepare` / `sink` / `release`),
- `runFlowModeRetaOn` a `runFlowModeRetaOff` sú takmer rovnaký kód,
- autor sa v kóde stratil a nedokáže ho sám rozširovať.

Zadanie bakalárky (bod 4) žiada **navrhnúť varianty úprav RSS jednotky**. Výsledkom práce
je teda RSS modul, ktorý dobre rozdeľuje prevádzku. Z doterajších meraní vieme, že voľba
hash funkcie rozhoduje len o jednotky percent, kým lepší spôsob výberu kanála môže
preťaženie znížiť o 64–90 % (JSQ orákulum). Ťažisko ďalšej práce preto nebude hash, ale
to, čo sa deje **po** ňom.

## 2. Ciele

1. **Oddeliť simulačný engine od RSS modulu.**
   - Engine je stabilný základ (čítanie flowov, pakety v čase, okná, metriky, výstup).
     Bežne sa doň nezasahuje.
   - RSS modul je jediné miesto, ktoré rozhoduje, kam flow alebo paket pôjde.
     Tu sa bude robiť výskum.
2. **RSS ako reťaz pomenovaných krokov** (podľa autorovho náčrtu):

   ```
   flow ─► tuple (IP only / plný) ─► symetria ─► hash ─► úprava hashu ─► index 0–127 ─► priradenie na kanál
                                                         (nový krok)                    (RETA / modulo / …)
   ```

   Každý krok je samostatná časť kódu. Variantu vytvoríš výmenou jedného kroku,
   ostatné zostanú. Hash ostane vymeniteľný (6 hashov), predvolene **Toeplitz**.
   JSQ orákulum stojí mimo reťaze: preskakuje hash aj RETA a kanál volí priamo.
   Slúži ako referenčná dolná hranica.
3. **Všetky parametre za behu, v súbore experimentu.** Žiadne `#define` ani nová
   kompilácia kvôli nastaveniu.
4. **Každý riadok výsledkov nesie svoje parametre** (dataset, tuple, symetria, hash, kľúč,
   RETA, metóda, váha), takže o každom čísle vieš, odkiaľ pochádza.
5. **Jednoduché C++:** štruktúry, `std::vector`, obyčajné `for` cykly a `switch`.
   Žiadne lambdy, `std::function`, dedičnosť ani šablóny navyše. Radšej viac riadkov,
   ktoré sa dajú čítať zhora nadol.
6. **Rovnaké čísla ako dnes** (presné znenie v časti 6).

## 3. Čo NEROBÍME (v tejto fáze)

- Nemeníme metodiku ani výsledky. Refaktor je čisté upratanie.
- Nepridávame nové RSS varianty (skupiny susedných kanálov, úpravu hashu, výber časti
  flowov podľa záťaže na kanál). To príde vo vetve `rss-distribution-variants`.
  Návrh však ukazuje, kam sa zapoja (časť 7).
- Nemeníme implementácie hashov v `external/` ani ich obálky v `src/wrappers/`,
  iba ich registráciu.
- Neprerábame grafy. `analyze.py` sa iba upraví, aby čítal nový formát výsledkov.
  Nové grafy podľa autorovho náčrtu budú samostatný krok.

**Neúspech napriek tomu, že to „funguje“:** čísla sa zmenia a nevieme prečo; kód, ktorému
autor nerozumie; viac skrytých prepínačov než dnes.

## 4. Obmedzenia

- **Výkon:** plný `odid1.csv` má ~2,3 mld. paketov a naraz ~782 tisíc flowov.
  Pri rovnakom počte simulovaných konfigurácií nesmie byť nový kód pomalší o viac než ~20 %.
  Preto zostáva RETA trik (rozhodnutie autora, časť 5.6).
- **Pamäť:** Mac so 16 GB RAM, server podobne.
- **Prostredie:** C++17, `Makefile`, macOS (clang) aj Linux server (g++). Python 3 + pandas
  a matplotlib vo `venv`.
- **Dáta:** súbory `data/odid*.csv` otvára iba autor. Testy bežia na syntetickom
  `data/flow_big.csv` a na malom testovacom súbore v repozitári.
- **Na rovnaké čísla treba zachovať:** generovanie kľúčov (`mt19937`, seed 65536,
  16 kľúčov × 40 B), RETA tabuľku `i % n`, hranice okien (počet paketov), poradie volaní
  pri štarte flowov (fwd pred rev), formát čísel v CSV a spôsob, akým simpletab odvodzuje
  index kľúča (dnes z adresy kľúča v poli `keys`).

## 5. Ako to bude vyzerať

### 5.1 Súbory

```
src/
  main.cpp                 načíta experiment, spustí behy
  experiment.hpp/.cpp      struct Experiment + načítanie zo súboru .conf
  engine/
    flow_reader.hpp/.cpp   CSV flowov → udalosti v čase (halda), bez callbackov
    window_counter.hpp/.cpp okná, histogramy, výpočet metrík
    metrics.hpp/.cpp       over-threshold, Jain, chi (bez zmeny výpočtu)
    results_writer.hpp/.cpp CSV so všetkými parametrami
  rss/
    tuple.hpp/.cpp         plný / IP only tuple, symetria none / xor / sort
    hashes.hpp/.cpp        zoznam hashov a kľúče
    digest.hpp/.cpp        úprava hashu pred RETA (zatiaľ iba „bez úpravy“)
    assign.hpp/.cpp        priradenie na kanál: RETA (i % n), modulo, JSQ orákulum
    rss_unit.hpp/.cpp      spája kroky; jediné rozhranie, ktoré volá engine
  wrappers/  external/     bez zmeny
experiments/
  hash_comparison.conf     dnešné porovnanie hashov
  ip_only.conf, full_tuple.conf, jsq.conf, …
tests/
  data/                    malý syntetický dataset
  golden/                  referenčné výsledky zo starého kódu
  compare.py               porovná nové výsledky s referenciou
docs/
  ako_to_funguje.md        jedna strana: tok programu, kde je čo
```

Odstráni sa: režim jednoduchých tuple (3/5-stĺpcové CSV) a `runFlowModeRetaOff` ako samostatná
funkcia (RETA zap./vyp. bude krok „priradenie na kanál“). Zostane dostupné v histórii gitu
a v tagu `v1.0-hash-comparison`.

**Z `external/` ani zo `src/wrappers/` sa nič neodstraňuje** (rozhodnutie autora). Všetky hashe,
ktoré majú obálku (toeplitz, jhash, chaskey, halfsiphash, simpletab, multiplyshift, siphash,
xorhash), budú v zozname hashov a dajú sa vybrať v experimente. Predvolený je Toeplitz.
`external/crc32c` nemá obálku a zostane tak, ako je.

### 5.2 Experiment

Experiment je textový súbor `kľúč = hodnota`. Príklad:

```
# experiments/hash_comparison.conf
data       = data/odid1.csv
out        = results/hash_comparison

# porovnávané parametre: vyberáš si, čo porovnať
tuple      = full                # full | ip   (môže byť aj oboje: full ip)
hash       = toeplitz jhash chaskey halfsiphash simpletab
reta       = on                  # on | off
method     = hash                # hash | jsq

# statické parametre: v každom behu vždy všetky
symmetry   = none xor sort
weighting  = flow packet byte
channels   = 8 16 20 32 40 64 128
keys       = 16
window     = 150000
```

Spustenie: `./run.sh experiments/hash_comparison.conf`. Výstupný adresár dostane aj kópiu
`.conf`, logy a grafy.

### 5.3 Jeden beh a „svety“

- **Jeden prechod datasetom = jedna váha.** Pakety sa generujú podľa váhy (pri `flow`
  je to jeden „paket“ na smer flowu), preto má každá váha vlastný prechod.
  Experiment s tromi váhami = tri prechody. Môžu bežať paralelne.
- **Svet** je jedna kombinácia ostatných parametrov RSS: tuple × symetria × hash × kľúč ×
  RETA, plus JSQ ako jeden svet bez kľúčov. Všetky svety sa simulujú v jednom prechode
  naraz, rovnako ako dnes 5 hashov × 16 kľúčov. JSQ ignoruje tuple aj symetriu, preto
  vzniká iba raz.
- **Cena rastie s počtom svetov.** Predvolený experiment (Toeplitz, 16 kľúčov, 3 symetrie)
  má 48 svetov, menej než dnešných 80. Kto zapne všetko so všetkým, počíta dlho. To je
  zámer: experiment má byť cielené porovnanie.

### 5.4 Hlavná slučka enginu (bez lambd)

```cpp
FlowReader reader(experiment.data, weighting);
Event e;
while (reader.next(e)) {
    if (e.type == EVENT_FLOW_START) rss.flow_start(e.flow, e.info, counter);   // RSS rozhodne
    if (e.type == EVENT_PACKET)     counter.add_packet(e.flow, e.weight, rss); // engine započíta
    if (e.type == EVENT_FLOW_END)   rss.flow_end(e.flow);
    if (counter.window_full())      writer.write_window(counter);               // metriky → CSV
}
```

`FlowReader` vnútri robí to isté čo dnešný `expandInterleaved` (halda, pakety rovnomerne
v čase), len udalosti **vracia**, namiesto aby volal callbacky. Poradie udalostí musí byť
rovnaké ako dnes.

### 5.5 Rozhranie RSS modulu

```cpp
struct FlowInfo {          // čo RSS vie o flowe pri jeho štarte
    Endpoints ep;          // surové polia smeru: src/dst IP (16 B), src/dst port, protokol;
                           // tuple (plný alebo IP only) z nich postaví až RSS, pre každý svet
    double  t_start, t_end;
    int64_t packets;       // koľko paketov tento smer odošle
    int64_t weight;        // váha jedného paketu
};

class RssUnit {
public:
    // Raz pri štarte smeru flowu: pre každý svet a každý počet kanálov zvolí kanál.
    // Histogramy (aktuálna záťaž kanálov v okne) dostane iba na čítanie.
    void flow_start(int flow, const FlowInfo &info, const WindowCounter &loads);

    // Kanál, na ktorý ide paket tohto flowu v danom svete a pri danom počte kanálov.
    int  channel(int flow, int world, int channel_count_index) const;

    void flow_end(int flow);
};
```

- **Engine počíta pakety**, RSS iba rozhoduje. Nová varianta tak nemôže omylom pokaziť
  meranie.
- **Rozhodnutie sa urobí raz pri štarte flowu** a uloží sa (rovnako ako dnes „skrinky“
  `flow_slots`). Pri pakete sa iba prečíta.
- Varianty sa vyberajú cez `enum` a `switch` (napr. `ASSIGN_RETA`, `ASSIGN_MODULO`,
  `ASSIGN_JSQ`). Nová varianta = nová hodnota v `enum` a nová vetva `case`.
  Žiadna dedičnosť.

### 5.6 Počítanie paketov: dva druhy svetov (rozhodnutie autora: ponechať RETA trik)

**RETA svety** (hash s RETA): toto je hlavný a najrýchlejší prípad.

```
pri štarte flowu:   hash → úprava hashu → index 0–127   (uloží sa do skrinky flowu)
pri každom pakete:  buckets[svet][index] += váha         (128 počítadiel na svet)
na konci okna:      kanál[RETA[c][i]] += buckets[svet][i] pre každý počet kanálov c
                    → metriky → CSV → vynulovať buckets
```

- Engine počíta iba do 128 RETA položiek. **Priradenie položiek na kanály je samostatný krok**:
  tabuľka `RETA[c][i]`, ktorú engine použije pri skladaní na konci okna. Dnes je to
  `i % n`. Iná tabuľka = iné priradenie, hash a počítanie sa nemenia.
- Tabuľka sa môže meniť **medzi oknami** (napr. preusporiadanie RETA podľa záťaže
  predošlého okna). Zmena uprostred okna by sa skladaním nedala zachytiť.
- Na paket pripadá jeden zápis na svet, rovnako rýchlo ako dnes.

**Priame svety** (modulo bez RETA, JSQ, neskôr skupiny kanálov so striedaním paketov):

```
pri štarte flowu:   zvolí sa rovno kanál pre každý počet kanálov  (uloží sa do skrinky)
pri každom pakete:  histogram[svet][c][kanál] += váha    (7 zápisov na svet)
na konci okna:      metriky → CSV → vynulovať histogramy
```

Oba druhy svetov môžu bežať v tom istom prechode. Engine má na každý druh jednu krátku,
samostatne pomenovanú funkciu, takže je vidno, ktorá cesta sa kedy použije.

## 6. Ako overíme, že sa nič nepokazilo

1. **Referenčné výsledky:** starý kód z commitu `17b5145` (začiatok tejto vetvy) sa spustí
   na malom syntetickom datasete pre všetky dnešné kombinácie: hashe × kľúče × symetrie ×
   váhy, RETA zap. aj vyp., plný tuple aj IP only, a JSQ. Kombinácie s `RETA_ON`,
   `IP_ONLY_ON` a menším oknom sa získajú prekompilovaním kópie starého kódu so zmenenými
   `#define`. Okno sa zmenší, aby vzniklo dosť okien aj pri váhe `flow`.
   Výsledky sa uložia do `tests/golden/`.
2. **Porovnanie:** `tests/compare.py` spojí nové a referenčné výsledky podľa (hash, symetria,
   tuple, RETA, metóda, váha, okno, kľúč, počet kanálov) a požaduje **presne rovnaké hodnoty**
   všetkých troch metrík, ako text v CSV. Nový kód preto zapisuje čísla rovnakým spôsobom ako
   dnes (štandardný `ostream`, 6 platných číslic). Nové CSV má viac stĺpcov, porovnávajú sa
   spoločné. JSQ sa porovnáva so starým behom so symetriou `none`; v novom CSV má
   symetriu `none`, keďže na ňu nereaguje.
3. Porovnanie sa spustí **po každom kroku** refaktoru. Ak sa čo i len jedno číslo líši,
   vieme, ktorý krok to spôsobil.
4. Na záver jeden plný beh na `odid1.csv` (spustí autor) a porovnanie s
   `super_results/` z 2. 11. 2026.

## 7. Kam sa zapoja budúce varianty (iba ukážka, neimplementuje sa)

- **Úprava hashu** → krok `digest`, nová hodnota v `enum`.
- **Iné rozdelenie RETA, RETA prepisovaná podľa záťaže** → tabuľka `RETA[c][i]` v kroku
  „priradenie na kanál“ (zmena vždy na hranici okna, časť 5.6).
- **Skupiny susedných kanálov** → priradenie vráti skupinu (prvý kanál + veľkosť) a pakety
  flowu sa v nej striedajú. Engine na to bude mať pri flowe počítadlo paketov.
- **Záťaž na kanál úmerná počtu kanálov** (pripomienka vedúceho) → parameter experimentu
  pre výber časti flowov a okno úmerné počtu kanálov; patrí do `FlowReader`
  a `WindowCounter`, nie do RSS. Referenčnú hodnotu záťaže na kanál dodá autor.

## 8. Postup (podrobne bude v pláne)

Po každom kroku: porovnanie s referenciou + autor si prejde a odsúhlasí, čo vzniklo.

0. Referenčné výsledky a `compare.py`.
1. `Experiment` a súbor `.conf`, parametre za behu namiesto `#define`.
2. `FlowReader` bez callbackov.
3. RSS modul: tuple, hashe, priradenie (RETA / modulo / JSQ). Zlúčenie RetaOn/RetaOff.
4. Nový formát výsledkov (jedno CSV na prechod, teda na váhu, so všetkými svetmi),
   `run.sh`, úprava `analyze.py` tak, aby všetky dnešné grafy fungovali ďalej.
5. `docs/ako_to_funguje.md`.

## 9. Rozhodnutia

Autor:
- Ponechať RETA trik (počítanie do 128 položiek, skladanie na konci okna). Priradenie
  položiek na kanály je samostatný krok (časť 5.6).
- Ťažisko práce nie je hash, ale kroky po ňom. Predvolený hash je Toeplitz.
- Statické parametre (všetky v každom behu): počty kanálov, 16 kľúčov, symetrie, váhy, okno.
- Porovnávané parametre: dataset, hash, RETA zap./vyp., IP only zap./vyp., varianta RSS.
- Režim jednoduchých tuple sa ruší.
- Z `external/` a `src/wrappers/` sa nič nemaže; všetky hashe s obálkou sú dostupné v experimente.
- Refaktor píše Claude po malých častiach, autor každú prejde a odsúhlasí.

Claude (autor ich môže zmeniť):

- Varianty cez `enum` + `switch`, nie dedičnosť (jednoduchšie na čítanie).
- Jeden prechod = jedna váha; svety = kombinácie ostatných parametrov.
- Formát experimentu: vlastný jednoduchý `kľúč = hodnota`, bez knižnice.
- Paralelné prechody (napr. viac váh naraz) rieši buď `run.sh`, alebo program cez vlákna,
  podľa toho, čo bude jednoduchšie na čítanie.
- Hranica výkonu: najviac ~20 % pomalšie pri rovnakom počte svetov.
