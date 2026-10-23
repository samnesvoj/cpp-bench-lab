#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "benchlab/harness.hpp"

namespace benchlab {

/// Compiler, standard library, build type and run configuration — needed to compare results.
void print_environment(std::ostream& os, const Config& cfg);

/// One table row; prints the header first when `header` is true. `name_width` is the width
/// of the benchmark column (use the longest "group/name" being run).
void print_row(std::ostream& os, const Result& r, bool header, std::size_t name_width);

/// Short description of the toolchain, e.g. "clang 21.0".
std::string compiler_name();

/// Machine-readable output: one row per benchmark with every statistic.
void write_csv(std::ostream& os, const std::vector<Result>& results);

/// JSON document with the environment (compiler, standard, build type, seed) and all results.
void write_json(std::ostream& os, const std::vector<Result>& results, const Config& cfg);

}  // namespace benchlab
