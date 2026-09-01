/**
 * @file metrics.hpp
 * @brief Load-distribution metrics computed per window over a channel histogram.
 *
 * Some metrics ignore an argument (kept for a uniform signature).
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef METRICS_HPP
#define METRICS_HPP

#include <vector>

/**
 * Jain's fairness index:  (Σx)² / (n · Σx²).
 * Range (0, 1].  1.0 = every channel got exactly the same count.
 * Higher is better.  Ignores @p window_packets and @p channels (uses hist.size()).
 * Source: benchmark.cpp computeFairness (fix: take hist by const ref, not by value).
 */
double computeFairness(const std::vector<int>& hist, long window_packets, int channels);

/**
 * Normalised Pearson χ²:  ( Σ (xᵢ − e)² / e ) / window_packets ,  e = window_packets / n.
 * 0.0 = perfectly uniform.  Higher = more skew.  Lower is better.
 * This is the metric that separates hash functions on low-entropy input.
 * Source: benchmark.cpp computeChi.
 */
double computeChi(const std::vector<int>& hist, long window_packets, int channels);

/**
 * Spread of the busiest vs. idlest channel, as a percentage of the window:
 *   (max − min) / window_packets · 100.
 * 0.0 = flat.  Lower is better.
 * Source: benchmark.cpp computeMinMaxDiff.
 */
double computeMinMaxDiff(const std::vector<int>& hist, long window_packets, int channels);

/**
 * Peak channel overload, as a fraction above the fair share:
 *   (max − avg) / avg ,  avg = window_packets / channels.
 * 0.0 = no channel is above fair share.  Lower is better.
 * Source: benchmark.cpp computeMaxDiffRun.
 */
double computeMaxDiffRun(const std::vector<int>& hist, long window_packets, int channels);

/**
 * Total packets above the fair share, summed over overloaded channels,
 * expressed in per-mille of the window:
 *   ( Σ max(0, xᵢ − avg) ) / (window_packets / 1000) ,  avg = window_packets / channels.
 * 0.0 = nothing over fair share.  Lower is better.
 * This is the CSV column `thresshold_sum` — see results/RESULTS_SCHEMA.md.
 * Source: benchmark.cpp computeOverThressholdSum. NOTE: the original divides by the
 * global WINDOW_SIZE; here divide by @p window_packets (same value, no global).
 */
double computeOverThreshold(const std::vector<int>& hist, long window_packets, int channels);

#endif // METRICS_HPP
