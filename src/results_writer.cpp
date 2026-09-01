/**
 * @file results_writer.cpp
 * @brief The CSV schema, in one place. See results_writer.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "results_writer.hpp"

// Column order == ResultRow field order. Header and write() must stay in lockstep.
ResultsWriter::ResultsWriter(std::ostream& os) : outstream(os) {
    outstream << "algorithm,symmetry,tuple_run_index,key_id,num_channels,"
                 "thresshold_sum,fairness,chi\n";
}

void ResultsWriter::write(const ResultRow& row) {
    outstream << row.algorithm            << ','
              << symmetryName(row.symmetry) << ','
              << row.window_index          << ','
              << row.key_id                << ','
              << row.num_channels          << ','
              << row.over_threshold        << ','
              << row.fairness              << ','
              << row.chi                   << '\n';
}
