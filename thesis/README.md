# Bakalárska práca: čo je kde a ako to funguje

**Návrh a analýza variantov distribúcie paketov do DMA kanálov jednotkou RSS frameworku NDK**

## Štruktúra

```
thesis/
├── projekt.tex            hlavný súbor: názov, abstrakt, zoznam kapitol
├── literatura.bib         zdroje na citovanie
│
├── kapitoly/              TEXT PRÁCE
│   ├── 01_uvod.tex
│   ├── 02_ndk.tex                     platforma NDK (zatiaľ osnova)
│   ├── 03_rss.tex                     ako funguje RSS
│   ├── 04_metriky_a_metodika.tex      metriky, dáta, okná
│   ├── 05_simulator.tex               náš simulátor
│   ├── 06_analyza_rss.tex             výsledky: hashe, kľúče, JSQ, okno
│   ├── 07_navrh_variant.tex           návrhy úprav RSS
│   ├── 08_vyhodnotenie_variant.tex    kostra, čaká na výsledky
│   └── 09_zaver.tex
│
├── prilohy/
│   ├── A_obsah_media.tex              obsah odovzdaného média
│   ├── B_doplnkove_grafy.tex          grafy ďalších datasetov
│   └── C_manual.tex                   návod na simulátor
│
├── tabulky/               tabuľky, vkladajú sa cez \input
├── obrazky-figures/       grafy
│   ├── hashe/                         porovnanie hashov
│   ├── kluce/                         vplyv RSS kľúča
│   ├── analyza/                       veľké toky, JSQ, okno
│   └── ostatne_vzorky/                odid2, odid4, odid6, iba IP
│
├── build/                 vygenerované súbory a hotové PDF, nemeniť
│
├── fitthesis.cls          šablóna FIT, nemeniť
├── osnova-makra.tex       makrá \TODO a pokyny
├── bib-styles/            štýl literatúry, nemeniť
└── template-fig/          logá VUT, nemeniť
```

## Ako sa to skladá

`projekt.tex` je jediný súbor, ktorý sa kompiluje. Cez `\input` si načíta kapitoly,
kapitoly si načítajú tabuľky a grafy:

```
projekt.tex → kapitoly/06_analyza_rss.tex → tabulky/jsq.tex
                                          → obrazky-figures/analyza/jsq_vs_toeplitz.png
            → literatura.bib
            = build/projekt.pdf
```

Prvý riadok kapitoly `% !TEX root = ../projekt.tex` hovorí VS Code, čo má kompilovať.
Preto funguje `Cmd+S` aj z otvorenej kapitoly. Tento riadok nemaž.

## Práca vo VS Code

| skratka | čo spraví |
|---|---|
| `Cmd+S` | uloží a skompiluje |
| `Cmd+Option+B` | skompiluje ručne |
| `Cmd+Option+V` | otvorí PDF vedľa textu |
| `Cmd+Option+J` | skočí z textu na miesto v PDF |
| `Cmd+klik v PDF` | skočí z PDF na riadok v texte |
| `Cmd+Shift+M` | zoznam chýb, oprav vždy prvú |

## Časté úkony

**Citácia:** zdroj pridaj do `literatura.bib` a v texte použi `\cite{microsoft-rss}`.

**Odkaz na kapitolu, obrázok alebo tabuľku:** `obrázok~\ref{fig:jsq}`.

**Graf:** ulož ho do správneho priečinka v `obrazky-figures/` a vlož:

```latex
\begin{figure}[ht]
  \centering
  \includegraphics[width=\textwidth]{analyza/moj_graf.png}
  \caption{Popis grafu.}
  \label{fig:moj-graf}
\end{figure}
```

**Tabuľka:** vytvor súbor v `tabulky/` a vlož ho cez `\input{tabulky/moja_tabulka}`.
Čísla v tabuľkách sa samy neaktualizujú. Po novom behu ich treba prepísať.

**Poznámka pre seba:** `\TODO{čo treba doplniť}` sa v PDF zobrazí červenou.

## Otvorené veci

- `\TODO` v texte: chýbajúce citácie, kapitola o NDK a výsledky variantov.
- Hodnoty pre `odid2`, `odid4`, `odid6` a iba IP sú orientačne odčítané zo starých grafov.
- Grafy `odid6` treba overiť. V starých výsledkoch boli zhodné s priečinkom označeným ako `odid1`,
  hoci skutočný `odid1` má iné hodnoty.
- Staré grafy majú anglické popisy.

## Pred odovzdaním

1. Vyrieš všetky `\TODO`.
2. V `projekt.tex` zmaž riadok `\nocite{*}`, inak sa vypíšu aj necitované zdroje.
3. V `osnova-makra.tex` zmeň `\osnovatrue` na `\osnovafalse`.
4. Pridaj `zadani.pdf` a v `projekt.tex` zapni `\documentclass[slovak,zadani]{fitthesis}`.
5. Doplň abstrakt, poďakovanie a prehlásenie vrátane použitia nástrojov AI.
6. Over názov práce podľa oficiálneho zadania.
