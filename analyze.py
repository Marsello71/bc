import pandas as pd 
import matplotlib.pyplot as plt

from pathlib import Path

def load_results(csv_path : Path) -> pd.DataFrame:
    if not csv_path.exists() : 
        raise FileNotFoundError("missing file with results")
    return pd.read_csv(csv_path)

def aggregate_by_algorithm(data, channel_cols) -> pd.DataFrame :
    data["fairness"] = compute_fairness_index(data, channel_cols)
    data["min_max_diff"] = compute_min_max(data, channel_cols)
    data["chi"] = compute_chi(data, channel_cols)

    result = data.groupby("algorithm").agg(
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

def compute_fairness_index(data : pd.DataFrame, channel_cols : list[str]) -> pd.Series :
    channels = data[channel_cols]
    sum_x = channels.sum(axis = 1) 
    sum_x_sq = (channels ** 2).sum(axis=1)
    n = len(channel_cols)
    return sum_x ** 2 / (n * sum_x_sq)

def plot_fairness_comparison(data : pd.DataFrame, column : str, output_path : Path) -> None :
    plt.figure()
    data = data.sort_values(column,ascending=False)
    plt.bar(data["algorithm"], data[column])
    plt.xticks(fontsize=8)
    plt.xlabel("algorithm")
    plt.axhline(1.0, linestyle="--", color="gray")
    plt.title(f"Comparison of Hash {column} Index")
    plt.ylabel("Jain's Fairness Index Normalized ")
    margin = (data[column].max() - data[column].min()) * 0.1
    plt.ylim(data[column].min() - margin, data[column].max() + margin)
    plt.savefig(output_path)
    plt.close()

def compute_min_max (data : pd.DataFrame, channel_cols : list[str]) -> pd.Series :
    channels = data[channel_cols]
    sum_x_min= channels.min(axis = 1) 
    sum_x_max= channels.max(axis = 1)
    diffrence = sum_x_max - sum_x_min
    diffrence = diffrence.sort_values()
    return(diffrence/data["tuple_count"]) * 100

def plot_min_max_comparison(data : pd.DataFrame, column : str, output_path : Path) -> None :
    plt.figure()
    data = data.sort_values(column)
    plt.bar(data["algorithm"], data[column])
    plt.xticks(fontsize=8)
    plt.xlabel("algorithm")
    plt.title(f"Comparison of {column} in %")
    plt.ylabel("Diffrence ")
    plt.savefig(output_path)
    plt.close()

def compute_chi (data : pd.DataFrame, channel_cols : list[str]) -> pd.Series :
    expected = data["tuple_count"]/ len(channel_cols)
    observed = data[channel_cols]
    diff = observed.sub(expected,axis=0)
    chi2 = ((diff ** 2).div(expected,axis=0)).sum(axis=1)
    return chi2 / data["tuple_count"]                   

def plot_chi_comparison(data : pd.DataFrame, column : str, output_path : Path) -> None :
    plt.figure()
    data = data.sort_values(column)
    plt.bar(data["algorithm"], data[column])
    plt.xticks(fontsize=8)
    plt.xlabel("algorithm")
    plt.title(f"Comparison of {column}")
    plt.ylabel(f"Normalized {column}")
    plt.savefig(output_path)
    plt.close()

def main() :
    RESULTS_CSV = Path("results/perf_results.csv")
    data = load_results(RESULTS_CSV)
    channel_cols = [c for c in data.columns if c.startswith("channel_")]
    results = aggregate_by_algorithm(data,channel_cols)

    plot_speed_comparison(results, "results/speed_comparison.png")
    plot_hash_per_sec_comparison(results, Path("results/hash_per_sec_comparison.png"))

    plot_fairness_comparison(results,"fairness_median", Path("results/fairness_MEDIAN_comparison.png"))
    plot_fairness_comparison(results,"fairness_worst", Path("results/fairness_WORST_comparison.png"))

    plot_min_max_comparison(results,"min_max_diff_median", Path("results/min_max_MEDIAN_diff_comparison.png"))
    plot_min_max_comparison(results,"min_max_diff_worst", Path("results/min_max_WORST_comparison.png"))

    plot_chi_comparison(results,"chi_median", Path("results/chi_square_MEDIAN_dif_comparison.png"))
    plot_chi_comparison(results,"chi_worst", Path("results/chi_square_WORST_dif_comparison.png"))

if __name__ == "__main__":
    main()






















