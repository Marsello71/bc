import pandas as pd 
import matplotlib.pyplot as plt

from pathlib import Path

def load_results(csv_path : Path) -> pd.DataFrame:
    if not csv_path.exists() : 
        raise FileNotFoundError("missing file with results")
    return pd.read_csv(csv_path)

def plot_speed_comparison(data : pd.DataFrame, output_path : Path) -> None:
    plt.figure()
    sorted_data = data.sort_values("avg_time_ns")
    plt.bar(sorted_data["algorithm"],sorted_data["avg_time_ns"])
    plt.xlabel("algorithm")
    plt.ylabel("Average time per hash")
    plt.title("Comparison of Hash functions time for one hash")
    plt.savefig(output_path)
    plt.close()

def plot_hash_per_sec_comparison(data : pd.DataFrame, output_path : Path) -> None:
    plt.figure()
    sorted_data = data.sort_values("total_time_ns")
    time_in_seconds = sorted_data["total_time_ns"] / 1e9
    plt.bar(sorted_data["algorithm"],sorted_data["tuple_count"] / time_in_seconds)
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

def plot_fairness_comparison(data : pd.DataFrame, channel_cols : list[str], output_path : Path) -> None :
    plt.figure()
    data["fairness"] = compute_fairness_index(data, channel_cols)
    data = data.sort_values("fairness")
    plt.bar(data["algorithm"], data["fairness"])
    plt.xlabel("algorithm")
    plt.axhline(1.0, linestyle="--", color="gray")
    plt.title("Comparison of Hash Fairness Index")
    plt.ylabel("Jain's Fairness Index")
    margin = (data["fairness"].max() - data["fairness"].min()) * 0.1
    plt.ylim(data["fairness"].min() - margin, data["fairness"].max() + margin)
    plt.savefig(output_path)
    plt.close()

def compute_min_max (data : pd.DataFrame, channel_cols : list[str]) -> pd.Series :
    channels = data[channel_cols]
    sum_x_min= channels.min(axis = 1) 
    sum_x_max= channels.max(axis = 1)
    diffrence = sum_x_max - sum_x_min
    diffrence = diffrence.sort_values()
    return(diffrence/data["tuple_count"]) * 100

def plot_min_max_comparison(data : pd.DataFrame, channel_cols : list[str], output_path : Path) -> None :
    plt.figure()
    data["min_max_diff"] = compute_min_max(data, channel_cols)
    data = data.sort_values("min_max_diff")
    plt.bar(data["algorithm"], data["min_max_diff"])
    plt.xlabel("algorithm")
    plt.title("Comparison of min max diffrence in %")
    plt.ylabel("Diffrence ")
    plt.savefig(output_path)
    plt.close()

def main() :
    RESULTS_CSV = Path("results/perf_results.csv")
    data = load_results(RESULTS_CSV)
    channel_cols = [c for c in data.columns if c.startswith("channel_")]

    
    plot_speed_comparison(data, "results/speed_comparison.png")
    plot_hash_per_sec_comparison(data, Path("results/hash_per_sec_comparison.png"))
    plot_fairness_comparison(data,channel_cols, Path("results/fairness_comparison.png"))
    plot_min_max_comparison(data,channel_cols, Path("results/min_max_diff_comparison.png"))

if __name__ == "__main__":
    main()
























