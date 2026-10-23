#include "benchlab/report.hpp"

#include <algorithm>
#include <iomanip>
#include <string>

namespace benchlab {

namespace {

std::string compiler() {
#if defined(__clang__)
    return "clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    return "gcc " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    return "msvc " + std::to_string(_MSC_VER);
#else
    return "unknown compiler";
#endif
}

}  // namespace

void print_environment(std::ostream& os, const Config& cfg) {
    os << "cpp-bench-lab · " << compiler() << " · C++" << (__cplusplus / 100 % 100)
#ifdef NDEBUG
       << " · release"
#else
       << " · DEBUG build (numbers are not meaningful)"
#endif
       << " · seed " << cfg.seed << " · " << cfg.samples << " samples\n\n";
}

void print_row(std::ostream& os, const Result& r, bool header, std::size_t name_width) {
    const auto w = static_cast<int>(std::max<std::size_t>(name_width, 9) + 2);
    if (header) {
        os << std::left << std::setw(w) << "benchmark" << std::right << std::setw(12) << "median" << std::setw(10)
           << "±mad" << std::setw(12) << "min" << "  ns/op\n";
        os << std::string(static_cast<std::size_t>(w) + 12 + 10 + 12 + 7, '-') << "\n";
    }
    const auto& s = r.ns_per_op;
    os << std::left << std::setw(w) << (r.bench->group + "/" + r.bench->name) << std::right << std::fixed
       << std::setprecision(2) << std::setw(12) << s.median << std::setw(9) << (s.relative_mad() * 100) << "%"
       << std::setw(12) << s.min << "\n";
}

}  // namespace benchlab
