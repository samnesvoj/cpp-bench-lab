#pragma once

#include <ostream>

#include "benchlab/harness.hpp"

namespace benchlab {

/// Compiler, standard library, build type and run configuration — needed to compare results.
void print_environment(std::ostream& os, const Config& cfg);

/// One table row; prints the header first when `header` is true.
void print_row(std::ostream& os, const Result& r, bool header);

}  // namespace benchlab
