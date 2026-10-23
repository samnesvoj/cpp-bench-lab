#include "benchlab/baseline.hpp"

#include <cmath>
#include <cstdio>
#include <regex>
#include <sstream>

namespace benchlab {

namespace {

// A JSON string body with escapes, as written by write_json.
const std::string kString = R"re("((?:[^"\\]|\\.)*)")re";

std::string unescape(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;
        out += s[i];
    }
    return out;
}

}  // namespace

std::optional<Baseline> parse_baseline(std::istream& in, std::string& error) {
    const std::regex compiler_re("\"compiler\"\\s*:\\s*" + kString);
    const std::regex result_re("\"group\"\\s*:\\s*" + kString + "\\s*,\\s*\"name\"\\s*:\\s*" + kString +
                               R"([^}]*"median"\s*:\s*([-+0-9.eE]+))");
    Baseline b;
    std::string line;
    std::smatch m;
    while (std::getline(in, line)) {
        if (b.compiler.empty() && std::regex_search(line, m, compiler_re)) b.compiler = unescape(m[1]);
        if (std::regex_search(line, m, result_re)) {
            b.median_ns[unescape(m[1]) + "/" + unescape(m[2])] = std::stod(m[3]);
        }
    }
    if (b.median_ns.empty()) {
        error = "no results found (expected a file written by --format json)";
        return std::nullopt;
    }
    return b;
}

std::string describe_change(double baseline_ns, double current_ns) {
    if (baseline_ns <= 0 || current_ns <= 0) return "n/a";
    const double ratio = baseline_ns / current_ns;
    if (std::abs(ratio - 1.0) < 0.03) return "same";  // within 3%: treat as noise
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2fx %s", ratio > 1 ? ratio : 1 / ratio, ratio > 1 ? "faster" : "slower");
    return buf;
}

}  // namespace benchlab
