//======================================================================================================================
/** @file polyphase_hb_downsampler.cpp
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

#include "polyphase_hb_downsampler.h"
#include <OSP/array_utilities/array_utilities.hpp>
#include <cassert>
#include <iostream>

polyphase_hb_downsampler::polyphase_hb_downsampler(const float *filter_taps, const uint num_taps,
                                                   const uint max_frame_size) {
    assert((max_frame_size & 0x1) == 0);
    assert((num_taps & 0x1) == 1);
    even_filter_taps = new float[(num_taps + 1) >> 1];
    odd_data = new float[max_frame_size >> 1]();
    even_data = new float[max_frame_size >> 1]();
    middle_tap = filter_taps[(num_taps - 1) >> 1];
    delay = ((num_taps + 1) >> 2);
    half_frame_size = max_frame_size >> 1;
    for (uint i = 0; i < num_taps; i += 2) {
        even_filter_taps[i >> 1] = filter_taps[i];
        // std::cout << "Up Even Filter i = " << i << " = " << even_filter_taps[i >> 1] << std::endl;
    }
    // std::cout << "Up Middle Tap " << middle_tap << std::endl;
    // std::cout << "Delay = " << delay << std::endl;
    even_filter = new fir_formii(even_filter_taps, (num_taps + 1) >> 1, max_frame_size);
    odd_filter = new circular_buffer(delay + half_frame_size, 0);
}
polyphase_hb_downsampler::~polyphase_hb_downsampler() {
    delete[] even_filter_taps;
    delete[] odd_data;
    delete[] even_data;
    delete even_filter;
    delete odd_filter;
}
void polyphase_hb_downsampler::process(float *in, float *out, const uint frame_size) {

    /* Commutator */
    unsigned half_frame = frame_size >> 1;

    for (unsigned i = 0; i < frame_size; i += 2) {
        even_data[i >> 1] = in[i];
        odd_data[i >> 1] = in[i + 1];
    }

    even_filter->process(even_data, even_data, half_frame);
    odd_filter->set(odd_data, half_frame);
    odd_filter->delay_block(out, half_frame, delay);

    array_multiply_const(out, middle_tap, half_frame);
    array_add_array(out, even_data, half_frame);
    array_multiply_const(out, 2.0, half_frame);
}