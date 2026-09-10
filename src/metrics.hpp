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

#include <cstdint>
#include <vector>

// Histogram entries are int64_t: with "byte" weighting a channel's running
// total in one 150k-packet window can exceed 2^31.

/**
 * Jain's fairness index:  (Σx)² / (n · Σx²).
 * Range (0, 1].  1.0 = every channel got exactly the same count.
 * Higher is better.  Ignores @p window_packets and @p channels (uses hist.size()).
 * Source: benchmark.cpp computeFairness (fix: take hist by const ref, not by value).
 */
double computeFairness(const std::vector<int64_t>& hist, long window_packets, int channels);

/**
 * Normalised Pearson χ²:  ( Σ (xᵢ − e)² / e ) / window_packets ,  e = window_packets / n.
 * 0.0 = perfectly uniform.  Higher = more skew.  Lower is better.
 * This is the metric that separates hash functions on low-entropy input.
 * Source: benchmark.cpp computeChi.
 */
double computeChi(const std::vector<int64_t>& hist, long window_packets, int channels);

/*
 * Total packets above the fair share, summed over overloaded channels,
 * expressed in per-mille of the window:
 *   ( Σ max(0, xᵢ − avg) ) / (window_packets / 1000) ,  avg = window_packets / channels.
 * 0.0 = nothing over fair share.  Lower is better.
 * This is the CSV column `thresshold_sum` — see results/RESULTS_SCHEMA.md.
 * Source: benchmark.cpp computeOverThressholdSum. NOTE: the original divides by the
 * global WINDOW_SIZE; here divide by @p window_packets (same value, no global).
 */
double computeOverThreshold(const std::vector<int64_t>& hist, long window_packets, int channels);

#endif // METRICS_HPP
