/**
 * @file metrics.cpp
 * @brief Implementation of the load-distribution metrics. See metrics.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "metrics.hpp"

#include <algorithm>
#include <cmath>

// Jain's fairness index: (Σx)² / (n · Σx²).
double computeFairness(const std::vector<int>& hist, long /*window_packets*/, int /*channels*/) {
    if (hist.empty()) {
        return 0.0;
    }
    double sum = 0.0, sum_sq = 0.0;
    for (double num : hist) {          // iterate as double -> no int*int overflow
        sum += num;
        sum_sq += num * num;
    }
    return (sum * sum) / (sum_sq * hist.size());
}

// Normalised Pearson chi-square: ( Σ (xᵢ − e)² / e ) / window_packets ,  e = window_packets / n.
double computeChi(const std::vector<int>& hist, long window_packets, int /*channels*/) {
    if (hist.empty() || window_packets == 0) {
        return 0.0;
    }
    double e = static_cast<double>(window_packets) / hist.size();
    double chi2 = 0.0;
    for (int num : hist) {
        double diff = num - e;
        chi2 += (diff * diff) / e;
    }
    return chi2 / window_packets;
}

// Packets above the fair share, summed over overloaded channels, in per-mille of the window.
double computeOverThreshold(const std::vector<int>& hist, long window_packets, int channels) {
    if (hist.empty() || window_packets == 0) {
        return 0.0;
    }
    double avg = static_cast<double>(window_packets) / channels;
    double sum = 0.0;
    for (int num : hist) {
        sum += std::max(0.0, num - avg);
    }
    return sum / (window_packets / 1000.0);
}

