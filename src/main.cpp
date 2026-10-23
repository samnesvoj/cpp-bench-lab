#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "benchlab/harness.hpp"
#include "benchlab/report.hpp"

namespace {

constexpr std::string_view kHelp = R"(Usage: cpp-bench-lab [options]

Runs reproducible C++20 micro-benchmarks and prints ns/op statistics.

Options:
  --filter TEXT   run only benchmarks whose "group/name" contains TEXT
  --list          list benchmarks and exit
  --quick         fewer, shorter samples (for CI smoke runs)
  --samples N     timed samples per benchmark (default 15)
  --seed N        RNG seed for generated inputs (default 42)
  -h, --help      show this help
)";

bool parse_size(const char* s, std::size_t& out) {
    char* end = nullptr;
    const auto v = std::strtoull(s, &end, 10);
    if (end == s || *end != '\0' || v == 0) return false;
    out = static_cast<std::size_t>(v);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    benchlab::Config cfg;
    std::string filter;
    bool list = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        const bool has_value = i + 1 < argc;
        if (arg == "-h" || arg == "--help") {
            std::cout << kHelp;
            return 0;
        } else if (arg == "--list") {
            list = true;
        } else if (arg == "--quick") {
            cfg.samples = 5;
            cfg.warmup = 1;
            cfg.min_sample_time = std::chrono::milliseconds(2);
        } else if (arg == "--filter" && has_value) {
            filter = argv[++i];
        } else if (arg == "--samples" && has_value && parse_size(argv[i + 1], cfg.samples)) {
            ++i;
        } else if (arg == "--seed" && has_value) {
            std::size_t seed = 0;
            if (!parse_size(argv[++i], seed)) {
                std::cerr << "--seed expects a positive integer\n";
                return 2;
            }
            cfg.seed = seed;
        } else {
            std::cerr << "unknown or incomplete option: " << arg << "\n\n" << kHelp;
            return 2;
        }
    }

    std::vector<const benchlab::Benchmark*> selected;
    for (const auto& b : benchlab::registry()) {
        if (filter.empty() || (b.group + "/" + b.name).find(filter) != std::string::npos) selected.push_back(&b);
    }
    if (list) {
        for (const auto* b : selected) std::cout << b->group << "/" << b->name << "  (n=" << b->n << ")\n";
        return 0;
    }
    if (selected.empty()) {
        std::cerr << "no benchmarks match --filter " << filter << "\n";
        return 1;
    }

    std::size_t name_width = 0;
    for (const auto* b : selected) name_width = std::max(name_width, b->group.size() + 1 + b->name.size());

    benchlab::print_environment(std::cout, cfg);
    std::vector<benchlab::Result> results;
    for (const auto* b : selected) {
        results.push_back(benchlab::run(*b, cfg));
        benchlab::print_row(std::cout, results.back(), results.size() == 1, name_width);
    }
    return 0;
}
