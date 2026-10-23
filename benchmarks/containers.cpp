// Container benchmarks: insertion, traversal and lookup.
#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "benchlab/harness.hpp"

namespace {

using benchlab::Context;
using benchlab::do_not_optimize;

constexpr std::size_t kN = 10'000;

template <class Container>
benchlab::Body push_back_body(std::size_t n, bool reserve) {
    return [n, reserve] {
        Container c;
        if constexpr (requires { c.reserve(n); }) {
            if (reserve) c.reserve(n);
        } else {
            (void)reserve;  // list/deque have no reserve()
        }
        for (std::size_t i = 0; i < n; ++i) c.push_back(static_cast<int>(i));
        do_not_optimize(c);
    };
}

BENCHLAB_REGISTER("containers", "vector push_back", kN, [](Context& ctx) {
    return push_back_body<std::vector<int>>(ctx.n, false);
});
BENCHLAB_REGISTER("containers", "vector push_back + reserve", kN, [](Context& ctx) {
    return push_back_body<std::vector<int>>(ctx.n, true);
});
BENCHLAB_REGISTER("containers", "deque push_back", kN, [](Context& ctx) {
    return push_back_body<std::deque<int>>(ctx.n, false);
});
BENCHLAB_REGISTER("containers", "list push_back", kN, [](Context& ctx) {
    return push_back_body<std::list<int>>(ctx.n, false);
});

// Traversal: contiguous memory vs pointer chasing through nodes allocated in shuffled order.
BENCHLAB_REGISTER("containers", "vector sum", kN, [](Context& ctx) {
    auto v = std::make_shared<std::vector<long>>(ctx.n);
    std::iota(v->begin(), v->end(), 0L);
    return [v] { do_not_optimize(std::accumulate(v->begin(), v->end(), 0L)); };
});
BENCHLAB_REGISTER("containers", "list sum (shuffled nodes)", kN, [](Context& ctx) {
    auto l = std::make_shared<std::list<long>>();
    std::vector<long> values(ctx.n);
    std::iota(values.begin(), values.end(), 0L);
    std::shuffle(values.begin(), values.end(), ctx.rng);
    for (long x : values) l->push_back(x);
    // splice into sorted order so iteration jumps around memory
    l->sort();
    return [l] { do_not_optimize(std::accumulate(l->begin(), l->end(), 0L)); };
});

// Lookup: n random queries against n keys.
template <class Lookup>
benchlab::Body lookup_body(Context& ctx, Lookup make) {
    std::vector<int> keys(ctx.n);
    std::iota(keys.begin(), keys.end(), 0);
    std::vector<int> queries(ctx.n);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(ctx.n) - 1);
    for (auto& q : queries) q = dist(ctx.rng);
    return make(std::move(keys), std::move(queries));
}

BENCHLAB_REGISTER("containers", "map find", kN, [](Context& ctx) {
    return lookup_body(ctx, [](std::vector<int> keys, std::vector<int> queries) -> benchlab::Body {
        auto m = std::make_shared<std::map<int, int>>();
        for (int k : keys) m->emplace(k, k);
        return [m, queries = std::move(queries)] {
            long hits = 0;
            for (int q : queries) hits += m->find(q)->second;
            do_not_optimize(hits);
        };
    });
});
BENCHLAB_REGISTER("containers", "unordered_map find", kN, [](Context& ctx) {
    return lookup_body(ctx, [](std::vector<int> keys, std::vector<int> queries) -> benchlab::Body {
        auto m = std::make_shared<std::unordered_map<int, int>>();
        m->reserve(keys.size());
        for (int k : keys) m->emplace(k, k);
        return [m, queries = std::move(queries)] {
            long hits = 0;
            for (int q : queries) hits += m->find(q)->second;
            do_not_optimize(hits);
        };
    });
});
BENCHLAB_REGISTER("containers", "sorted vector lower_bound", kN, [](Context& ctx) {
    return lookup_body(ctx, [](std::vector<int> keys, std::vector<int> queries) -> benchlab::Body {
        auto v = std::make_shared<std::vector<int>>(std::move(keys));
        return [v, queries = std::move(queries)] {
            long hits = 0;
            for (int q : queries) hits += *std::lower_bound(v->begin(), v->end(), q);
            do_not_optimize(hits);
        };
    });
});

}  // namespace
