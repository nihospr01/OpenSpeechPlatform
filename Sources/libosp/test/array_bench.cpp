#include <benchmark/benchmark.h>
#include <OSP/array_utilities/array_utilities.hpp>
#include <cassert>
#include <iostream>
#include <string>

using namespace std;


void array_sum_bench(benchmark::State &state) {
    int num = state.range(0);
    float *arr1 = new float[num];
    float total;
    for (auto i = 0; i < num; i++)
        arr1[i] = (float)i;

    for (auto _ : state) {
        benchmark::DoNotOptimize(total = array_sum(arr1, num));
        benchmark::ClobberMemory();
    }
    delete[] arr1;
}

void array_multiply_const_bench(benchmark::State &state) {
    unsigned num = (unsigned)state.range(0);
    float *arr1 = new float[num];
    for (unsigned i = 0; i < num; i++)
        arr1[i] = (float)i;

    for (auto _ : state) {
        array_multiply_const(arr1, 2.0, num);
        benchmark::ClobberMemory();
    }
    delete[] arr1;
}

BENCHMARK(array_sum_bench)->Arg(32)->Arg(48);
BENCHMARK(array_multiply_const_bench)->Arg(32)->Arg(48);
BENCHMARK_MAIN();
