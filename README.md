# cpp-bench-lab

Small, reproducible C++20 micro-benchmarks for containers, memory allocation and standard algorithms —
with a dependency-free harness you can read in five minutes.

```console
$ ./build/cpp-bench-lab
cpp-bench-lab · clang 21.0 · C++20 · release · seed 42 · 15 samples

benchmark                                           median     ±mad         min  ns/op
---------------------------------------------------------------------------------------
containers/vector push_back                           0.65     0.23%        0.65
containers/vector push_back + reserve                 0.67     0.19%        0.67
containers/deque push_back                            0.56     0.36%        0.56
containers/list push_back                            10.84     0.97%       10.49
containers/vector sum                                 0.05     0.17%        0.05
containers/list sum (shuffled nodes)                  2.33     0.42%        2.29
containers/map find                                  38.80     2.19%       36.21
containers/unordered_map find                         0.87     0.27%        0.86
containers/sorted vector lower_bound                 17.33     0.24%       17.19
```

<sub>Apple M5 Pro · macOS · Apple clang, `-O3` Release build. Your numbers will differ — that is the point of
running it yourself.</sub>

## Build and run

Requires CMake 3.20+ and a C++20 compiler (GCC 11+, Clang 14+, Apple clang 15+, MSVC 19.30+).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/cpp-bench-lab                     # everything
./build/cpp-bench-lab --filter lookup     # only matching "group/name"
./build/cpp-bench-lab --list              # what is available
ctest --test-dir build                    # unit tests + smoke runs
```

| Option | Description |
|---|---|
| `--filter TEXT` | run benchmarks whose `group/name` contains `TEXT` |
| `--list` | list benchmarks with their problem size |
| `--quick` | 5 short samples per benchmark (CI smoke runs) |
| `--samples N` | timed samples per benchmark (default 15) |
| `--seed N` | seed for generated inputs (default 42) |
| `--format F` | `table` (default), `csv` or `json` |
| `--output FILE` | write CSV/JSON to a file; the table is still printed |
| `--compare FILE` | add a *vs baseline* column against a previous `--format json` run |

### Comparing runs

Export results to compare compilers, flags or machines:

```bash
./build/cpp-bench-lab --format json --output gcc.json      # table on screen, JSON in the file
./build/cpp-bench-lab --format csv > results.csv           # CSV on stdout, table on stderr
```

The JSON file records the compiler, C++ standard, build type, seed and sample count next to every statistic
(median, MAD, mean, standard deviation, min, max), so a result never loses its context.

Then measure a change against that baseline:

```console
$ ./build/cpp-bench-lab --format json --output before.json
$ # ...change compiler flags, the standard library or the code...
$ ./build/cpp-bench-lab --compare before.json
benchmark                          median     ±mad         min  ns/op   vs baseline
-----------------------------------------------------------------------------------
algorithms/std::sort random ints    12.15     0.17%       12.12        same
...
```

Changes within 3% are reported as `same`; benchmarks missing from the baseline show `new`, and a note is printed
when the baseline was built with a different compiler.

## How results are measured

- **Untimed setup.** Each benchmark has a *prepare* step (build the container, generate inputs) and a timed body.
- **Calibration.** Repetitions are doubled until one sample lasts at least 20 ms, so timer resolution never dominates.
- **Warm-up** runs are discarded.
- **Robust statistics.** Every sample is converted to *ns per operation*; the table shows the **median** and the
  **median absolute deviation** as a percentage, which outliers (a context switch, a page fault) barely move.
- **Reproducible inputs.** Inputs come from `std::mt19937_64` reseeded with `seed + sample` before every sample,
  so every run of the program — and every benchmark, regardless of order — sees identical data.
- **No dead-code elimination.** Results pass through `do_not_optimize`, and memory is clobbered between repetitions.
- **Release builds only.** Debug builds print a warning in the header line.

## Benchmarks

| Group | What it compares |
|---|---|
| `containers` | `push_back` into `vector` (with/without `reserve`), `deque`, `list`; contiguous vs pointer-chasing traversal; lookups in `map`, `unordered_map` and a sorted `vector` |
| `allocation` | `new`/`delete` of small objects vs `std::pmr` monotonic and pool resources; vector growth on the heap vs a stack buffer; copying short (SSO) vs long strings |
| `algorithms` | `std::sort` vs `std::stable_sort` vs `std::ranges::sort` on random and already-sorted data; median by full sort vs `std::nth_element`; linear `find` vs `binary_search` on 16 and 4096 elements |

## Things worth noticing

Results depend on the standard library as much as on the hardware — which is exactly what the suite is for:

- On libc++ (LLVM 17+, Apple clang), `std::stable_sort` of integers can beat `std::sort` because libc++ switches to a
  radix sort for arithmetic types. With libstdc++ (GCC) the usual ordering returns.
- `std::nth_element` finds a median roughly 3× faster than sorting everything.
- For 16 elements a linear scan is in the same league as binary search; for 4096 it is ~50× slower.
- Iterating a `std::list` whose nodes are scattered in memory is an order of magnitude slower than summing a
  `std::vector`, even though both are O(n).

## Adding a benchmark

Drop a `.cpp` file into `benchmarks/` — it is picked up automatically:

```cpp
#include <string>

#include "benchlab/harness.hpp"

BENCHLAB_REGISTER("strings", "append char", 10'000, [](benchlab::Context& ctx) {
    return [n = ctx.n] {               // everything above this line is untimed setup
        std::string s;
        for (std::size_t i = 0; i < n; ++i) s += 'x';
        benchlab::do_not_optimize(s);
    };
});
```

`n` is the number of logical operations per call; results are reported per operation.

## License

[MIT](LICENSE)
