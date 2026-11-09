# Refaktor RSS simulátora – plán implementácie

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Cieľ:** Prestavať simulátor na stabilný engine + samostatný RSS modul s parametrami
v súbore experimentu, pri presne rovnakých výsledkoch ako dnes.

**Architektúra:** Najprv referenčné výsledky starého kódu. Potom postupné kroky, po každom
z nich sa výstup porovná s referenciou: parametre za behu → čítanie flowov bez callbackov →
RSS modul (tuple, hashe, priradenie) → svety a nový formát CSV → JSQ ako svet →
`run.sh` a `analyze.py` → dokumentácia. Do kroku 4 vrátane program zapisuje starý formát
CSV a porovnáva sa bajt po bajte (`diff -r`). Od kroku 5 je nový formát a porovnáva
`tests/compare.py`.

**Technológie:** C++17, `Makefile` (clang na macOS, g++ na Linuxe), Python 3 + pandas
+ matplotlib vo `venv`, bash.

**Spec:** `docs/superpowers/specs/2026-11-09-rss-simulator-refactor-design.md`

## Global Constraints

- Čisté upratanie: výsledky musia byť **presne rovnaké** ako z commitu `17b5145`, všetky tri metriky ako text v CSV (štandardný `ostream`, 6 platných číslic).
- C++17, `Makefile`, musí sa kompilovať clangom (macOS) aj g++ (Linux).
- Jednoduché C++: štruktúry, `std::vector`, obyčajné `for` cykly, `enum` + `switch`. **Žiadne** lambdy, `std::function`, dedičnosť ani šablóny navyše.
- RETA trik zostáva: počíta sa do 128 položiek na svet, skladanie `RETA[c][i]` na konci okna (spec 5.6).
- Z `external/` ani zo `src/wrappers/` sa nič nemaže. Všetky hashe s obálkou sú dostupné, predvolený je `toeplitz`.
- Výkon: najviac ~20 % pomalšie než dnes pri rovnakom počte svetov.
- Claude **nikdy neotvára** obsah `data/odid*.csv`. Testy používajú `tests/data/flows_small.csv` a `data/flow_big.csv`.
- Na rovnaké čísla treba zachovať: kľúče `mt19937(65536)`, 40 B; RETA `i % n`; hranice okien podľa počtu paketov; poradie fwd pred rev; simpletab odvodzuje index kľúča z adresy v poli kľúčov (pole sa nesmie kopírovať ani presúvať).
- Po každej úlohe: krátke vysvetlenie pre autora (po slovensky, čo kde vzniklo a prečo) a **autorovo odsúhlasenie pred ďalšou úlohou**.
- Komentáre v kóde po anglicky, dokumentácia pre autora po slovensky.

## Review Focus

1. **Chyby v súbore experimentu** (neznámy kľúč, neznámy hash, `channels` > 128, prázdny zoznam, nečíselné `window`) → zrozumiteľná chyba s číslom riadku, kód 1, nič sa nepočíta. *Test: úloha 1.*
2. **Okrajové riadky v dátach** (rev bez paketov, `flowend` < `flowstart`, poškodený riadok, flow s 1 paketom, viac paketov s nulovým trvaním) → rovnaké správanie ako dnes. *Test: sú v `flows_small.csv`, pokrýva ich referencia (úloha 0).*
3. **Okno väčšie než počet paketov** → CSV iba s hlavičkou, `analyze.py` nespadne. *Test: úloha 7.*
4. **Hashe mimo referencie** (`multiplyshift`, `siphash`, `xorhash`) → beh nespadne a dá správny počet riadkov. *Test: úloha 4.*
5. **Veľké dáta** (~782 tisíc súbežných flowov, 2,3 mld. paketov) → pamäť a čas v limite. *Test: meranie výkonu v úlohe 6, plný beh autora v úlohe 8.*

---

### Task 0: Testovacie dáta, referenčné výsledky a porovnávač

**Files:**
- Create: `tests/make_test_data.py`, `tests/data/flows_small.csv` (vygenerovaný, commitnúť cez `git add -f`)
- Create: `tests/make_golden.sh`, `tests/compare.py`, `tests/test.conf`
- Modify: `.gitignore` (pridať `tests/out/`, `tests/golden/`)

**Interfaces:**
- Produces: `tests/golden/<reta>_<tuple>_<sym>_<weighting>.csv`, kde `<reta>` ∈ {`retaon`,`retaoff`}, `<tuple>` ∈ {`full`,`ip`}, `<sym>` ∈ {`none`,`xorfold`,`sortfold`}, `<weighting>` ∈ {`flow`,`packet`,`byte`}; a `tests/golden/jsq_<weighting>.csv`. Spolu 36 + 3 súbory v **starom** formáte CSV.
- Produces: `python3 tests/compare.py <referenčný_adresár> <nový_adresár>`, kód 0 = všetko rovnaké, 1 = rozdiel alebo chýbajúce riadky.
- Produces: `tests/test.conf` (formát z úlohy 1, zatiaľ ho číta iba človek):
  ```
  data      = tests/data/flows_small.csv
  out       = tests/out
  tuple     = full ip
  hash      = toeplitz jhash chaskey halfsiphash simpletab
  reta      = on off
  method    = hash jsq
  symmetry  = none xor sort
  weighting = flow packet byte
  channels  = 8 16 20 32 40 64 128
  keys      = 16
  window    = 500
  ```

- [ ] **Step 1: `tests/make_test_data.py`** — zo `data/flow_big.csv` zoberie riadky 1–1500 (bez hlavičky) a zmenší ich: `packets_new = max(1, packets // 100)`, `bytes_new = bytes * packets_new // packets` (pre oba smery; ak má smer 0 paketov, ostane 0 paketov aj 0 bajtov). Na koniec pripojí okrajové riadky s časom štartu ≥ poslednému: rev bez paketov; `flowend` skôr než `flowstart`; poškodený riadok (8 polí); flow s 1 paketom; flow s 20 paketmi a nulovým trvaním. Výstup: `tests/data/flows_small.csv` s pôvodnou hlavičkou.
- [ ] **Step 2: Vygenerovať a overiť dáta**
  Run: `python3 tests/make_test_data.py && wc -l tests/data/flows_small.csv`
  Expected: 1507 riadkov (hlavička + 1500 + 6 okrajových).
- [ ] **Step 3: `tests/make_golden.sh`** — vytvorí dočasný `git worktree` z commitu `17b5145` (napr. v `$(mktemp -d)`), pre každú kombináciu `RETA_ON` ∈ {1,0} × `IP_ONLY_ON` ∈ {0,1} upraví v kópii `src/config.hpp` (`RETA_ON`, `IP_ONLY_ON`, `WINDOW_SIZE = 500`), skompiluje (`make`) a spustí `./benchmark tests/data/flows_small.csv <out> <sym 0|1|2> <weighting>` pre 3 symetrie × 3 váhy. JSQ raz (`RETA_ON 1`, `IP_ONLY_ON 0`): `./benchmark … 0 jsq <weighting>` pre 3 váhy. Výstupy pomenuje podľa *Interfaces*, uloží do `tests/golden/` a worktree odstráni (`git worktree remove`).
- [ ] **Step 4: Spustiť a overiť referenciu**
  Run: `bash tests/make_golden.sh && ls tests/golden | wc -l && head -2 tests/golden/retaon_full_none_packet.csv`
  Expected: `39`; hlavička `algorithm,symmetry,weighting,tuple_run_index,key_id,num_channels,thresshold_sum,fairness,chi` a riadok s `toeplitz,none,packet,1,0,8,…`. Súbory `*_flow.csv` musia mať aspoň 2 okná (okrem hlavičky aspoň 1120 riadkov).
- [ ] **Step 5: `tests/compare.py`** — načíta oba adresáre a každý riadok premení na kľúč `(method, tuple, reta, symmetry, weighting, hash, key_id, num_channels, window)` a hodnoty `(overload, fairness, chi)` **ako text**.
  - Starý formát podľa názvu súboru: `<reta>_<tuple>_…` → tuple a reta z názvu, `method = hash`, `hash = algorithm`, `window = tuple_run_index`, `overload = thresshold_sum`. `jsq_<w>.csv` → `method = jsq`, tuple/reta/hash = `-`, symetria `none`. Kvôli úlohe 8 aj `run_sym<0|1|2>_<w>.csv` → `full`, `retaon`, symetria podľa čísla.
  - Nový formát (úloha 5) rozpozná podľa hlavičky (stĺpec `method`).
  - Vypíše počet porovnaných riadkov, prvých 10 rozdielov a počty riadkov, ktoré chýbajú na jednej alebo druhej strane.
- [ ] **Step 6: Overiť porovnávač**
  Run: `python3 tests/compare.py tests/golden tests/golden; echo $?`
  Expected: `OK: <N> riadkov, 0 rozdielov`, kód `0`.
  Run: `mkdir -p tests/out && cp -r tests/golden tests/out/g2 && sed -i.bak '2s/,[^,]*$/,9.99999/' tests/out/g2/retaon_full_none_packet.csv && rm tests/out/g2/*.bak && python3 tests/compare.py tests/golden tests/out/g2; echo $?`
  Expected: 1 rozdiel, kód `1`.
- [ ] **Step 7: Vysvetlenie pre autora** — čo je referencia, prečo zmenšené dáta, ako čítať výstup `compare.py`. Počkať na odsúhlasenie.
- [ ] **Step 8: Commit**
  ```bash
  git add tests/make_test_data.py tests/make_golden.sh tests/compare.py tests/test.conf .gitignore
  git add -f tests/data/flows_small.csv
  git commit -m "tests: small test dataset, golden results from 17b5145, comparer"
  ```

---

### Task 1: Súbor experimentu

**Files:**
- Create: `src/types.hpp` — spoločné typy bez ďalších závislostí (sem sa v úlohách 3 a 4 presunú aj `Weighting`, `Symmetry`, `Endpoints`, `FlowInfo`, aby nevznikali kruhové `#include`)
- Create: `src/experiment.hpp`, `src/experiment.cpp`
- Create: `tests/unit_tests.cpp`
- Modify: `Makefile` (cieľ `test`: `tests/unit_tests` zlinkovaný so všetkými objektmi okrem toho, ktorý obsahuje `main()`)
- Create: `experiments/hash_comparison.conf` (dnešné porovnanie: `data/odid1.csv`, `tuple = full`, 5 hashov, `reta = on`, `method = hash jsq`, všetky symetrie, váhy a kanály, `keys = 16`, `window = 150000`)

**Interfaces:**
- Produces (`src/types.hpp`): `enum class TupleKind { Full, IpOnly };   // "full" | "ip"`
- Produces (`src/experiment.hpp`):
  ```cpp
  struct Experiment {
      std::string data;                             // required
      std::string out;                              // required
      std::vector<TupleKind>   tuples;              // default {Full}
      std::vector<std::string> hashes;              // default {"toeplitz"}; names, checked in task 4
      std::vector<bool>        reta;                // "on"=true, "off"=false; default {true}
      bool                     method_hash;         // "hash" in method; default true
      bool                     method_jsq;          // "jsq" in method;  default false
      std::vector<Symmetry>    symmetries;          // "none" "xor" "sort"; default all three
      std::vector<Weighting>   weightings;          // "flow" "packet" "byte"; default all three
      std::vector<int>         channels;            // default {8,16,20,32,40,64,128}; each 1..128
      int                      keys;                // default 16; >= 1
      long                     window;              // default 150000; >= 1
  };
  Experiment loadExperiment(const std::string &path);   // throws std::runtime_error("<path>:<line>: <reason>")
  std::string experimentText(const Experiment &e);       // normalized key = value dump, copied to the output dir
  ```
  `Symmetry` je zatiaľ z `src/transform.hpp`, `Weighting` z `src/flow_expand.hpp`. V úlohách 3 a 4 sa presunú do `src/types.hpp`, názvy hodnôt ostávajú. Prázdne riadky a všetko za `#` sa ignoruje. Hodnoty sa oddeľujú medzerami.

- [ ] **Step 1: Failing testy v `tests/unit_tests.cpp`** (vlastné makro `CHECK(cond)` počíta chyby, `main` vráti počet chýb):
  - `tests/test.conf` sa načíta: `tuples == {Full, IpOnly}`, 5 hashov v poradí `toeplitz jhash chaskey halfsiphash simpletab`, `reta == {true,false}`, `method_hash && method_jsq`, 3 symetrie, 3 váhy, 7 kanálov, `keys == 16`, `window == 500`.
  - súbor iba s `data` a `out` → všetky predvolené hodnoty podľa *Interfaces*.
  - každý z týchto vstupov vyhodí `std::runtime_error`, ktorého text obsahuje číslo riadku: neznámy kľúč `windw = 5`; `channels = 8 200`; `window = abc`; `reta = maybe`; `tuple =` (prázdny zoznam); chýbajúce `data`.
- [ ] **Step 2: Spustiť, overiť, že padá**
  Run: `make test`
  Expected: chyba kompilácie (`loadExperiment` neexistuje).
- [ ] **Step 3: Implementovať `loadExperiment` a `experimentText` v `src/experiment.cpp`**
- [ ] **Step 4: Spustiť testy**
  Run: `make test && ./tests/unit_tests`
  Expected: `all checks passed`, kód 0.
- [ ] **Step 5: Vysvetlenie pre autora** — formát `.conf`, predvolené hodnoty, ako vyzerá chyba. Odsúhlasenie.
- [ ] **Step 6: Commit** — `git commit -m "add experiment file (.conf) loader"`

---

### Task 2: Parametre za behu namiesto `#define`

**Files:**
- Create: `src/main.cpp` (nový `main`; program sa volá `rss_sim`)
- Modify: `src/benchmark.cpp` (odstrániť `main`; `runFlowModeRetaOn/Off` a `runJsqMode` dostanú parametre namiesto konštánt; zrušiť `runSimpleTupleMode`, `countFields`)
- Create: `src/benchmark.hpp` (deklarácie troch run funkcií)
- Modify: `src/config.hpp` (zmazať `RETA_ON`, `IP_ONLY_ON`, `OFFSET`, `WINDOW_SIZE`, `NUM_KEYS`, `CHANNEL_COUNTS`; ponechať `KEY_SEED`, `RSS_KEY_SIZE`, `TUPLE_SIZE`, `RETA_SIZE` a `static_assert` na mocninu dvoch)
- Modify: `src/parser.hpp/.cpp` (`parseFlowLine(line, TupleKind)`; zrušiť `parseLineToTuple`)
- Modify: `src/reta.hpp` (tabuľky pre ľubovoľný zoznam kanálov za behu)
- Modify: `Makefile` (`TARGET := rss_sim`), `.gitignore` (`/rss_sim`, `/tests/unit_tests`)

**Interfaces:**
- Consumes: `Experiment loadExperiment(...)` (úloha 1).
- Produces (`src/benchmark.hpp`):
  ```cpp
  struct RunParams {                     // what used to be #defines and constexpr
      TupleKind              tuple;      // decides tuple layout and symmetry offset (Full 18, IpOnly 16)
      std::vector<int>       channels;
      int                    keys;
      long                   window;
  };
  void runFlowModeRetaOn (std::istream&, ResultsWriter&, Symmetry, Weighting, const RunParams&, const KeySet&, const std::vector<int> &hash_ids);
  void runFlowModeRetaOff(std::istream&, ResultsWriter&, Symmetry, Weighting, const RunParams&, const KeySet&, const std::vector<int> &hash_ids);
  void runJsqMode        (std::istream&, ResultsWriter&, Symmetry, Weighting, const RunParams&);
  ```
  `hash_ids` = indexy do `hash_functions_arr` v poradí z experimentu.
- Produces: `./rss_sim <experiment.conf>` spustí všetky kombinácie postupne a zapíše **starý formát** s názvami súborov ako v `tests/golden/` do `experiment.out`.
- `FlowSlot` (dnes podľa `RETA_ON`) nahradí: `uint8_t` v RetaOn a `uint32_t` v RetaOff (každá funkcia má vlastný typ, odstráni sa krehkosť zo spec 4).

- [ ] **Step 1: Prepojiť** — `main` načíta experiment, vygeneruje kľúče (`getKeys` s počtom `keys`, jeden vector pre celý beh, nekopírovať kvôli simpletab), zavolá `simpletabInit`, pre každú kombináciu tuple × reta × symetria × váha otvorí dataset a výstup a zavolá príslušnú run funkciu. JSQ (ak `method_jsq`) iba so symetriou `none`, jeden súbor na váhu.
- [ ] **Step 2: Kompilácia bez varovaní z `src/`**
  Run: `make 2>&1 | grep -E "src/.*(warning|error)"`
  Expected: prázdny výstup.
- [ ] **Step 3: Porovnanie s referenciou bajt po bajte**
  Run: `./rss_sim tests/test.conf && diff -r tests/golden tests/out && echo IDENTICKE`
  Expected: `IDENTICKE`.
- [ ] **Step 4: Unit testy ďalej prechádzajú** — `make test && ./tests/unit_tests`.
- [ ] **Step 5: Vysvetlenie pre autora** — kam zmizli `#define`, čo je `RunParams`, ako sa spúšťa `rss_sim`. Odsúhlasenie.
- [ ] **Step 6: Commit** — `git commit -m "runtime parameters from the experiment file instead of #defines"`

---

### Task 3: Čítanie flowov bez callbackov

**Files:**
- Create: `src/engine/flow_reader.hpp`, `src/engine/flow_reader.cpp` (nahrádza `src/flow_expand.*`, tie sa zmažú)
- Modify: `src/benchmark.cpp` (tri run funkcie ako slučka `while (reader.next(e))`, žiadne lambdy)
- Modify: `Makefile` (`src/engine/*.cpp` v `SRCS`)

**Interfaces:**
- Produces (`src/types.hpp`, presun z `flow_expand.hpp`):
  ```cpp
  enum class Weighting { Flow = 0, Packet = 1, Byte = 2 };
  const char *weightingName(Weighting w);                     // "flow" | "packet" | "byte"
  using Tuple = std::array<uint8_t, TUPLE_SIZE>;
  struct FlowInfo { Tuple tuple; double t_start, t_end; int64_t packets; int64_t weight; };
  ```
- Produces (`src/engine/flow_reader.hpp`):
  ```cpp
  enum EventType { EVENT_FLOW_START, EVENT_PACKET, EVENT_FLOW_END };
  struct Event {
      EventType type;
      int       flow;      // id of the flow direction, reused after its FLOW_END
      int64_t   weight;    // EVENT_PACKET only
      FlowInfo  info;      // EVENT_FLOW_START only
  };
  class FlowReader {
  public:
      FlowReader(std::istream &in, Weighting w, TupleKind tuple, Symmetry sym);
      bool next(Event &e);   // false when the file and all flows are exhausted
  };
  ```
  `FlowInfo.tuple` je v tejto úlohe už so symetriou (ako dnes vstup do `prepare`). V úlohe 4 sa zmení na `Endpoints`.
- **Poradie udalostí musí byť rovnaké ako dnes:** pre každý riadok najprv všetky pakety s `next_time <= t_start` (PACKET; ak to bol posledný paket smeru, hneď za ním FLOW_END), potom FLOW_START fwd, potom FLOW_START rev (iba ak má pakety). Na konci súboru zvyšok haldy. Udalosti sa generujú **postupne** (`next()` vráti jednu), nie celý zvyšok naraz. Pri vyprázdňovaní na konci by išlo o milióny paketov.
- `flow` id prideľuje `FlowReader` (zoznam voľných id ako dnes `free_slots`).

- [ ] **Step 1: Unit test v `tests/unit_tests.cpp`** — `FlowReader` nad textom z dvoch flowov z príkladu „A 0.0–0.4 s 3 pakety, B 0.1–0.3 s 2 pakety“ (váha `packet`) vráti presne: START A, PACKET A, START B, PACKET B, PACKET A, PACKET B, END B, PACKET A, END A.
- [ ] **Step 2: Spustiť, overiť, že padá** — `make test` → chyba kompilácie.
- [ ] **Step 3: Implementovať `FlowReader` a prepísať run funkcie na slučku**
- [ ] **Step 4: Testy** — `make test && ./tests/unit_tests` → OK.
- [ ] **Step 5: Referencia** — `./rss_sim tests/test.conf && diff -r tests/golden tests/out && echo IDENTICKE` → `IDENTICKE`.
- [ ] **Step 6: Vysvetlenie pre autora** — udalosti namiesto callbackov, kde je halda, ako sa prideľujú id. Odsúhlasenie.
- [ ] **Step 7: Commit** — `git commit -m "flow reader returns events instead of calling callbacks"`

---

### Task 4: RSS modul – tuple, hashe, priradenie

**Files:**
- Create: `src/rss/tuple.hpp/.cpp` (presun z `parser.cpp` a `transform.cpp`; `transform.*` sa zmaže)
- Create: `src/rss/hashes.hpp/.cpp`
- Create: `src/rss/digest.hpp/.cpp` (krok „úprava hashu“ zo spec 5.1, zatiaľ iba „bez úpravy“)
- Create: `src/rss/assign.hpp/.cpp` (RETA tabuľky, modulo, JSQ orákulum; `src/reta.hpp` sa zmaže)
- Modify: `src/wrappers/hash_register.cpp` (všetkých 8 obálok)
- Modify: `src/engine/flow_reader.*` (`FlowInfo` nesie `Endpoints`, tuple sa už nestavia v čítačke)
- Modify: `src/benchmark.cpp` (run funkcie používajú `rss/`)

**Interfaces:**
- Produces (`src/types.hpp`, presun z `transform.hpp`):
  ```cpp
  enum class Symmetry { None = 0, XorFold = 1, SortFold = 2 };
  const char *symmetryName(Symmetry s);                       // "none" | "xorfold" | "sortfold"
  struct Endpoints { uint8_t src_ip[16]; uint8_t dst_ip[16]; uint16_t src_port; uint16_t dst_port; uint8_t protocol; };
  // FlowInfo: field `Tuple tuple` is replaced by `Endpoints ep`
  ```
- Produces (`src/rss/tuple.hpp`):
  ```cpp
  Tuple  buildTuple(TupleKind kind, const Endpoints &ep);      // Full: src_ip|src_port(BE)|dst_ip|dst_port(BE); IpOnly: src_ip|dst_ip|4x 0
  size_t symmetryOffset(TupleKind kind);                       // Full 18, IpOnly 16
  Tuple  applySymmetry(Symmetry s, const Tuple &in, size_t offset);
  Tuple  rssInput(TupleKind kind, Symmetry s, const Endpoints &ep);   // buildTuple + applySymmetry
  ```
  Rev smer má v `Endpoints` prehodené src/dst.
- Produces (`src/rss/digest.hpp`):
  ```cpp
  enum class DigestKind { None };                  // new hash post-processing variants are added here
  uint32_t applyDigest(DigestKind kind, uint32_t hash);   // None: returns hash unchanged
  ```
  Volá sa medzi `rssHash` a `retaIndex` / `moduloChannel`. V tomto refaktore je vždy `None`.
- Produces (`src/rss/hashes.hpp`):
  ```cpp
  using Key = std::array<uint8_t, config::RSS_KEY_SIZE>;
  std::vector<Key> generateKeys(int count);       // mt19937(KEY_SEED), byte-for-byte the old getKeys
  void             initHashes(const std::vector<Key> &keys);   // simpletabInit; keys must outlive all hashing, never copied
  int              findHash(const std::string &name);           // index into hash_functions_arr, -1 if unknown
  uint32_t         rssHash(int hash_id, const Tuple &t, const Key &key);
  ```
  `hash_functions_arr` (v `hash_register`) bude mať 8 položiek v poradí: `toeplitz, jhash, chaskey, halfsiphash, simpletab, multiplyshift, siphash, xorhash`. Neznámy hash v experimente → chyba pri štarte.
- Produces (`src/rss/assign.hpp`):
  ```cpp
  using RetaTable = std::array<uint8_t, RETA_SIZE>;
  RetaTable buildRoundRobinReta(int channels);         // entry i -> i % channels
  uint8_t   retaIndex(uint32_t hash);                  // hash & (RETA_SIZE - 1)
  int       moduloChannel(uint32_t hash, int channels);

  struct JsqOracle {                                   // state of the oracle, one per pass
      std::vector<std::vector<double>> rate_sum;       // [channel count][channel]
      double  packet_rate_total;
      int64_t rated_flows;
      std::vector<double> flow_wrate, flow_prate;      // per flow id
  };
  void jsqInit(JsqOracle &o, const std::vector<int> &channels);
  void jsqFlowStart(JsqOracle &o, int flow, const FlowInfo &info,
                    const std::vector<std::vector<int64_t>> &window_hist,  // delivered this window [c][ch]
                    int64_t packets_in_window, long window,
                    uint8_t *chosen);                  // out: one channel per channel count
  void jsqFlowEnd(JsqOracle &o, int flow, const uint8_t *chosen);
  ```
  Výpočty JSQ (poradie operácií, `MIN_DURATION = 1e-3`, nulovanie pri `rated_flows == 0`) presne ako dnes v `runJsqMode`.

- [ ] **Step 1: Failing unit testy**
  - `buildTuple(Full, ep)` pre `ep` = 10.0.0.1:1234 → 10.0.0.2:80 (IPv4-mapped) sa rovná bajtom zo starého `tuple37`; `buildTuple(IpOnly, …)` sa rovná starému `tuple32`, posledné 4 bajty sú 0.
  - `applySymmetry(XorFold, …)` aj `SortFold` dajú rovnaký výsledok pre fwd aj rev smer.
  - `generateKeys(16)` dvakrát po sebe = rovnaké kľúče; `generateKeys(16)[0]` = prvý kľúč starého `getKeys()`.
  - `buildRoundRobinReta(20)`: kanály 0–7 majú po 7 položiek, kanály 8–19 po 6.
  - `findHash("toeplitz") == 0`, `findHash("xorhash") == 7`, `findHash("nope") == -1`.
  - `applyDigest(DigestKind::None, 0xDEADBEEF) == 0xDEADBEEF`.
- [ ] **Step 2: Spustiť, overiť, že padá** — `make test` → chyba kompilácie.
- [ ] **Step 3: Implementovať `rss/` a prepojiť run funkcie**
- [ ] **Step 4: Testy** — `make test && ./tests/unit_tests` → OK.
- [ ] **Step 5: Referencia** — `./rss_sim tests/test.conf && diff -r tests/golden tests/out && echo IDENTICKE` → `IDENTICKE`.
- [ ] **Step 6: Hashe mimo referencie** — `tests/extra_hashes.conf` (ako `test.conf`, ale `out = tests/out_extra`, `hash = multiplyshift siphash xorhash`, `tuple = full`, `reta = on`, `method = hash`). Run: `./rss_sim tests/extra_hashes.conf` → kód 0 a `tests/out_extra/retaon_full_none_packet.csv` má (bez hlavičky) presne 3/5 riadkov `tests/golden/retaon_full_none_packet.csv` (3 hashe namiesto 5).
- [ ] **Step 7: Vysvetlenie pre autora** — kroky RSS reťaze v `rss/`, kde sa pridá nový hash, prečo `Endpoints`. Odsúhlasenie.
- [ ] **Step 8: Commit** — `git commit -m "RSS module: tuple, hash registry with all wrappers, assignment"`

---

### Task 5: Svety, počítanie a nový formát výsledkov (hashe)

**Files:**
- Create: `src/rss/world.hpp/.cpp`, `src/rss/rss_unit.hpp/.cpp`
- Create: `src/engine/window_counter.hpp/.cpp`
- Move: `src/metrics.*` → `src/engine/metrics.*` (výpočet bez zmeny)
- Modify: `src/results_writer.*` → `src/engine/results_writer.*` (nový formát)
- Modify: `src/main.cpp` (prechody váha × symetria; `--list-passes`, `--pass <váha> <symetria>`)
- Modify: `src/benchmark.cpp` (zmazať `runFlowModeRetaOn/Off`; `runJsqMode` zatiaľ zostáva so starým formátom, zapisuje `jsq_<w>.csv`)

**Interfaces:**
- Produces (`src/rss/world.hpp`):
  ```cpp
  enum class WorldKind { HashReta, HashModulo, Jsq };
  struct World { WorldKind kind; TupleKind tuple; int hash_id; int key_id; };   // Jsq: hash_id = key_id = -1
  std::vector<World> buildWorlds(const Experiment &exp, Symmetry sym);
  // order: tuple, hash, reta (on before off), key. Jsq is added in task 6.
  ```
- Produces (`src/rss/rss_unit.hpp`):
  ```cpp
  class RssUnit {
  public:
      RssUnit(const std::vector<World> &worlds, const std::vector<int> &channels,
              Symmetry sym, const std::vector<Key> &keys);   // keeps a reference to keys, never copies (simpletab)
      void flowStart(int flow, const FlowInfo &info, const WindowCounter &counter);
      void flowEnd(int flow);
      const uint8_t *slots(int flow) const;   // this flow's decisions: 1 byte per HashReta world, NC bytes per direct world
  };
  ```
  Rozloženie `slots` je v poradí svetov z `buildWorlds`; `WindowCounter` ho číta v rovnakom poradí. HashReta: `retaIndex(applyDigest(None, rssHash(...)))`; HashModulo: pre každé `c` `moduloChannel(applyDigest(None, rssHash(...)), channels[c])`.
- Produces (`src/engine/window_counter.hpp`):
  ```cpp
  class WindowCounter {
  public:
      WindowCounter(const std::vector<World> &worlds, const std::vector<int> &channels, long window);
      void    addPacket(const uint8_t *slots, int64_t weight);   // HashReta: buckets[w][idx] += weight; direct: hist[w][c][ch] += weight
      bool    windowFull() const;
      long    windowIndex() const;          // 1-based, matches the old tuple_run_index
      int64_t windowWeight() const;         // the old window_sum_total
      int64_t packetsInWindow() const;      // packets counted since the last window end
      void    channelLoads(int world, int c, std::vector<int64_t> &out) const;  // HashReta: fold RETA[c]; direct: copy hist
      const std::vector<std::vector<int64_t>> &directHist(int world) const;
      void    endWindow();                  // zero buckets, hists and window weight
  };
  ```
- Produces (`src/engine/results_writer.hpp`), hlavička CSV:
  `dataset,weighting,method,tuple,symmetry,hash,reta,key_id,num_channels,window,overload,fairness,chi`
  `method` ∈ {`hash`,`jsq`}; `tuple` ∈ {`full`,`ip`,`-`}; `symmetry` = `symmetryName`; `reta` ∈ {`on`,`off`,`-`}; čísla cez `ostream` ako dnes.
  ```cpp
  void writeWindow(std::ostream &os, const std::string &dataset, Weighting w, Symmetry sym,
                   const std::vector<World> &worlds, const std::vector<int> &channels,
                   const WindowCounter &counter);
  ```
- Produces: výstup prechodu `<out>/<weighting>_<symmetry>.csv` a kópia `<out>/experiment.conf` (`experimentText`).
- Produces: `./rss_sim exp.conf --list-passes` vypíše jeden riadok na prechod: `<weighting> <symmetry>`. `./rss_sim exp.conf --pass packet none` spustí jeden prechod. Bez prepínača všetky postupne.

- [ ] **Step 1: Failing unit test** — `WindowCounter` s jedným `HashReta` svetom, kanálmi `{8, 20}`, oknom 4: štyri pakety s RETA indexom 0, 8, 20, 127 a váhou 1 → `windowFull()`; `channelLoads(0, 0)` = `{2,0,0,0,1,0,0,1}` (0, 8 → kanál 0; 20 → 4; 127 → 7); `channelLoads(0, 1)` má súčet 4 a hodnota kanála 0 je 2 (0 a 20 → 0); po `endWindow()` sú všetky nuly.
- [ ] **Step 2: Spustiť, overiť, že padá** — `make test` → chyba kompilácie.
- [ ] **Step 3: Implementovať `World`, `RssUnit`, `WindowCounter`, writer a hlavnú slučku v `main.cpp`** (spec 5.4: `flowStart` → `addPacket` → `flowEnd` → pri plnom okne `writeWindow` + `endWindow`).
- [ ] **Step 4: Testy** — `make test && ./tests/unit_tests` → OK.
- [ ] **Step 5: Referencia** — `rm -rf tests/out && ./rss_sim tests/test.conf && python3 tests/compare.py tests/golden tests/out`
  Expected: `OK`, 0 rozdielov, 0 chýbajúcich riadkov na oboch stranách (JSQ sa zatiaľ porovná zo starého formátu `jsq_<w>.csv`).
- [ ] **Step 6: Prechody** — `./rss_sim tests/test.conf --list-passes` vypíše 9 riadkov; `./rss_sim tests/test.conf --pass packet none` vytvorí iba `tests/out/packet_none.csv`.
- [ ] **Step 7: Vysvetlenie pre autora** — svety, dva spôsoby počítania (RETA a priamy), skladanie, nový CSV. Odsúhlasenie.
- [ ] **Step 8: Commit** — `git commit -m "worlds, window counter with RETA and direct paths, new results format"`

---

### Task 6: JSQ ako svet, upratanie a výkon

**Files:**
- Modify: `src/rss/world.cpp` (pridať JSQ svet: iba ak `method_jsq` a `sym == None`, ako posledný)
- Modify: `src/rss/rss_unit.cpp` (JSQ cez `jsqFlowStart`/`jsqFlowEnd`, záťaž z `counter.directHist`)
- Delete: `src/benchmark.cpp`, `src/benchmark.hpp`, zvyšky `parser.*` presunúť do `src/engine/flow_reader.cpp` (`splitCsvLine`, `ipToBytes16`, čas) a `parser.*` zmazať
- Modify: `Makefile`, `README.md` (iba odkaz na `docs/ako_to_funguje.md`, ten vznikne v úlohe 8)

**Interfaces:**
- Consumes: `JsqOracle` a `jsqFlowStart/End` (úloha 4), `World`, `RssUnit`, `WindowCounter` (úloha 5).
- JSQ riadky v CSV: `method = jsq`, `tuple = -`, `hash = -`, `reta = -`, `key_id = 0`, `symmetry = none`.

- [ ] **Step 1: Implementovať JSQ svet a zmazať starý kód**
- [ ] **Step 2: Kompilácia** — `make 2>&1 | grep -E "src/.*(warning|error)"` → prázdne; v `src/` nie je žiadny výskyt `std::function` ani lambdy (`grep -rnE "std::function|\[&\]|\[=\]" src/` → prázdne).
- [ ] **Step 3: Referencia** — `rm -rf tests/out && ./rss_sim tests/test.conf && python3 tests/compare.py tests/golden tests/out` → `OK`, 0 rozdielov, 0 chýbajúcich.
- [ ] **Step 4: Výkon** — na `data/flow_big.csv` porovnať starý a nový beh pri rovnakých svetoch (5 hashov × 16 kľúčov, `full`, RETA zap., `packet`, `none`, okno 150000). Starý: `benchmark` z worktree `17b5145` (`./benchmark data/flow_big.csv tests/out/old_perf.csv 0 packet`). Nový: `./rss_sim <conf> --pass packet none`. Každý 3×, porovnať medián `real`.
  Expected: nový ≤ 1,2 × starý. Ak nie, zastaviť a ukázať autorovi čísla.
- [ ] **Step 5: Vysvetlenie pre autora** — JSQ v RSS module, čo sa zmazalo a prečo. Odsúhlasenie.
- [ ] **Step 6: Commit** — `git commit -m "JSQ as a world, remove the old benchmark code"`

---

### Task 7: `run.sh`, experimenty a `analyze.py`

**Files:**
- Modify: `src/main.cpp` (prepínač `--print-out` vypíše `experiment.out`)
- Modify: `run.sh` (`./run.sh <experiment.conf>`)
- Modify: `analyze.py` (`load_results` číta nový formát)
- Create: `experiments/full_tuple.conf`, `experiments/ip_only.conf`, `experiments/jsq.conf`; upraviť `experiments/hash_comparison.conf`
- Create: `tests/flow_big_check.conf` (`data/flow_big.csv`, Toeplitz, `method = hash jsq`, okno 150000)

**Interfaces:**
- `run.sh <conf>`: `make`; zoznam prechodov z `./rss_sim <conf> --list-passes`; každý prechod ako proces na pozadí (najviac `JOBS`, predvolene 9); logy a značky `.failed` ako dnes (do `<out>/logs`, `<out>` sa zistí ako `./rss_sim <conf> --print-out`); pri chybe skončiť bez `analyze.py`; inak `python analyze.py <out> <out>`.
- `analyze.py`: `load_results` načíta `<out>/*_*.csv` v novom formáte a premenuje stĺpce na staré mená (`algorithm` = `hash` alebo `jsq`, `thresshold_sum` = `overload`, `tuple_run_index` = `window`). Do dnešných grafov idú iba riadky `method == hash`, prvý `tuple` a prvé `reta` z experimentu (ak ich je viac, vypíše, ktoré použil). Prázdne súbory (iba hlavička) preskočí.

- [ ] **Step 1: Prázdne okno** — `tests/empty_window.conf` (`tests/data/flows_small.csv`, `window = 100000000`, inak predvolené). Run: `./rss_sim tests/empty_window.conf && python3 analyze.py <out> <out>; echo $?` → kód 0, CSV iba s hlavičkou, bez výnimky.
- [ ] **Step 2: Upraviť `run.sh` a `analyze.py`**
- [ ] **Step 3: Celý beh na `flow_big`** — `./run.sh tests/flow_big_check.conf` → kód 0, v `<out>` 9 CSV, `logs/` bez `.failed`, 8 PNG ako dnes.
- [ ] **Step 4: Vysvetlenie pre autora** — ako spustiť experiment, kde sú výsledky, ako pridať nový `.conf`. Odsúhlasenie.
- [ ] **Step 5: Commit** — `git commit -m "run.sh and analyze.py driven by experiment files"`

---

### Task 8: Dokumentácia a plný beh

**Files:**
- Create: `docs/ako_to_funguje.md` (po slovensky, ~1 strana: tok programu od `.conf` po CSV, súbory a ich úlohy, kroky RSS reťaze, kde pridať novú variantu hashu / priradenia / úpravy hashu, ako spustiť testy)
- Modify: `README.md` (odkazy na `docs/ako_to_funguje.md` a `experiments/`)

- [ ] **Step 1: Napísať `docs/ako_to_funguje.md`** — každý súbor v `src/` je v ňom spomenutý; obsahuje diagram reťaze zo spec 2 a hlavnú slučku zo spec 5.4 (podľa skutočného kódu).
- [ ] **Step 2: Kontrola úplnosti** — `ls src src/engine src/rss` a každý `.cpp`/`.hpp` je v dokumente spomenutý.
- [ ] **Step 3: Návod pre autora na plný beh** — autor spustí `./run.sh experiments/hash_comparison.conf` na `data/odid1.csv` (Claude dáta neotvára) a potom `python3 tests/compare.py super_results <out>`. `compare.py` páruje `run_sym*` a `jsq_*` súbory zo `super_results/` s novými. Expected: `OK`, 0 rozdielov.
- [ ] **Step 4: Vysvetlenie pre autora a záverečné odsúhlasenie**
- [ ] **Step 5: Commit** — `git commit -m "docs: how the simulator works"`
