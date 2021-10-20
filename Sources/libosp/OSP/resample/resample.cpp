//======================================================================================================================
/** @file resample.cpp
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

#include <OSP/resample/resample.hpp>
#include <cstring>
#include <memory>

resample::resample(const float *filter_taps, const uint num_taps, const uint max_frame_size, const uint interp_factor,
                   const uint deci_factor)
    : deci_factor(deci_factor), interp_factor(interp_factor) {
    if (interp_factor > 1) {
        ifilter = new ResampleUp(filter_taps, num_taps, max_frame_size, interp_factor);
        buff = new float[max_frame_size * interp_factor];
    } else
        dfilter = new ResampleDown(filter_taps, num_taps, max_frame_size, deci_factor);
}

resample::~resample() {
    if (interp_factor > 1) {
        delete[] buff;
        delete ifilter;
    } else {
        delete dfilter;
    }
}

void resample::resamp(float *data_in, float *data_out, uint in_size) {
    if (interp_factor > 1) {
        if (deci_factor == 1) {
            ifilter->up(data_in, data_out, in_size);
            return;
        }
        ifilter->up(data_in, buff, in_size);
        this->decimate(buff, data_out, in_size * interp_factor);
        return;
    }
    dfilter->down(data_in, data_out, in_size);
}

inline void resample::decimate(float *data_in, float *data_out, uint in_len) {
    float *iptr = data_in;
    float *optr = data_out;
    float *endptr = &data_in[in_len]; // 1 past end of array

    while (iptr < endptr) {
        *optr++ = *iptr;
        iptr += deci_factor;
    }
}