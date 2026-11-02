# Výsledky – časť 1: porovnanie hash funkcií pre RSS

Grafy z prvej časti výskumu: ako rovnomerne rozdeľujú rôzne hash funkcie
flowy do DMA kanálov pri zachovaní pravidla „1 flow = 1 kanál“.

Kód, ktorý ich vytvoril, je označený tagom **`v1.0-hash-comparison`**.

## Čo je v ktorom adresári

| Adresár | Pôvodný názov | Dataset | Tuple pre hash | RETA | Vygenerované |
|---|---|---|---|---|---|
| `odid1_full_tuple/` | `results1` | `data/odid1.csv` | plný: IP + porty (`IP_ONLY_ON 0`) | zapnutá, 128 položiek | 24. 9. 2026 |
| `odid2_full_tuple/` | `results2` | `data/odid2.csv` | plný: IP + porty (`IP_ONLY_ON 0`) | zapnutá, 128 položiek | 24. 9. 2026 |
| `odid4_full_tuple/` | `results4` | `data/odid4.csv` | plný: IP + porty (`IP_ONLY_ON 0`) | zapnutá, 128 položiek | 24. 9. 2026 |
| `odid6_full_tuple/` | `results6` | `data/odid6.csv` | plný: IP + porty (`IP_ONLY_ON 0`) | zapnutá, 128 položiek | 24. 9. 2026 |
| `odid1_ip_only/` | `ip_res1` | `data/odid1.csv` | iba IP adresy (`IP_ONLY_ON 1`) | zapnutá, 128 položiek | 29. 9. 2026 |

Spoločné nastavenia všetkých behov:

- hash funkcie: toeplitz, jhash (lookup3), chaskey, halfsiphash, multiply-shift
- 16 náhodných RSS kľúčov, pevný seed `KEY_SEED = 65536`
- počty DMA kanálov: 8, 16, 20, 32, 40, 64, 128
- okno: 150 000 paketov
- váha: flow / packet / byte, symetria: none / xorfold / sortfold
- `odid1.csv` obsahuje zrkadlené duplicitné riadky (~12 % riadkov, ~42 %
  paketov); vyčistená verzia `odid1_dedup.csv` tu použitá **nebola**.
  `odid2/4/6` na duplicity overené neboli.

`results1/2/4/6` vznikli s kódom z commitu `68bbf21`. Ten je až na pridaný
prepínač `IP_ONLY_ON` zhodný s týmto tagom: s `IP_ONLY_ON 0` dáva rovnaký tuple
(IP + porty, offset 18).

## Grafy v každom adresári

| Súbor | Čo ukazuje |
|---|---|
| `overload_{none,xorfold,sortfold}.png` | preťaženie kanálov (‰ nad spravodlivým podielom) podľa algoritmu a počtu kanálov; riadok 2 = pomer k Toeplitzu |
| `spread.png` | η²: aký podiel rozptylu vysvetľuje voľba algoritmu (0 = na hashi nezáleží, 1 = rozhoduje iba hash) |
| `keys_{8,40,128}.png` | rozptyl výsledkov medzi 16 kľúčmi pri 8, 40 a 128 kanáloch (váha flow) |

## Na čo myslieť pri interpretácii

- **20 a 40 kanálov:** 128 položiek RETA sa nedá rozdeliť rovnomerne
  (pri n=20 má 8 kanálov 7 položiek a 12 kanálov 6). Aj dokonalý hash tu dá
  preťaženie 37,5 ‰ (n=20) a 50 ‰ (n=40). Špičky v grafoch pri týchto počtoch
  spôsobuje tabuľka, nie hash.
- **Duplicitné riadky** v `odid1.csv` nadhodnocujú preťaženie približne
  o 2–4 % (merané na výreze `odid1`). Poradie algoritmov to nemení.

## Ako výsledky zopakovať

```bash
git checkout v1.0-hash-comparison
# src/config.hpp: IP_ONLY_ON 0 = plný tuple, IP_ONLY_ON 1 = iba IP
./run.sh data/odid1.csv <vystupny_adresar>
```

Surové CSV sa neukladajú (stovky MB na jeden beh), dajú sa takto kedykoľvek
znova vygenerovať.
