#include "benchlab/report.hpp"

#include <algorithm>
#include <iomanip>
#include <string>

namespace benchlab {

std::string compiler_name() {
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

namespace {

constexpr bool kReleaseBuild =
#ifdef NDEBUG
    true;
#else
    false;
#endif

std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

std::string csv_field(const std::string& s) {
    if (s.find_first_of(",\"\n") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) out += (c == '"') ? std::string("\"\"") : std::string(1, c);
    return out + "\"";
}

}  // namespace

void print_environment(std::ostream& os, const Config& cfg) {
    os << "cpp-bench-lab · " << compiler_name() << " · C++" << (__cplusplus / 100 % 100)
#ifdef NDEBUG
       << " · release"
#else
       << " · DEBUG build (numbers are not meaningful)"
#endif
       << " · seed " << cfg.seed << " · " << cfg.samples << " samples\n\n";
}

void print_row(std::ostream& os, const Result& r, bool header, std::size_t name_width, const Baseline* baseline) {
    const auto w = static_cast<int>(std::max<std::size_t>(name_width, 9) + 2);
    const auto key = r.bench->group + "/" + r.bench->name;
    if (header) {
        os << std::left << std::setw(w) << "benchmark" << std::right << std::setw(12) << "median" << std::setw(10)
           << "±mad" << std::setw(12) << "min" << "  ns/op";
        if (baseline) os << "   vs baseline";
        os << "\n" << std::string(static_cast<std::size_t>(w) + 12 + 10 + 12 + 7 + (baseline ? 15 : 0), '-') << "\n";
    }
    const auto& s = r.ns_per_op;
    os << std::left << std::setw(w) << key << std::right << std::fixed << std::setprecision(2) << std::setw(12)
       << s.median << std::setw(9) << (s.relative_mad() * 100) << "%" << std::setw(12) << s.min;
    if (baseline) {
        const auto it = baseline->median_ns.find(key);
        os << "        " << (it == baseline->median_ns.end() ? "new" : describe_change(it->second, s.median));
    }
    os << "\n";
}

void write_csv(std::ostream& os, const std::vector<Result>& results) {
    os << "group,name,n,repetitions,samples,median_ns,mad_ns,mean_ns,stddev_ns,min_ns,max_ns\n";
    os << std::setprecision(6);
    for (const auto& r : results) {
        const auto& s = r.ns_per_op;
        os << csv_field(r.bench->group) << ',' << csv_field(r.bench->name) << ',' << r.bench->n << ','
           << r.repetitions << ',' << s.samples << ',' << s.median << ',' << s.mad << ',' << s.mean << ','
           << s.stddev << ',' << s.min << ',' << s.max << "\n";
    }
}

void write_json(std::ostream& os, const std::vector<Result>& results, const Config& cfg) {
    os << std::setprecision(6);
    os << "{\n  \"environment\": {\"compiler\": \"" << json_escape(compiler_name()) << "\", \"cplusplus\": " << __cplusplus
       << ", \"release\": " << (kReleaseBuild ? "true" : "false") << ", \"seed\": " << cfg.seed
       << ", \"samples\": " << cfg.samples << "},\n  \"unit\": \"ns/op\",\n  \"results\": [";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        const auto& s = r.ns_per_op;
        os << (i ? "," : "") << "\n    {\"group\": \"" << json_escape(r.bench->group) << "\", \"name\": \""
           << json_escape(r.bench->name) << "\", \"n\": " << r.bench->n << ", \"repetitions\": " << r.repetitions
           << ", \"median\": " << s.median << ", \"mad\": " << s.mad << ", \"mean\": " << s.mean
           << ", \"stddev\": " << s.stddev << ", \"min\": " << s.min << ", \"max\": " << s.max << "}";
    }
    os << "\n  ]\n}\n";
}

}  // namespace benchlab
