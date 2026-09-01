/**
 * @file results_writer.hpp
 * @brief The one place that knows the results CSV schema.
 *
 * Keep field order == column order == results/README.md.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef RESULTS_WRITER_HPP
#define RESULTS_WRITER_HPP

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

#include "transform.hpp"

/// One CSV row = one window of one run at one channel count.
struct ResultRow {
    std::string          algorithm;
    Symmetry             symmetry;        
    long                 window_index;
    int                  key_id;
    int                  num_channels;

    // metrics for this window
    double over_threshold;
    double fairness;
    double chi;
};

/// Writes the header on construction, then one line per write().
class ResultsWriter {
public:
    explicit ResultsWriter(std::ostream& os);
    void write(const ResultRow& row);

private:
    std::ostream& os_;
};

#endif // RESULTS_WRITER_HPP
