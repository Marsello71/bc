/**
 * @file results_writer.cpp
 * @brief The CSV schema, in one place. See results_writer.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "results_writer.hpp"

ResultsWriter::ResultsWriter(std::ostream& os) : os_(os) {
    // TODO: write the header line, matching write() below field-for-field:
    // "algorithm,symmetry,tuple_run_index,key_id,num_channels,"
    // "thresshold_sum,fairness,chi,min_max_diff,max_diff\n"
}

void ResultsWriter::write(const ResultRow& row) {
    // TODO, in the same order as the header:
    //   row.algorithm
    //   symmetryName(row.symmetry)
    //   row.window_index
    //   row.key_id ? std::to_string(*row.key_id) : "N/A"
    //   row.num_channels
    //   row.over_threshold, row.fairness, row.chi, row.min_max_diff, row.max_diff
    //
    // Keep the default ostream formatting for the doubles (6 significant digits)
    // so the golden diff on `thresshold_sum` stays clean — do NOT add setprecision.
    (void)row;
}
