#pragma once

#include <istream>
#include <map>
#include <optional>
#include <string>

namespace benchlab {

/// Medians from a previous run, keyed by "group/name".
struct Baseline {
    std::string compiler;
    std::map<std::string, double> median_ns;
};

/// Reads a file written by `--format json`. Returns nullopt and sets `error` when nothing usable is found.
std::optional<Baseline> parse_baseline(std::istream& in, std::string& error);

/// Human-readable change from baseline to current, e.g. "1.85x faster", "1.20x slower", "same".
std::string describe_change(double baseline_ns, double current_ns);

}  // namespace benchlab
