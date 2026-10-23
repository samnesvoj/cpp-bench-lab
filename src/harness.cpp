#include "benchlab/harness.hpp"

#include <algorithm>

namespace benchlab {

std::vector<Benchmark>& registry() {
    static std::vector<Benchmark> benchmarks;
    return benchmarks;
}

namespace {

using Clock = std::chrono::steady_clock;

std::chrono::nanoseconds time_sample(const Benchmark& b, Context& ctx, std::size_t reps) {
    auto body = b.prepare(ctx);  // untimed setup
    const auto start = Clock::now();
    for (std::size_t r = 0; r < reps; ++r) {
        body();
        clobber_memory();
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start);
}

}  // namespace

Result run(const Benchmark& b, const Config& cfg) {
    std::mt19937_64 rng(cfg.seed);
    Context ctx{b.n, rng};

    for (std::size_t i = 0; i < cfg.warmup; ++i) time_sample(b, ctx, 1);

    // Calibrate: double the repetitions until one sample is long enough to time reliably.
    std::size_t reps = 1;
    while (reps < (std::size_t{1} << 30)) {
        if (time_sample(b, ctx, reps) >= cfg.min_sample_time) break;
        reps *= 2;
    }

    std::vector<double> per_op;
    per_op.reserve(cfg.samples);
    const double ops = static_cast<double>(reps) * static_cast<double>(std::max<std::size_t>(b.n, 1));
    for (std::size_t s = 0; s < cfg.samples; ++s) {
        rng.seed(cfg.seed + s);  // same inputs on every run of the program
        per_op.push_back(static_cast<double>(time_sample(b, ctx, reps).count()) / ops);
    }
    return {&b, reps, compute_stats(per_op)};
}

}  // namespace benchlab
