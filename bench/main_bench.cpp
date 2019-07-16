#include <benchmark/benchmark.h>
#include <real/real.hpp>

std::optional<size_t> boost::real::const_precision_iterator::maximum_precision = 10;

BENCHMARK_MAIN();
