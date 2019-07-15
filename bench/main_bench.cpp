#include <benchmark/benchmark.h>
#include <real/real.hpp>

std::optional<size_t> boost::real::real::maximum_precision = 10;
std::optional<size_t> boost::real::real_algorithm::maximum_precision = 10;

BENCHMARK_MAIN();
