"""Draw the results of one simulation.

usage: python analyze.py config.conf

Reads the output CSV named by `out` in the config file and saves a plot next
to it (result.csv -> result.png). Every point is the mean over all windows.
"""

import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # write image files, no window needed
import matplotlib.pyplot as plt
import pandas as pd

# method in the CSV -> line color
COLORS = {"hash": "#2a78d6", "jsq": "#eb6834"}
MARKERS = {"hash": "o", "jsq": "s"}

TEXT = "#1f1f1e"
MUTED = "#6b6a63"
GRID = "#e6e5e0"
SURFACE = "#fcfcfb"

# CSV column -> panel title
METRICS = {
    "overload": "Overload (‰)",
    "fairness": "Jain fairness",
    "chi": "Normalised χ²",
}

# names from the config file -> names on the plot
HASH_NAMES = {
    "toeplitz": "Toeplitz", "jhash": "jhash", "chaskey": "Chaskey",
    "halfsiphash": "HalfSipHash", "simpletab": "Simple tabulation",
    "multiplyshift": "Multiply-shift", "siphash": "SipHash", "xorhash": "XOR hash",
}
TUPLE_NAMES = {"full": "full tuple", "ip": "IP-only tuple"}
SYMMETRY_NAMES = {"none": "no symmetry", "xor": "XOR symmetry", "sort": "sorted symmetry"}


def read_config(path):
    """Read 'key = value' lines into a dictionary, without comments."""
    config = {}
    for line in Path(path).read_text().splitlines():
        line = line.split("#")[0]
        if "=" in line:
            key, value = line.split("=", 1)
            config[key.strip()] = value.strip()
    return config


def mean_over_windows(data):
    """One row per (method, channel count): the mean of every metric."""
    columns = list(METRICS)
    return data.groupby(["method", "num_channels"])[columns].mean().reset_index()


def print_table(summary):
    """The same numbers as the plot, as text."""
    table = summary.pivot(index="num_channels", columns="method", values=list(METRICS))
    print(table.round(4).to_string())


def number(text):
    """150000 -> 150 000"""
    return f"{int(text):,}".replace(",", "\u202f")


def plot_subtitle(config):
    """The settings of this run, in one line."""
    if config.get("scaling") == "on":
        window = f"{number(config['can_size'])} packets per channel in a window"
    else:
        window = f"{number(config['window'])} packets in a window"
    parts = [
        Path(config["data"]).name,
        TUPLE_NAMES[config.get("tuple", "full")],
        SYMMETRY_NAMES[config.get("symmetry", "none")],
        f"RETA {config.get('reta', 'on')}",
        f"{config.get('weighting', 'packet')} weighting",
        f"{config.get('keys', '16')} keys",
        window,
    ]
    return "  ·  ".join(parts)


def plot(summary, config, output_path):
    channels = sorted(summary["num_channels"].unique())
    positions = {n: i for i, n in enumerate(channels)}  # evenly spaced channel counts
    hash_name = config.get("hash", "toeplitz")
    hash_name = HASH_NAMES.get(hash_name, hash_name)
    labels = {"hash": hash_name, "jsq": "JSQ oracle"}

    fig, axes = plt.subplots(1, 3, figsize=(15, 5), facecolor=SURFACE)

    for ax, (metric, title) in zip(axes, METRICS.items()):
        ax.set_facecolor(SURFACE)

        for method in ("hash", "jsq"):
            line = summary[summary["method"] == method].sort_values("num_channels")
            x = [positions[n] for n in line["num_channels"]]
            ax.plot(x, line[metric], color=COLORS[method], marker=MARKERS[method],
                    linewidth=2, markersize=7, markeredgecolor=SURFACE,
                    markeredgewidth=1.5, label=labels[method])

        ax.set_title(title, color=TEXT, fontsize=11, loc="left")
        ax.set_xticks(range(len(channels)))
        ax.set_xticklabels(channels)
        ax.set_xlabel("DMA channels", color=MUTED)
        ax.set_ylim(bottom=0)
        ax.tick_params(colors=MUTED, length=0)
        ax.grid(axis="y", color=GRID, linewidth=0.8)
        ax.set_axisbelow(True)
        for side in ("top", "right", "left"):
            ax.spines[side].set_visible(False)
        ax.spines["bottom"].set_color(GRID)

    handles, names = axes[0].get_legend_handles_labels()
    fig.legend(handles, names, loc="upper right", ncol=2, frameon=False,
               labelcolor=TEXT, fontsize=11, bbox_to_anchor=(0.995, 0.985))

    fig.text(0.012, 0.945, f"RSS load distribution: {hash_name} vs JSQ",
             color=TEXT, fontsize=15, fontweight="bold")
    fig.text(0.012, 0.895, plot_subtitle(config), color=MUTED, fontsize=10.5)
    fig.tight_layout(rect=(0, 0, 1, 0.87))
    fig.savefig(output_path, dpi=150, facecolor=SURFACE)
    plt.close(fig)


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("usage: analyze.py <config.conf>\n")
        return 1

    config = read_config(sys.argv[1])
    csv_path = Path(config["out"])
    data = pd.read_csv(csv_path)

    if data.empty:
        print(f"{csv_path}: no full window, nothing to plot (is `window` bigger than the dataset?)")
        return 0

    summary = mean_over_windows(data)
    print_table(summary)

    output_path = csv_path.with_suffix(".png")
    plot(summary, config, output_path)
    print(f"plot saved to {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
