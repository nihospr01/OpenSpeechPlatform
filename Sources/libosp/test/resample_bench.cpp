// Benchmark different resamplers
#include <OSP/resample/32_48_filter.h>
#include <OSP/resample/48_32_filter.h>
#include "OSP/multirate_filterbank/elevenband_filtercoef.hpp"
#include "OSP/resample/polyphase_hb_downsampler.h"
#include "OSP/resample/polyphase_hb_upsampler.h"
#include <OSP/resample/resample.hpp>
#include <OSP/resample/resample_down.h>
#include <OSP/resample/resample_up.h>
#include <benchmark/benchmark.h>
#include <cassert>
#include <iostream>
#include <string>

using namespace std;

void ResampleDown_bench(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size >> 1];
    inp[0] = 1.0;
    ResampleDown *ds = new ResampleDown(hb1_lin, hb1_length, frame_size, 2);

    for (auto _ : state) {
        ds->down(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}

void ResampleUp_bench(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size << 1];
    inp[0] = 1.0;
    ResampleUp *ds = new ResampleUp(hb1_lin, hb1_length, frame_size, 2);

    for (auto _ : state) {
        ds->up(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}

void resample_up(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size << 1];
    inp[0] = 1.0;
    resample *ds = new resample(hb1_lin, hb1_length, frame_size, 2, 1);

    for (auto _ : state) {
        ds->resamp(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}

void resample_down(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size >> 1];
    inp[0] = 1.0;
    resample *ds = new resample(hb1_lin, hb1_length, frame_size, 1, 2);

    for (auto _ : state) {
        ds->resamp(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}
// 32 -> 48 bytes upsampling
void resample_3_2(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size << 1];
    inp[0] = 1.0;
    resample *ds = new resample(filter_32_48, FILTER_32_48_SIZE, frame_size, 3, 2);

    for (auto _ : state) {
        ds->resamp(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}

// 48 -> 32 bytes upsampling
void resample_2_3(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size << 1];
    inp[0] = 1.0;
    resample *ds = new resample(filter_48_32, FILTER_48_32_SIZE, frame_size, 2, 3);

    for (auto _ : state) {
        ds->resamp(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}
void resample_1_16(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size];
    inp[0] = 1.0;
    resample *ds = new resample(hb1_lin, hb1_length, frame_size, 1, 16);

    for (auto _ : state) {
        ds->resamp(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete ds;
}

BENCHMARK(resample_down)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
BENCHMARK(ResampleDown_bench)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

BENCHMARK(resample_up)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
BENCHMARK(ResampleUp_bench)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK(resample_3_2)->Arg(32);
BENCHMARK(resample_2_3)->Arg(48);
BENCHMARK(resample_1_16)->Arg(32);

BENCHMARK_MAIN();
