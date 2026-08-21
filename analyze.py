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

def main() :
    RESULTS_CSV = Path("results/perf_results.csv")
    OUTPUT_PNG = Path("results/speed_comparison.png")
    data = load_results(RESULTS_CSV)
    plot_speed_comparison(data, OUTPUT_PNG)
    print(f"Chart was saved in {OUTPUT_PNG}")

    plot_hash_per_sec_comparison(data, Path("results/hash_per_sec_comparison.png"))

if __name__ == "__main__":
    main()
























