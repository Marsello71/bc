import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

from pathlib import Path
import sys

CRYPTO_ALGOS = {"chaskey", "halfsiphash", "siphash", "jhash"}
NON_CRYPTO_ALGOS = {"crc32c", "ascon", "spookyhash", "nsgahash4"}


def load_results(csv_path : Path) -> pd.DataFrame:
    if not csv_path.exists() : 
        raise FileNotFoundError("missing file with results")
    return pd.read_csv(csv_path)

def aggregate_by_algorithm_run_avg(data: pd.DataFrame, DMA: int) -> pd.DataFrame:
    filtered = data[data["num_channels"] == DMA]

    # key_value = priemer thresshold_sum cez vsetky okna toho kluca.
    # thresshold_sum uz je v promile (benchmark.cpp: sum / (WINDOW_SIZE/1000)),
    # takze priemer cez okna = promile z celkoveho poctu paketov toho behu.
    per_key = filtered.groupby(["algorithm", "key_id"]).agg(
        key_value=("thresshold_sum", "mean"),
    ).reset_index()

    rows = []
    for algorithm, group in per_key.groupby("algorithm"):
        typical_permille = group["key_value"].median()  # typicky kluc
        worst_permille = group["key_value"].max()        # najhorsi kluc
        rows.append({
            "algorithm": algorithm,
            "typical_permille": typical_permille,
            "worst_permille": worst_permille,
        })

    per_algo = pd.DataFrame(rows)
    return per_algo.sort_values("typical_permille")


def plot_algo_grid(data, median_col, worst_col, algos, nrows, ncols, output_path, DMA, y_label) -> None:

    fig, axes = plt.subplots(nrows, ncols, figsize=(12, 8), sharey=True, constrained_layout=True)
    toeplitz_data = data[(data["algorithm"] == "toeplitz") & (data["num_channels"] == DMA)]

    for ax, name in zip(axes.flat, algos) :
        algo_data = data[(data["algorithm"] == name) & (data["num_channels"] == DMA)]
        ax.plot(toeplitz_data["tuple_run_index"],toeplitz_data[median_col], label="Toeplitz", linestyle = "--", color="black")
        ax.plot(algo_data["tuple_run_index"],algo_data[worst_col], label="Worst")
        ax.plot(algo_data["tuple_run_index"],algo_data[median_col], label="Median")
        ax.set_xlabel("Window index (1 window = 100,000 tuples)")
        ax.set_title(name)
        ax.ticklabel_format(axis="y", useOffset=False, style="plain")

    for ax in axes[:, 0]:
        ax.set_ylabel(y_label)
        
    fig.suptitle(f"{median_col} on {DMA} channels")

    handles, labels = axes.flat[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper right")

    fig.savefig(output_path)
    plt.close(fig)

def plot_threshold_bar(data: pd.DataFrame, output_path: Path, DMA: int) -> None:
    """
    2 stlpce na algoritmus: typical_permille (modry) a worst_permille (oranzovy).
    Toeplitz dostava rovnaku dvojicu stlpcov, ale cierno/sivu farbu (baseline).
    """
    fig, ax = plt.subplots(figsize=(10, 5.5), constrained_layout=True)

    x = range(len(data))
    width = 0.35

    median_colors = ["black" if a == "toeplitz" else "#4C72B0" for a in data["algorithm"]]
    worst_colors = ["dimgray" if a == "toeplitz" else "#DD8452" for a in data["algorithm"]]

    bars_median = ax.bar([i - width / 2 for i in x], data["typical_permille"], width,
                          label="Typical key (median over keys)", color=median_colors)
    bars_worst = ax.bar([i + width / 2 for i in x], data["worst_permille"], width,
                         label="Worst key (max over keys)", color=worst_colors)

    ax.bar_label(bars_median, fmt="%.2f", padding=2, fontsize=7)
    ax.bar_label(bars_worst, fmt="%.2f", padding=2, fontsize=7)

    ax.set_xticks(list(x))
    ax.set_xticklabels(data["algorithm"])
    y_max = max(data["typical_permille"].max(), data["worst_permille"].max())
    ax.set_ylim(0, y_max * 1.12)
    ax.set_ylabel("Packets over fair share [‰ of total packets]")
    ax.set_xlabel("Algorithm")
    ax.set_title(f"Channel overload: typical key vs. worst key — {DMA} channels")

    handles, labels = ax.get_legend_handles_labels()
    handles += [
        Patch(color="black", label="Toeplitz — typical"),
        Patch(color="dimgray", label="Toeplitz — worst"),
    ]
    ax.legend(handles=handles)

    fig.savefig(output_path)
    plt.close(fig)


def main() :
    if len(sys.argv) != 4 : 
        sys.stderr.write("tu run the analysis main needs 4 argumenst: [dataset] [output_file] [DMA channels]\n")
    RESULTS_CSV = Path(sys.argv[1])
    data = load_results(RESULTS_CSV)
    DMA = int(sys.argv[3])
    results = aggregate_by_algorithm_run_avg(data, DMA)

    plot_threshold_bar(results, Path(sys.argv[2] + "overload_comparison.png"), DMA)

if __name__ == "__main__":
    main()






















