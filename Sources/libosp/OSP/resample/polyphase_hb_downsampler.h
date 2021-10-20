//======================================================================================================================
/** @file polyphase_hb_downsampler.h
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

#ifndef OPENSPEECHPLATFORMLIBRARIES_HB_DOWNSAMPLER_H
#define OPENSPEECHPLATFORMLIBRARIES_HB_DOWNSAMPLER_H

#include <OSP/circular_buffer/circular_buffer.hpp>
#include <OSP/filter/fir_formii.h>
#include <sys/types.h>

class polyphase_hb_downsampler {

  public:
    polyphase_hb_downsampler(const float *filter_taps, const uint num_taps, const uint max_frame_size);
    ~polyphase_hb_downsampler();
    void process(float *in, float *out, const uint frame_size);

  private:
    fir_formii *even_filter;
    circular_buffer *odd_filter;
    float *even_data;
    float *even_filter_taps;
    float *odd_data;
    float middle_tap;
    uint delay;
    uint half_frame_size;
};

#endif // OPENSPEECHPLATFORMLIBRARIES_HB_DOWNSAMPLER_H
