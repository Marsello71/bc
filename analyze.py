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

"""
def aggregate_by_algorithm(data) -> pd.DataFrame :
    result = data.groupby(["algorithm","num_channels","tuple_run_index"]).agg(
    fairness_worst = ("fairness", "min"),
    fairness_median = ("fairness", "median"),
    min_max_diff_worst = ("min_max_diff", "max"),
    min_max_diff_median = ("min_max_diff", "median"),
    chi_worst = ("chi", "max"),
    chi_median = ("chi", "median"),
    #avg_time_ns = ("avg_time_ns", "median"),
    max_diff_median = ("max_diff", "median"),
    max_diff_worst = ("max_diff", "max"),
    ).reset_index()

    return result

def plot_speed_comparison(data : pd.DataFrame, output_path : Path) -> None:
    plt.figure()
    sorted_data = data.sort_values("avg_time_ns")
    plt.bar(sorted_data["algorithm"],sorted_data["avg_time_ns"])
    plt.xticks(fontsize=8)
    plt.xlabel("algorithm")
    plt.ylabel("Average time per hash")
    plt.title("Comparison of Hash functions time for one hash")
    plt.savefig(output_path)
    plt.close()

def plot_hash_per_sec_comparison(data : pd.DataFrame, output_path : Path) -> None:
    plt.figure()
    sorted_data = data.sort_values("avg_time_ns")
    hash_per_sec = 1e9 / sorted_data["avg_time_ns"]
    plt.bar(sorted_data["algorithm"], hash_per_sec)
    plt.xticks(fontsize=8)
    plt.xlabel("algorithm")
    plt.ylabel("Hash per second")
    plt.title("Comparison of Hash per second chart")
    plt.savefig(output_path)
    plt.close()

def plot_channel_scaling(data : pd.DataFrame, column : str,output_path : Path) -> None: 
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    channels = sorted(data["num_channels"].unique())
    ax1.set_xticks(channels)
    ax2.set_xticks(channels)

    for name in CRYPTO_ALGOS : 
        algo_data = data[data["algorithm"] == name]
        algo_data = algo_data.sort_values("num_channels")
        if name == "toeplitz" : 
            ax1.plot(algo_data["num_channels"],algo_data[column], marker="o", label=name, linestyle = "--", color="black")
        else :
            ax1.plot(algo_data["num_channels"],algo_data[column], marker="o", label=name)

    for name in NON_CRYPTO_ALGOS : 
        algo_data = data[data["algorithm"] == name]
        algo_data = algo_data.sort_values("num_channels")
        if name == "toeplitz" : 
            ax2.plot(algo_data["num_channels"],algo_data[column], marker="o", label=name, linestyle = "--", color="black")
        else :
            ax2.plot(algo_data["num_channels"],algo_data[column], marker="o", label=name)

    ax1.set_title("Cryptogrphic")
    ax2.set_title("Non-cryptogrphic")
    ax1.set_xlabel("Number of DMA channels")
    ax2.set_xlabel("Number of DMA channels")
    ax1.set_ylabel(column)
    ax2.set_ylabel(column)

    ax1.legend()
    ax2.legend()
    fig.suptitle(f"Scaling by number of channels — {column}")

    fig.savefig(output_path)
    plt.close(fig)
"""
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

    """
    #plot_speed_comparison(results,Path(sys.argv[2] +  "speed_comparison.png"))
    #plot_hash_per_sec_comparison(results, Path(sys.argv[2] + "hash_per_sec_comparison.png"))

    plot_channel_scaling(results,"fairness_median", Path(sys.argv[2]) / "fairness_MEDIAN_comparison.png")
    plot_channel_scaling(results,"fairness_worst", Path(sys.argv[2]) / "fairness_WORST_comparison.png")

    plot_channel_scaling(results,"min_max_diff_median", Path(sys.argv[2]) / "min_max_MEDIAN_diff_comparison.png")
    plot_channel_scaling(results,"min_max_diff_worst", Path(sys.argv[2]) / "min_max_WORST_comparison.png")

    plot_channel_scaling(results,"chi_median", Path(sys.argv[2]) / "chi_square_MEDIAN_dif_comparison.png")
    plot_channel_scaling(results,"chi_worst", Path(sys.argv[2]) / "chi_square_WORST_dif_comparison.png")
    """

    """
    plot_algo_grid(results, "fairness_median", "fairness_worst", CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "fairness_CRYPTO_grid.png", DMA,
                    "Fairness index (0–1, 1 = perfectly fair)")
    plot_algo_grid(results, "fairness_median", "fairness_worst", NON_CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "fairness_NONCRYPTO_grid.png", DMA,
                    "Fairness index (0–1, 1 = perfectly fair)")

    plot_algo_grid(results, "min_max_diff_median", "min_max_diff_worst", CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "min_max_diff_CRYPTO_grid.png", DMA,
                    "Channel load range: max−min as % of total tuples")
    plot_algo_grid(results, "min_max_diff_median", "min_max_diff_worst", NON_CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "min_max_diff_NONCRYPTO_grid.png", DMA,
                    "Channel load range: max−min as % of total tuples")

    plot_algo_grid(results, "chi_median", "chi_worst", CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "chi_CRYPTO_grid.png", DMA,
                    "Normalized χ² statistic\n(χ²/tuple_count, lower = more uniform)")
    plot_algo_grid(results, "chi_median", "chi_worst", NON_CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "chi_NONCRYPTO_grid.png", DMA,
                    "Normalized χ² statistic\n(χ²/tuple_count, lower = more uniform)")

    plot_algo_grid(results, "max_diff_median", "max_diff_worst", CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "max_avg_diff_CRYPTO_grid.png", DMA,
                    "Peak channel overload \n(fraction above fair share, 0 = perfectly fair)")
    plot_algo_grid(results, "max_diff_median", "max_diff_worst", NON_CRYPTO_ALGOS, 2, 2,
                    Path(sys.argv[2]) / "max_avg_diff_NONCRYPTO_grid.png", DMA,
                    "Peak channel overload \n(fraction above fair share, 0 = perfectly fair)")
    """



if __name__ == "__main__":
    main()






















