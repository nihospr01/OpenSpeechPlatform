// NO LONGER USED
// Left as a reference.


//======================================================================================================================
/** @file hb_sample.cpp
 *  @author Open Speech Platform (OSP) Team, UCSD
 *  @copyright Copyright (C) 2021 Regents of the University of California Redistribution and use in source and binary
 *  forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *      1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *      following disclaimer.
 *
 *      2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *      following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//======================================================================================================================

#include <cassert>
#include <iostream>
#include <cstddef>

class HBSample {
  public:
    HBSample(const float *filter_taps, const uint tap_length);
    ~HBSample();
    void up(float *in, float *out, uint frame_size);
    void down(float *in, float *out, uint frame_size);

  private:
    float *c_even;
    float *n_even;
    float *c_odd;
    float *n_odd;
    float *even_taps;
    float *odd_taps;
    float delay = 0;
    unsigned num_taps;
};

HBSample::HBSample(const float *filter_taps, const uint tap_length) {
    assert((tap_length & 0x1) == 1); // odd number of taps required

    // split taps into even and odd
    num_taps = unsigned((tap_length + 1) / 2);
    even_taps = new float[num_taps];
    odd_taps = new float[num_taps];

    int n = 0;
    for (unsigned i = 1; i <= (unsigned)tap_length; i++) {
        if (i & 0x1)
            odd_taps[n] = filter_taps[i - 1];
        else
            even_taps[n++] = filter_taps[i - 1];
    }
    even_taps[n] = 0;

    c_even = new float[num_taps]();
    n_even = new float[num_taps]();
    c_odd = new float[num_taps]();
    n_odd = new float[num_taps]();
}

HBSample::~HBSample() {
    delete[] c_odd;
    delete[] c_even;
    delete[] n_odd;
    delete[] n_even;
    delete[] even_taps;
    delete[] odd_taps;
}

void HBSample::up(float *__restrict in, float *__restrict out, uint frame_size) {
    unsigned nt = num_taps - 1;
    for (unsigned i = 0; i < (unsigned)frame_size; i++) {
        float x = in[i];

        for (unsigned j = 0; j < nt; j++)
            n_odd[j] = c_odd[j + 1] + odd_taps[j] * x;
        n_odd[nt] = odd_taps[nt] * x;
        *out++ = n_odd[0];
        std::swap(c_odd, n_odd);

        for (unsigned j = 0; j < nt; j++)
            n_even[j] = c_even[j + 1] + even_taps[j] * x;
        n_even[nt] = even_taps[nt] * x;
        *out++ = n_even[0];
        std::swap(c_even, n_even);
    }
}

void HBSample::down(float *__restrict in, float *__restrict out, uint frame_size) {
    for (unsigned i = 0; i < (unsigned)frame_size; i += 2) {
        for (unsigned j = 0; j < num_taps - 1; j++)
            n_odd[j] = c_odd[j + 1] + odd_taps[j] * in[i];
        n_odd[num_taps - 1] = odd_taps[num_taps - 1] * in[i];

        for (unsigned j = 0; j < num_taps - 1; j++)
            n_even[j] = c_even[j + 1] + even_taps[j] * delay;
        n_even[num_taps - 1] = even_taps[num_taps - 1] * delay;

        delay = in[i + 1];
        *out++ = n_odd[0] + n_even[0];
        std::swap(c_odd, n_odd);
        std::swap(c_even, n_even);
    }
}