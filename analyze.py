import pandas as pd 
import matplotlib.pyplot as plt

from pathlib import Path
import sys

CRYPTO_ALGOS = {"chaskey", "halfsiphash", "siphash", "ascon"}
NON_CRYPTO_ALGOS = {"crc32c", "jhash", "spookyhash"}


def load_results(csv_path : Path) -> pd.DataFrame:
    if not csv_path.exists() : 
        raise FileNotFoundError("missing file with results")
    return pd.read_csv(csv_path)

def aggregate_by_algorithm(data) -> pd.DataFrame :
    result = data.groupby(["algorithm","num_channels"]).agg(
    fairness_worst = ("fairness", "min"),
    fairness_median = ("fairness", "median"),
    min_max_diff_worst = ("min_max_diff", "max"),
    min_max_diff_median = ("min_max_diff", "median"),
    chi_worst = ("chi", "max"),
    chi_median = ("chi", "median"),
    avg_time_ns = ("avg_time_ns", "median"),
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
        ax1.plot(algo_data["num_channels"],algo_data[column], marker="o", label=name)

    for name in NON_CRYPTO_ALGOS : 
        algo_data = data[data["algorithm"] == name]
        algo_data = algo_data.sort_values("num_channels")
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
        
def main() :
    if len(sys.argv) != 3 : 
        sys.stderr.write("tu run the analysis main needs 3 argumenst: [dataset] [output_file]\n")
    RESULTS_CSV = Path(sys.argv[1])
    data = load_results(RESULTS_CSV)
    results = aggregate_by_algorithm(data)

    #plot_speed_comparison(results,Path(sys.argv[2] +  "speed_comparison.png"))
    #plot_hash_per_sec_comparison(results, Path(sys.argv[2] + "hash_per_sec_comparison.png"))

    plot_channel_scaling(results,"fairness_median", Path(sys.argv[2]) / "fairness_MEDIAN_comparison.png")
    plot_channel_scaling(results,"fairness_worst", Path(sys.argv[2]) / "fairness_WORST_comparison.png")

    plot_channel_scaling(results,"min_max_diff_median", Path(sys.argv[2]) / "min_max_MEDIAN_diff_comparison.png")
    plot_channel_scaling(results,"min_max_diff_worst", Path(sys.argv[2]) / "min_max_WORST_comparison.png")

    plot_channel_scaling(results,"chi_median", Path(sys.argv[2]) / "chi_square_MEDIAN_dif_comparison.png")
    plot_channel_scaling(results,"chi_worst", Path(sys.argv[2]) / "chi_square_WORST_dif_comparison.png")

if __name__ == "__main__":
    main()






















