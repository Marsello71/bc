/**
 * @file metrics.cpp
 * @brief Implementation of the load-distribution metrics. See metrics.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "metrics.hpp"

#include <algorithm>

double computeFairness(const std::vector<int>& hist, long window_packets, int channels) {
    // TODO: benchmark.cpp:47-57. sum, sum_sq over hist; return (sum*sum)/(sum_sq*hist.size()).
    // Guard: empty hist -> 0.0.
    (void)window_packets; (void)channels;
    return 0.0;
}

double computeChi(const std::vector<int>& hist, long window_packets, int channels) {
    // TODO: benchmark.cpp:60-75. e = window_packets / hist.size();
    // chi2 += (x-e)*(x-e)/e; return chi2 / window_packets.
    (void)channels;
    (void)hist; (void)window_packets;
    return 0.0;
}

double computeMinMaxDiff(const std::vector<int>& hist, long window_packets, int channels) {
    // TODO: benchmark.cpp:78-90. (max-min) / window_packets * 100.
    (void)channels;
    (void)hist; (void)window_packets;
    return 0.0;
}

double computeMaxDiffRun(const std::vector<int>& hist, long window_packets, int channels) {
    // TODO: benchmark.cpp:92-102. avg = window_packets / channels; return (max-avg)/avg.
    (void)hist; (void)window_packets; (void)channels;
    return 0.0;
}

double computeOverThreshold(const std::vector<int>& hist, long window_packets, int channels) {
    // TODO: benchmark.cpp:104-116. avg = window_packets / channels;
    // sum += max(0, x-avg) over channels; return sum / (window_packets / 1000.0).
    (void)hist; (void)window_packets; (void)channels;
    return 0.0;
}
