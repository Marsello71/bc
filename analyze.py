import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

from pathlib import Path
from matplotlib.colors import to_rgba
import sys

CHANNELS = [8, 16, 20, 32, 40, 64, 128]

COLORS = {
    "toeplitz": "black", "jhash": "#4C72B0", "chaskey": "#55A868",
    "halfsiphash": "#C44E52", "multiplyshift": "#CCB974",
}
MARKERS = {"chaskey": "o", "multiplyshift": "s", "halfsiphash": "^", "jhash": "D"}
LABELS = {
    "chaskey": "Chaskey", "multiplyshift": "multiply-shift (NH)",
    "halfsiphash": "HalfSipHash", "jhash": "jhash (lookup3)",
}
BOX_ALGOS = ["toeplitz", "jhash", "chaskey", "halfsiphash", "multiplyshift"]

SYM_ORDER = ["none", "xorfold", "sortfold"]
SYM_LABELS = {"none": "none", "xorfold": "xor", "sortfold": "sort"}

# vaha zataze na kanal - hlavna os celej analyzy
#   flow   - 1 jednotka na smer toku (spravodlivost hashu, bez vplyvu elephant flow-ov)
#   packet - 1 jednotka na realny paket
#   byte   - velkost paketu v bajtoch
WEIGHT_ORDER = ["flow", "packet", "byte"]
WEIGHT_LABELS = {"flow": "per flow", "packet": "per packet", "byte": "per byte"}


def load_results(results_dir: Path) -> pd.DataFrame:
    files = sorted(results_dir.glob("run_sym*.csv"))
    if not files:
        raise FileNotFoundError(f"Nenašiel som 'run_sym*.csv' v {results_dir}")
    frames = []
    for f in files:
        frame = pd.read_csv(f)
        if frame.empty:                      # vaha co v tomto behu nedala ani jedno okno
            continue
        if "weighting" not in frame.columns:  # stare CSV z pred zavedenia vahy
            frame["weighting"] = "flow"
        frames.append(frame)
    return pd.concat(frames, ignore_index=True)


def aggregate_over_channels(data: pd.DataFrame, metric: str) -> pd.DataFrame:
    """priemer cez okna, potom priemer cez kluce -> jedna hodnota na (algoritmus, pocet kanalov)"""
    per_key = (data.groupby(["algorithm", "num_channels", "key_id"])[metric]
               .mean().reset_index())
    return (per_key.groupby(["algorithm", "num_channels"])[metric]
            .agg(mean="mean", std="std").reset_index())


def _toeplitz_ratio(agg: pd.DataFrame) -> pd.DataFrame:
    """prida stlpec rel = mean / (Toeplitz mean pri rovnakom pocte kanalov)"""
    ref = (agg[agg["algorithm"] == "toeplitz"]
           .rename(columns={"mean": "ref"})[["num_channels", "ref"]])
    out = agg.merge(ref, on="num_channels", how="left")
    out["rel"] = out["mean"] / out["ref"]
    return out


def plot_overload_grid(data: pd.DataFrame, sym: str, output_path: Path) -> None:
    """
    2x3: riadok 1 = absolutne thresshold_sum (log os), riadok 2 = pomer k Toeplitzu.
    stlpce = flow / packet / byte, pri fixnej symetrii.
    Toto je hlavny graf - v jednom obrazku vidno aj urovnovu zmenu (elephant flow-y
    zdvihnu cele riadok 1), aj to ci sa krivky v riadku 2 zbiehaju k 1.0 (hash prestava
    byt dolezity).
    """
    sub_sym = data[data["symmetry"] == sym]
    fig, axes = plt.subplots(2, 3, figsize=(15, 9), sharex=True, constrained_layout=True)

    ratio_algos = [a for a in BOX_ALGOS if a != "toeplitz"]  # toeplitz je v rade 2 vzdy 1.0

    # prvy priechod: spocitaj agg/ratio pre kazdu vahu a zapamataj si spolocny
    # rozsah pre riadok 2 - bez spolocnej Y osi sa "zbiehanie" nedalo vidiet,
    # len docitat z rozdielnych cisel na kazdej osi
    per_weighting = {}
    rel_min, rel_max = 1.0, 1.0
    for w in WEIGHT_ORDER:
        sub_w = sub_sym[sub_sym["weighting"] == w]
        if sub_w.empty:
            per_weighting[w] = None
            continue
        agg = aggregate_over_channels(sub_w, "thresshold_sum")
        ratio = _toeplitz_ratio(agg)
        per_weighting[w] = (agg, ratio)
        rel = ratio[ratio["algorithm"].isin(ratio_algos)]["rel"].dropna()
        if not rel.empty:
            rel_min = min(rel_min, rel.min())
            rel_max = max(rel_max, rel.max())
    margin = max(0.005, (rel_max - rel_min) * 0.1)
    rel_lo, rel_hi = rel_min - margin, rel_max + margin

    for col, w in enumerate(WEIGHT_ORDER):
        ax_abs = axes[0, col]
        ax_rel = axes[1, col]

        if per_weighting[w] is None:   # tato vaha nemala ani jedno cele okno - nic sa nekresli
            for ax in (ax_abs, ax_rel):
                ax.axis("off")
                ax.text(0.5, 0.5, "no windows\n(not enough traffic\nfor this weighting)",
                        ha="center", va="center", fontsize=9, color="0.5",
                        transform=ax.transAxes)
            ax_abs.set_title(WEIGHT_LABELS[w], fontsize=11)
            continue

        agg, ratio = per_weighting[w]

        for algo in BOX_ALGOS:
            g = agg[agg["algorithm"] == algo].sort_values("num_channels")
            if g.empty:
                continue
            xpos = [CHANNELS.index(c) for c in g["num_channels"]]
            ax_abs.plot(xpos, g["mean"], marker=MARKERS.get(algo, "o"), color=COLORS[algo],
                        label=LABELS.get(algo, algo), linewidth=1.5, markersize=5)
        ax_abs.set_yscale("log")
        ax_abs.set_title(WEIGHT_LABELS[w], fontsize=11)
        ax_abs.grid(True, axis="y", which="both", color="#e1e0d9", linewidth=0.6)
        ax_abs.set_axisbelow(True)

        ax_rel.axhline(1.0, linestyle="--", linewidth=1, color="0.6", zorder=1)
        for algo in ratio_algos:
            g = ratio[ratio["algorithm"] == algo].sort_values("num_channels")
            if g.empty:
                continue
            xpos = [CHANNELS.index(c) for c in g["num_channels"]]
            ax_rel.plot(xpos, g["rel"], marker=MARKERS.get(algo, "o"), color=COLORS[algo],
                        linewidth=1.5, markersize=5, zorder=3)
        ax_rel.grid(True, axis="y", color="#e1e0d9", linewidth=0.6)
        ax_rel.set_axisbelow(True)
        ax_rel.set_ylim(rel_lo, rel_hi)
        ax_rel.set_xticks(range(len(CHANNELS)))
        ax_rel.set_xticklabels(CHANNELS)
        ax_rel.set_xlabel("DMA channels")

    axes[0, 0].set_ylabel("Channel overload [‰] (log)")
    axes[1, 0].set_ylabel("Relative to Toeplitz (×)")

    handles = [plt.Line2D([], [], marker=MARKERS.get(a, "o"), color=COLORS[a],
                          label=LABELS.get(a, a)) for a in BOX_ALGOS]
    fig.legend(handles=handles, loc="upper center", ncol=len(BOX_ALGOS),
              frameon=False, bbox_to_anchor=(0.5, 1.05))

    fig.suptitle(f"Channel overload — symmetry: {SYM_LABELS[sym]}", y=1.1, fontsize=13)
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def prep_spread(data: pd.DataFrame, sym: str) -> pd.DataFrame:
    """pre kazdu (vaha, pocet kanalov): rozdiel najhorsi - najlepsi algoritmus.
    ak je tento rozdiel maly, na vybere hashu nezalezi."""
    sub = data[data["symmetry"] == sym]
    per_key = (sub.groupby(["weighting", "algorithm", "num_channels", "key_id"])["thresshold_sum"]
               .mean().reset_index())
    per_algo = (per_key.groupby(["weighting", "algorithm", "num_channels"])["thresshold_sum"]
                .mean().reset_index())
    return (per_algo.groupby(["weighting", "num_channels"])["thresshold_sum"]
            .agg(spread=lambda s: s.max() - s.min()).reset_index())


def plot_spread(data: pd.DataFrame, output_path: Path) -> None:
    """1x3 (symetrie), v kazdom panely 3 ciary (vahy) - rozpatie thresshold_sum
    medzi najlepsim a najhorsim algoritmom. Toto je graf, co priamo odpovie na
    otazku 'zalezi na hashi': ak ciary padaju k nule idesr flow -> packet -> byte,
    zalezi menej a menej."""
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.5), sharey=True, constrained_layout=True)

    for ax, sym in zip(axes, SYM_ORDER):
        sp = prep_spread(data, sym)
        for w in WEIGHT_ORDER:
            g = sp[sp["weighting"] == w].sort_values("num_channels")
            if g.empty:
                continue
            xpos = [CHANNELS.index(c) for c in g["num_channels"]]
            ax.plot(xpos, g["spread"], marker="o", markersize=5, linewidth=1.5,
                    label=WEIGHT_LABELS[w])
        ax.set_xticks(range(len(CHANNELS)))
        ax.set_xticklabels(CHANNELS)
        ax.set_xlabel("DMA channels")
        ax.set_title(SYM_LABELS[sym], fontsize=11)
        ax.grid(True, axis="y", color="#e1e0d9", linewidth=0.6)
        ax.set_axisbelow(True)

    axes[0].set_ylabel("Spread: worst − best algorithm [‰]")
    axes[-1].legend(frameon=False, title="weighting")

    fig.suptitle("the hash spread (worst - best)", fontsize=13)
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def aggregate_by_algorithm__for_box_plot(data: pd.DataFrame, DMA: int, metric: str) -> pd.DataFrame:
    filtered = data[data["num_channels"] == DMA]
    per_key = (filtered
           .groupby(["symmetry", "algorithm", "key_id"])[metric]
           .mean()
           .reset_index(name="key_mean"))

    return per_key

def plot_metric_boxplot(agg, metric, output_path, algos, DMA):
    fig, axes = plt.subplots(2, 2, figsize=(13, 10), constrained_layout=True)
    n_keys = agg["key_id"].nunique()

    for ax, sym in zip(axes.flat, SYM_ORDER):
        sub = agg[agg["symmetry"] == sym]
        if sub.empty:             # ziadne okna pre tuto vahu/symetriu
            ax.axis("off")
            ax.text(0.5, 0.5, "no windows\n(not enough traffic\nfor this weighting)",
                    ha="center", va="center", fontsize=9, color="0.5",
                    transform=ax.transAxes)
            ax.set_title(SYM_LABELS[sym], fontsize=12, fontweight="bold")
            continue
        data = [sub.loc[sub["algorithm"] == a, "key_mean"].values for a in algos]

        bp = ax.boxplot(
            data,
            widths=0.35,
            whis=(0, 100),
            showfliers=False,
            patch_artist=True,
            medianprops=dict(color="black", linewidth=1.6),
            whiskerprops=dict(color="0.4", linewidth=1.0),
            capprops=dict(color="0.4", linewidth=1.0),
        )
        for patch, a in zip(bp["boxes"], algos):
            patch.set_facecolor(to_rgba(COLORS[a], 0.40))
            patch.set_edgecolor(COLORS[a])
            patch.set_linewidth(1.4)

        for i, med in enumerate(bp["medians"]):
            ax.plot(i + 1, med.get_ydata()[0], marker="D", color="black",
                    markersize=5, zorder=5)

        ax.set_title(SYM_LABELS[sym], fontsize=12, fontweight="bold")
        ax.set_xticks(range(1, len(algos) + 1))
        ax.set_xticklabels(algos, rotation=25, ha="right")
        ax.set_ylabel(f"{metric} [‰ of fair share]")
        ax.grid(True, axis="y", color="#e1e0d9", linewidth=0.6)
        ax.set_axisbelow(True)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
        ax.margins(y=0.12)

    lax = axes.flat[3]                       # 4. bunka = legenda + vysvetlivka
    lax.axis("off")
    handles = [Patch(facecolor=to_rgba(COLORS[a], 0.35), edgecolor=COLORS[a],
                     linewidth=1.4, label=a) for a in algos]
    handles.append(plt.Line2D([], [], marker="D", color="black", linestyle="none",
                              markersize=5, label="median"))
    lax.legend(handles=handles, title="Algorithm", frameon=False,
               loc="upper center", fontsize=10)
    lax.text(0.5, 0.44,
             "box  =  25th–75th percentile across keys\n"
             "whiskers  =  best and worst key\n"
             "diamond / line  =  median\n"
             f"each box summarises {n_keys} RSS keys\n"
             "note: every panel has its own Y scale",
             transform=lax.transAxes, ha="center", va="top",
             fontsize=9, color="0.35")

    fig.suptitle(f"Channel overload: per-key spread by algorithm — {DMA} DMA channels, flow weighting",
                 fontsize=14, fontweight="bold")
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: analyze.py <results_dir> <outdir>\n")
        sys.exit(1)

    results_dir = Path(sys.argv[1])
    outdir = Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    combined_all = load_results(results_dir)

    # A: 3 obrazky (jeden na symetriu), v kazdom flow/packet/byte vedla seba
    for sym in SYM_ORDER:
        plot_overload_grid(combined_all, sym, outdir / f"overload_{sym}.png")

    # B: zalezi na hashi? jeden obrazok, rozpatie algoritmov flow -> packet -> byte
    plot_spread(combined_all, outdir / "spread.png")

    # C: citlivost na RSS kluc pri flow vahe (cista kvalita hashu, bez elephant flow-ov)
    flow_only = combined_all[combined_all["weighting"] == "flow"]
    for dma in (8, 40, 128):
        agg = aggregate_by_algorithm__for_box_plot(flow_only, dma, "thresshold_sum")
        plot_metric_boxplot(agg, "thresshold_sum", outdir / f"keys_{dma}.png", BOX_ALGOS, dma)


if __name__ == "__main__":
    main()
