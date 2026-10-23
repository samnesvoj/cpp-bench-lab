// Memory allocation benchmarks: general-purpose heap vs std::pmr resources, and small-string optimisation.
#include <array>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>

#include "benchlab/harness.hpp"

namespace {

using benchlab::Context;
using benchlab::do_not_optimize;

constexpr std::size_t kN = 10'000;

struct Node {
    long value;
    Node* next;
};

// n individual heap allocations, then n frees — the pattern of node-based containers.
BENCHLAB_REGISTER("allocation", "new/delete 16-byte objects", kN, [](Context& ctx) {
    return [n = ctx.n] {
        std::vector<Node*> nodes(n);
        for (std::size_t i = 0; i < n; ++i) nodes[i] = new Node{static_cast<long>(i), nullptr};
        do_not_optimize(nodes.data());
        for (Node* p : nodes) delete p;
    };
});

// Same objects from a monotonic buffer: allocation is a pointer bump, release is a no-op.
BENCHLAB_REGISTER("allocation", "pmr monotonic_buffer 16-byte objects", kN, [](Context& ctx) {
    return [n = ctx.n] {
        std::pmr::monotonic_buffer_resource arena(n * sizeof(Node));
        std::pmr::polymorphic_allocator<Node> alloc(&arena);
        std::vector<Node*> nodes(n);
        for (std::size_t i = 0; i < n; ++i) {
            nodes[i] = alloc.allocate(1);
            nodes[i]->value = static_cast<long>(i);
        }
        do_not_optimize(nodes.data());
    };
});

// A pool keeps freed blocks for reuse: fast when objects are allocated and freed repeatedly.
BENCHLAB_REGISTER("allocation", "pmr unsynchronized_pool 16-byte objects", kN, [](Context& ctx) {
    auto pool = std::make_shared<std::pmr::unsynchronized_pool_resource>();
    return [pool, n = ctx.n] {
        std::pmr::polymorphic_allocator<Node> alloc(pool.get());
        std::vector<Node*> nodes(n);
        for (std::size_t i = 0; i < n; ++i) nodes[i] = alloc.allocate(1);
        do_not_optimize(nodes.data());
        for (Node* p : nodes) alloc.deallocate(p, 1);
    };
});

// A growing vector reallocates; with a stack buffer behind a monotonic resource it never touches the heap.
BENCHLAB_REGISTER("allocation", "std::vector<int> growth (heap)", kN, [](Context& ctx) {
    return [n = ctx.n] {
        std::vector<int> v;
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        do_not_optimize(v);
    };
});
BENCHLAB_REGISTER("allocation", "pmr::vector<int> growth (stack buffer)", kN, [](Context& ctx) {
    return [n = ctx.n] {
        // A monotonic resource never reuses freed memory, so all the geometric growth steps together
        // need up to ~4× n ints; the null upstream turns any overflow into an exception, not a heap call.
        alignas(std::max_align_t) static std::array<std::byte, kN * sizeof(int) * 4> buffer;
        std::pmr::monotonic_buffer_resource arena(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
        std::pmr::vector<int> v(&arena);
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        do_not_optimize(v);
    };
});

// Small-string optimisation: short strings live inside the std::string object, long ones need the heap.
std::vector<std::string> make_texts(Context& ctx, std::size_t length) {
    std::vector<std::string> texts(ctx.n);
    std::uniform_int_distribution<int> letter('a', 'z');
    for (auto& t : texts) {
        t.resize(length);
        for (auto& c : t) c = static_cast<char>(letter(ctx.rng));
    }
    return texts;
}
BENCHLAB_REGISTER("allocation", "copy 8-char strings (SSO)", kN, [](Context& ctx) {
    auto src = std::make_shared<std::vector<std::string>>(make_texts(ctx, 8));
    return [src] {
        std::vector<std::string> copy(*src);
        do_not_optimize(copy);
    };
});
BENCHLAB_REGISTER("allocation", "copy 64-char strings (heap)", kN, [](Context& ctx) {
    auto src = std::make_shared<std::vector<std::string>>(make_texts(ctx, 64));
    return [src] {
        std::vector<std::string> copy(*src);
        do_not_optimize(copy);
    };
});

}  // namespace
