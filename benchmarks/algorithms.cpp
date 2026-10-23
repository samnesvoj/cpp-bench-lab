// Standard algorithm benchmarks: sorting, selection and searching on generated data.
#include <algorithm>
#include <numeric>
#include <vector>

#include "benchlab/harness.hpp"

namespace {

using benchlab::Context;
using benchlab::do_not_optimize;

std::vector<int> random_ints(Context& ctx) {
    std::vector<int> v(ctx.n);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (auto& x : v) x = dist(ctx.rng);
    return v;
}

// Sorting copies a fresh shuffled vector each repetition, so every run sorts unsorted data.
template <class Sort>
benchlab::Body sort_body(Context& ctx, Sort sort) {
    auto input = std::make_shared<const std::vector<int>>(random_ints(ctx));
    return [input, sort] {
        std::vector<int> v(*input);
        sort(v);
        do_not_optimize(v.data());
    };
}

constexpr std::size_t kSortN = 100'000;

BENCHLAB_REGISTER("algorithms", "std::sort random ints", kSortN, [](Context& ctx) {
    return sort_body(ctx, [](std::vector<int>& v) { std::sort(v.begin(), v.end()); });
});
BENCHLAB_REGISTER("algorithms", "std::stable_sort random ints", kSortN, [](Context& ctx) {
    return sort_body(ctx, [](std::vector<int>& v) { std::stable_sort(v.begin(), v.end()); });
});
BENCHLAB_REGISTER("algorithms", "std::ranges::sort random ints", kSortN, [](Context& ctx) {
    return sort_body(ctx, [](std::vector<int>& v) { std::ranges::sort(v); });
});
BENCHLAB_REGISTER("algorithms", "std::sort already sorted", kSortN, [](Context& ctx) {
    auto input = std::make_shared<std::vector<int>>(random_ints(ctx));
    std::sort(input->begin(), input->end());
    return [input] {
        std::vector<int> v(*input);
        std::sort(v.begin(), v.end());
        do_not_optimize(v.data());
    };
});

// Median: full sort vs selection.
BENCHLAB_REGISTER("algorithms", "median via std::sort", kSortN, [](Context& ctx) {
    return sort_body(ctx, [](std::vector<int>& v) {
        std::sort(v.begin(), v.end());
        do_not_optimize(v[v.size() / 2]);
    });
});
BENCHLAB_REGISTER("algorithms", "median via std::nth_element", kSortN, [](Context& ctx) {
    return sort_body(ctx, [](std::vector<int>& v) {
        std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2), v.end());
        do_not_optimize(v[v.size() / 2]);
    });
});

// Searching: for tiny arrays a linear scan competes with binary search; for large ones it cannot.
template <bool Binary>
benchlab::Body search_body(Context& ctx, std::size_t size) {
    auto data = std::make_shared<std::vector<int>>(size);
    std::iota(data->begin(), data->end(), 0);
    auto queries = std::make_shared<std::vector<int>>(ctx.n);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(size) - 1);
    for (auto& q : *queries) q = dist(ctx.rng);
    return [data, queries] {
        long found = 0;
        for (int q : *queries) {
            if constexpr (Binary) {
                found += std::binary_search(data->begin(), data->end(), q);
            } else {
                found += std::find(data->begin(), data->end(), q) != data->end();
            }
        }
        do_not_optimize(found);
    };
}

constexpr std::size_t kQueries = 10'000;

BENCHLAB_REGISTER("algorithms", "linear find in 16 ints", kQueries, [](Context& ctx) {
    return search_body<false>(ctx, 16);
});
BENCHLAB_REGISTER("algorithms", "binary_search in 16 ints", kQueries, [](Context& ctx) {
    return search_body<true>(ctx, 16);
});
BENCHLAB_REGISTER("algorithms", "linear find in 4096 ints", kQueries, [](Context& ctx) {
    return search_body<false>(ctx, 4096);
});
BENCHLAB_REGISTER("algorithms", "binary_search in 4096 ints", kQueries, [](Context& ctx) {
    return search_body<true>(ctx, 4096);
});

}  // namespace
