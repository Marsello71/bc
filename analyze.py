import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

from pathlib import Path
import sys

CHANNELS = [8, 16, 20, 32, 40, 64, 128]

COLORS = {
    "toeplitz": "black", "jhash": "#4C72B0", "chaskey": "#55A868",
    "halfsiphash": "#C44E52", "crc32c": "#CCB974",
}
GROUP_A = ["chaskey", "halfsiphash"]   # krypto ARX
GROUP_B = ["crc32c", "jhash"]          # nekrypto
MARKERS = {"chaskey": "o", "crc32c": "s", "halfsiphash": "^", "jhash": "D"}
LABELS = {
    "chaskey": "Chaskey", "crc32c": "CRC32C",
    "halfsiphash": "HalfSipHash", "jhash": "jhash (lookup3)",
}
METRIC_NAME = {"thresshold_sum": "Channel overload", "chi": "Distribution χ²"}

BOX_ALGOS = ["toeplitz", "jhash", "chaskey", "halfsiphash", "crc32c"]
SYM_ORDER = ["none", "xorfold", "sortfold"]
SYM_LABELS = {"none": "none", "xorfold": "xor", "sortfold": "sort"}
SYM_COLORS = {"none": "#4C72B0", "xorfold": "#55A868", "sortfold": "#C44E52"}



def load_results(csv_path : Path) -> pd.DataFrame:
    if not csv_path.exists() : 
        raise FileNotFoundError("missing file with results")
    return pd.read_csv(csv_path)

def aggregate_by_algorithm_run_avg(data: pd.DataFrame, DMA: int) -> pd.DataFrame:
    filtered = data[data["num_channels"] == DMA]
    sym = filtered["symmetry"].iloc[0]   # kazdy vstupny subor = jedna symetria

    per_key = filtered.groupby(["algorithm", "key_id"]).agg(
        key_value=("thresshold_sum", "mean"),
    ).reset_index()

    rows = []
    for algorithm, group in per_key.groupby("algorithm"):
        rows.append({
            "symmetry": sym,
            "algorithm": algorithm,
            "typical_permille": group["key_value"].median(),
            "worst_permille": group["key_value"].max(),
        })

    per_algo = pd.DataFrame(rows)
    return per_algo.sort_values("typical_permille")

def aggregate_over_channels(data, metric):
    per_key = (data.groupby(["symmetry", "algorithm", "num_channels", "key_id"])[metric]
               .mean().reset_index())
    return (per_key.groupby(["symmetry", "algorithm", "num_channels"])[metric]
            .agg(mean="mean", std="std").reset_index())


def plot_threshold_bar(data0, data1, data2, output_dir: Path, DMA):
    datas = [data0, data1, data2]
    fig, axes = plt.subplots(2, 2, figsize=(12, 10), sharey=True,
                             constrained_layout=True)

    width = 0.35
    y_max = max(d[["typical_permille", "worst_permille"]].max().max() for d in datas)

    for ax, d in zip(axes.flat, datas):   # 3 panely, 4. bunka ostava legende
        x = range(len(d))
        mc = ["black"   if a == "toeplitz" else "#4C72B0" for a in d["algorithm"]]
        wc = ["dimgray" if a == "toeplitz" else "#DD8452" for a in d["algorithm"]]
        bm = ax.bar([i - width/2 for i in x], d["typical_permille"], width, color=mc)
        bw = ax.bar([i + width/2 for i in x], d["worst_permille"],   width, color=wc)
        ax.bar_label(bm, fmt="%.2f", padding=2, fontsize=7.5)
        ax.bar_label(bw, fmt="%.2f", padding=2, fontsize=7.5)
        ax.set_xticks(list(x)); ax.set_xticklabels(d["algorithm"], rotation=30, ha="right")
        ax.set_title(d["symmetry"].iloc[0])

    axes[0, 0].set_ylim(0, y_max * 1.15)

    legend_ax = axes[1, 1]
    legend_ax.axis("off")
    legend_ax.legend(handles=[
        Patch(color="#4C72B0", label="Typical key (median over keys)"),
        Patch(color="#DD8452", label="Worst key (max over keys)"),
        Patch(color="black",   label="Toeplitz — typical"),
        Patch(color="dimgray", label="Toeplitz — worst"),
    ], loc="center", frameon=False)

    for ax in axes[:, 0]:
        ax.set_ylabel("Packets over fair share [‰ of total packets]")

    fig.suptitle(f"Channel overload: typical vs worst key — {DMA} channels")
    fig.savefig(output_dir / f"overload_bar_{DMA}.png", dpi=150)
    plt.close(fig)

def _toeplitz_ratio(agg: pd.DataFrame) -> pd.DataFrame:
    """prida stlpec rel = mean / (Toeplitz mean pri rovnakej symetrii a poc. kanalov)"""
    ref = (agg[agg["algorithm"] == "toeplitz"]
           .rename(columns={"mean": "ref"})[["symmetry", "num_channels", "ref"]])
    out = agg.merge(ref, on=["symmetry", "num_channels"], how="left")
    out["rel"] = out["mean"] / out["ref"]
    return out


def plot_metric_vs_channels(agg: pd.DataFrame, metric: str, output_path: Path,
                            algos: list) -> None:
    data = _toeplitz_ratio(agg)
    name = METRIC_NAME.get(metric, metric)
    syms = ["none", "xorfold", "sortfold"]

    fig, axes = plt.subplots(2, 2, figsize=(11, 9), sharey=True, constrained_layout=True)

    rel = data[data["algorithm"].isin(algos)]["rel"].dropna()
    margin = max(0.02, (rel.max() - rel.min()) * 0.15)
    lo, hi = min(rel.min(), 1.0) - margin, max(rel.max(), 1.0) + margin

    for ax, sym in zip(axes.flat, syms):
        sub = data[data["symmetry"] == sym]
        ax.axhline(1.0, linestyle="--", linewidth=1, color="0.6", zorder=1)
        for algo in algos:
            g = sub[sub["algorithm"] == algo].sort_values("num_channels")
            if g.empty:
                continue
            xpos = [CHANNELS.index(c) for c in g["num_channels"]]
            ax.plot(xpos, g["rel"], marker=MARKERS[algo], markersize=5, linewidth=1.5,
                    color=COLORS[algo], label=LABELS[algo], zorder=3)
        ax.set_title(sym, fontsize=10, pad=6)
        ax.set_xticks(range(len(CHANNELS)))
        ax.set_xticklabels(CHANNELS)
        ax.set_ylim(lo, hi)
        ax.grid(True, axis="y", color="#e1e0d9", linewidth=0.6)
        ax.set_axisbelow(True)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

    axes[1, 1].axis("off")
    legend_handles = [plt.Line2D([], [], linestyle="--", color="0.6",
                                 label="Toeplitz (baseline = 1.0)")]
    legend_handles += [plt.Line2D([], [], marker=MARKERS[a], color=COLORS[a],
                                  label=LABELS[a]) for a in algos]
    axes[1, 1].legend(handles=legend_handles, loc="center", frameon=False)

    for ax in (axes[0, 0], axes[1, 0]):
        ax.set_ylabel(f"{name} relative to Toeplitz  (×)")
    for ax in (axes[1, 0], axes[0, 1]):
        ax.set_xlabel("DMA channels")

    fig.suptitle(f"{name}: deviation from Toeplitz across DMA channel counts",
                 fontsize=13)
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)

def aggregate_by_algorithm__for_box_plot(data: pd.DataFrame, DMA: int, metric : str) -> pd.DataFrame:
    filtered = data[data["num_channels"] == DMA]
    per_key = (filtered
           .groupby(["symmetry", "algorithm", "key_id"])[metric]
           .mean()
           .reset_index(name="key_mean"))

    return per_key

def plot_metric_boxplot(agg: pd.DataFrame, metric: str, output_path: Path,
                            algos: list) -> None:
    fig, ax = plt.subplots(figsize=(14, 7), constrained_layout=True)
    data, positions, colors = [], [], []
    for i, algo in enumerate(algos):
        for off, sym in zip((-0.27, 0.0, 0.27), SYM_ORDER):
            vals = agg[(agg.algorithm == algo) & (agg.symmetry == sym)]["key_mean"].values
            data.append(vals)
            positions.append(i + off)
            colors.append(SYM_COLORS[sym])


    bp = ax.boxplot(data, positions=positions, widths=0.22,
        whis=(0, 100), showfliers=False, patch_artist=True)

    for patch, c in zip(bp["boxes"], colors):
        patch.set_facecolor(c)
        patch.set_alpha(0.8)
    for m in bp["medians"]: 
        m.set_color("black")

    ax.set_xticks(range(len(algos)))
    ax.set_xticklabels(algos)
    ax.set_xlim(-0.6, len(algos) - 0.4)

    name = METRIC_NAME.get(metric, metric)
    ax.set_ylabel(f"{name} [‰ of total packets]" if metric == "thresshold_sum" else name)
    ax.grid(True, axis="y", color="#e1e0d9", linewidth=0.6)
    ax.set_axisbelow(True)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    handles = [Patch(facecolor=SYM_COLORS[s], alpha=0.8, label=SYM_LABELS[s])
           for s in SYM_ORDER]
    ax.legend(handles=handles, title="symmetry", frameon=False)

    ax.set_title(f"{name}: spread across 16 keys")
    fig.savefig(output_path,dpi = 150)
    plt.close(fig)

def main() :
    if len(sys.argv) != 6 :
        sys.stderr.write("usage: analyze.py <sym0.csv> <sym1.csv> <sym2.csv> <outdir> <DMA>\n")
        sys.exit(1)

    outdir = Path(sys.argv[4])
    outdir.mkdir(parents=True, exist_ok=True)
    DMA = int(sys.argv[5])

    frames = [load_results(Path(p)) for p in sys.argv[1:4]]

    # bar: typicky vs najhorsi kluc, jeden panel na symetriu, fixny DMA
    results = [aggregate_by_algorithm_run_avg(f, DMA) for f in frames]
    plot_threshold_bar(*results, outdir, DMA)

    # ciary: pomer metriky k Toeplitzu vs pocet kanalov, jeden panel na symetriu.
    # dve dvojice algo zvlast - inak je v paneli prilis vela ciar
    combined = pd.concat(frames, ignore_index=True)
    for metric in ("thresshold_sum", "chi"):
        agg = aggregate_over_channels(combined, metric)
        plot_metric_vs_channels(agg, metric, outdir / f"{metric}_vs_channels_A.png", GROUP_A)
        plot_metric_vs_channels(agg, metric, outdir / f"{metric}_vs_channels_B.png", GROUP_B)

    for metric in ("thresshold_sum", "chi"):
        agg = aggregate_by_algorithm__for_box_plot(combined, DMA, metric)
        plot_metric_boxplot(agg, metric, outdir / f"{metric}_vs_keys_box_{DMA}.png",BOX_ALGOS)
        
if __name__ == "__main__":
    main()






















