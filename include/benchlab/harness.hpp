#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "benchlab/stats.hpp"

namespace benchlab {

/// Prevent the optimiser from discarding a value or the work that produced it.
template <class T>
inline void do_not_optimize(T const& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(value) : "memory");
#else
    static volatile const void* sink;
    sink = &value;
#endif
}

inline void clobber_memory() {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : : "memory");
#endif
}

/// Everything a benchmark body needs: the problem size and a deterministic RNG.
struct Context {
    std::size_t n;
    std::mt19937_64& rng;
};

/// A benchmark is prepared once per sample (untimed) and returns the callable that is timed.
/// The timed callable performs `n` logical operations, so results are reported per operation.
using Body = std::function<void()>;
using Prepare = std::function<Body(Context&)>;

struct Benchmark {
    std::string group;
    std::string name;
    std::size_t n;
    Prepare prepare;
};

struct Config {
    std::uint64_t seed = 42;
    std::size_t samples = 15;
    std::size_t warmup = 2;
    std::chrono::nanoseconds min_sample_time = std::chrono::milliseconds(20);
};

struct Result {
    const Benchmark* bench;
    std::size_t repetitions;  ///< how many times the body ran per sample
    Stats ns_per_op;
};

/// Runs one benchmark: warm-up, calibration of repetitions so each sample lasts at least
/// `min_sample_time`, then `samples` timed samples. The RNG is reseeded per benchmark,
/// so results do not depend on which other benchmarks ran first.
Result run(const Benchmark& b, const Config& cfg);

/// Global registry filled by BENCHLAB_REGISTER at static-initialisation time.
std::vector<Benchmark>& registry();

struct Registrar {
    Registrar(std::string group, std::string name, std::size_t n, Prepare p) {
        registry().push_back({std::move(group), std::move(name), n, std::move(p)});
    }
};

}  // namespace benchlab

#define BENCHLAB_CONCAT_(a, b) a##b
#define BENCHLAB_CONCAT(a, b) BENCHLAB_CONCAT_(a, b)
/// BENCHLAB_REGISTER("group", "name", n, [](benchlab::Context& ctx) { ...; return [=] { ... }; });
#define BENCHLAB_REGISTER(group, name, n, ...) \
    static const ::benchlab::Registrar BENCHLAB_CONCAT(benchlab_reg_, __COUNTER__)(group, name, n, __VA_ARGS__)
