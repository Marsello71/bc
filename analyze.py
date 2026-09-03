import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import ScalarFormatter

from pathlib import Path
import sys

ALGOS = {"chaskey", "halfsiphash", "crc32c", "jhash"}

COLORS = {
    "toeplitz": "black", "jhash": "#4C72B0", "chaskey": "#55A868",
    "halfsiphash": "#C44E52", "crc32c": "#8172B2", "xorhash": "#CCB974",
}


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

def plot_metric_vs_channels(agg, metric, output_path, log_y=False):
    syms = ["none", "xorfold", "sortfold"]
    fig, axes = plt.subplots(1, 3, figsize=(16, 5), sharey=True, constrained_layout=True)

    for ax, sym in zip(axes, syms):
        sub = agg[agg["symmetry"] == sym]
        for algo, g in sub.groupby("algorithm"):
            g = g.sort_values("num_channels")
            ax.plot(g["num_channels"], g["mean"], marker="o", ms=4,
                    label=algo, color=COLORS.get(algo))
            ax.fill_between(g["num_channels"], g["mean"] - g["std"], g["mean"] + g["std"],
                            alpha=0.15, color=COLORS.get(algo))
        ax.set_title(sym)
        ax.set_xscale("log", base=2)
        ax.set_xticks([8, 16, 20, 32, 40, 64, 128])
        ax.xaxis.set_major_formatter(ScalarFormatter())
        ax.xaxis.set_minor_locator(plt.NullLocator())
        ax.set_xlabel("DMA channels")
        if log_y:
            ax.set_yscale("log")

    axes[0].set_ylabel("‰ over fair share" if metric == "thresshold_sum" else "normalized χ²")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=6, bbox_to_anchor=(0.5, -0.05))
    fig.suptitle(metric)
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
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

    # ciary: metrika vs pocet kanalov, jeden panel na symetriu
    combined = pd.concat(frames, ignore_index=True)
    for metric in ("thresshold_sum", "chi"):
        agg = aggregate_over_channels(combined, metric)
        plot_metric_vs_channels(agg, metric, outdir / f"{metric}_vs_channels.png", log_y=True)

if __name__ == "__main__":
    main()






















