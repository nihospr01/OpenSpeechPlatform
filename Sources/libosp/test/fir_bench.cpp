// Benchmark FIR Filters

#include <benchmark/benchmark.h>

#include <OSP/filter/fir_formii.h>
#include <OSP/multirate_filterbank/elevenband_filtercoef.hpp>
#include <iostream>

using namespace std;

void fir_bench(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);

    float *inp = new float[frame_size]();
    float *out = new float[frame_size];
    inp[0] = 1.0;

    fir_formii *f = new fir_formii(&elevenband_taps_min[10][0], elevenband_taps_min[10].size(), frame_size);

    for (auto _ : state) {
        f->process(inp, out, frame_size);        
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
}

// this is the previous version
class fir_formii_old {

  public:
    fir_formii_old(const float *const taps, uint num_taps);
    ~fir_formii_old();
    void process(const float *data_in, float *data_out, uint frame_size);

  private:
    float *taps;
    float *delays1;
    float *delays2;
    uint num_taps;
};
fir_formii_old::fir_formii_old(const float *const taps_, uint num_taps) : num_taps(num_taps) {
    taps = new float[num_taps];
    memcpy(taps, taps_, num_taps * sizeof(float));
    delays1 = new float[num_taps]();
    delays2 = new float[num_taps]();
}

fir_formii_old::~fir_formii_old() {
    delete[] taps;
    delete[] delays1;
    delete[] delays2;
}

void fir_formii_old::process(const float *data_in, float *data_out, uint frame_size) {

    float *current_delay_block = delays1;
    float *next_delay_block = delays2;

    for (uint i = 0; i < frame_size; i++) {
        for (uint j = 0; j < num_taps - 1; j++) {
            next_delay_block[j] = current_delay_block[j + 1] + taps[j] * data_in[i];
        }
        next_delay_block[num_taps - 1] = taps[num_taps - 1] * data_in[i];
        data_out[i] = next_delay_block[0];
        std::swap(current_delay_block, next_delay_block);
    }
}
void fir_bench_old(benchmark::State &state) {
    uint frame_size = (uint)state.range(0);
    float *inp = new float[frame_size]();
    float *out = new float[frame_size];
    inp[0] = 1.0;

    // band 10 min_phase
    fir_formii_old *f = new fir_formii_old(&elevenband_taps_min[10][0], elevenband_taps_min[10].size());

    for (auto _ : state) {
        f->process(inp, out, frame_size);
        benchmark::ClobberMemory();
    }
    delete[] inp;
    delete[] out;
    delete f;
}

BENCHMARK(fir_bench)->Arg(32);
BENCHMARK(fir_bench_old)->Arg(32);
BENCHMARK_MAIN();
