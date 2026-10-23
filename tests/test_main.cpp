// Minimal self-contained test runner (no external dependencies).
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "benchlab/harness.hpp"
#include "benchlab/stats.hpp"

namespace {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};
std::vector<TestCase>& tests() {
    static std::vector<TestCase> t;
    return t;
}
struct Add {
    Add(std::string n, std::function<void()> f) { tests().push_back({std::move(n), std::move(f)}); }
};
int failures = 0;

#define TEST(name) \
    void name();   \
    const Add add_##name(#name, name); \
    void name()
#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::cerr << "  " << __FILE__ << ":" << __LINE__ << " CHECK(" #cond ")\n"; \
            ++failures;                                                             \
        }                                                                           \
    } while (0)
#define CHECK_NEAR(a, b) CHECK(std::abs((a) - (b)) < 1e-9)

TEST(median_odd_and_even) {
    CHECK_NEAR(benchlab::median_of({5, 1, 3}), 3.0);
    CHECK_NEAR(benchlab::median_of({4, 1, 3, 2}), 2.5);
    CHECK_NEAR(benchlab::median_of({7}), 7.0);
}

TEST(stats_of_known_sample) {
    const auto s = benchlab::compute_stats({2, 4, 4, 4, 5, 5, 7, 9});
    CHECK_NEAR(s.mean, 5.0);
    CHECK_NEAR(s.median, 4.5);
    CHECK_NEAR(s.min, 2.0);
    CHECK_NEAR(s.max, 9.0);
    CHECK_NEAR(s.stddev, std::sqrt(32.0 / 7.0));
    CHECK_NEAR(s.mad, 0.5);  // |x - 4.5| = 2.5 .5 .5 .5 .5 .5 2.5 4.5 -> median .5
    CHECK(s.samples == 8);
}

TEST(mad_ignores_outliers) {
    const auto s = benchlab::compute_stats({10, 10, 10, 11, 10, 1000});
    CHECK_NEAR(s.median, 10.0);
    CHECK_NEAR(s.mad, 0.0);
    CHECK(s.mean > 100);  // the mean is dragged up, the median is not
}

TEST(empty_sample_throws) {
    bool threw = false;
    try {
        (void)benchlab::compute_stats({});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(run_calibrates_and_reports_per_operation) {
    std::size_t calls = 0;
    benchlab::Benchmark b{"test", "busy loop", 100, [&](benchlab::Context& ctx) -> benchlab::Body {
        return [&calls, n = ctx.n] {
            ++calls;
            unsigned long x = 0;
            for (std::size_t i = 0; i < n; ++i) benchlab::do_not_optimize(x += i);
        };
    }};
    benchlab::Config cfg;
    cfg.samples = 3;
    cfg.warmup = 1;
    cfg.min_sample_time = std::chrono::microseconds(200);
    const auto r = benchlab::run(b, cfg);
    CHECK(r.repetitions >= 1);
    CHECK(r.ns_per_op.samples == 3);
    CHECK(r.ns_per_op.median > 0);
    CHECK(calls >= r.repetitions * 3);
}

TEST(inputs_are_reproducible_for_a_seed) {
    std::vector<std::uint64_t> seen;
    benchlab::Benchmark b{"test", "rng", 1, [&](benchlab::Context& ctx) -> benchlab::Body {
        seen.push_back(ctx.rng());
        return [] {};
    }};
    benchlab::Config cfg;
    cfg.samples = 4;
    cfg.warmup = 0;
    cfg.min_sample_time = std::chrono::nanoseconds(0);
    (void)benchlab::run(b, cfg);
    const auto first = seen;
    seen.clear();
    (void)benchlab::run(b, cfg);
    CHECK(first == seen);
    CHECK(first.size() >= 4);
}

}  // namespace

int main() {
    for (const auto& t : tests()) {
        const int before = failures;
        t.fn();
        std::cout << (failures == before ? "PASS " : "FAIL ") << t.name << "\n";
    }
    std::cout << tests().size() << " tests, " << failures << " failed checks\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
