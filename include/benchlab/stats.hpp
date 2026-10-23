#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace benchlab {

/// Summary of repeated timing samples (all values in nanoseconds per operation).
struct Stats {
    double min = 0;
    double median = 0;
    double mean = 0;
    double stddev = 0;
    double mad = 0;  ///< median absolute deviation — robust to outliers
    double max = 0;
    std::size_t samples = 0;

    /// Relative spread (MAD / median); small values mean a stable measurement.
    [[nodiscard]] double relative_mad() const { return median > 0 ? mad / median : 0; }
};

[[nodiscard]] inline double median_of(std::vector<double> v) {
    if (v.empty()) throw std::invalid_argument("median of an empty sample");
    const auto mid = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(mid), v.end());
    double m = v[mid];
    if (v.size() % 2 == 0) {
        m = (m + *std::max_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(mid))) / 2.0;
    }
    return m;
}

[[nodiscard]] inline Stats compute_stats(const std::vector<double>& samples) {
    if (samples.empty()) throw std::invalid_argument("compute_stats needs at least one sample");
    Stats s;
    s.samples = samples.size();
    const auto [lo, hi] = std::minmax_element(samples.begin(), samples.end());
    s.min = *lo;
    s.max = *hi;
    s.mean = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
    double sq = 0;
    for (double x : samples) sq += (x - s.mean) * (x - s.mean);
    s.stddev = samples.size() > 1 ? std::sqrt(sq / static_cast<double>(samples.size() - 1)) : 0.0;
    s.median = median_of(samples);
    std::vector<double> dev;
    dev.reserve(samples.size());
    for (double x : samples) dev.push_back(std::abs(x - s.median));
    s.mad = median_of(std::move(dev));
    return s;
}

}  // namespace benchlab
