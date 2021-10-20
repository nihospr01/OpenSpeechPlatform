//======================================================================================================================
/** @file peak_detect.cpp
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

#include <OSP/subband/peak_detect.hpp>
#include <cmath>

peak_detect::peak_detect(float fsamp, float attack_time, float release_time) {
    std::shared_ptr<peak_detect_t> data_current = std::make_shared<peak_detect_t>();
    fsamp_ = fsamp;
    prev_peak = 0;

    releasePool.add(data_current);
    std::atomic_store(&currentParam, data_current);
    this->set_param(attack_time, release_time);
}

peak_detect::~peak_detect() { releasePool.release(); }

void peak_detect::set_param(float attack_time, float release_time) {

    std::shared_ptr<peak_detect_t> data_next = std::make_shared<peak_detect_t>();

    data_next->attack_time_ = attack_time;
    data_next->release_time_ = release_time;
    data_next->att_ = 0.001f * attack_time * fsamp_ / 2.425f;
    data_next->alpha_ = data_next->att_ / (1.0f + data_next->att_);
    data_next->rel_ = 0.001f * release_time * fsamp_ / 1.782f;
    data_next->beta_ = data_next->rel_ / (1.0f + data_next->rel_);

    releasePool.add(data_next);
    std::atomic_store(&currentParam, data_next);
    releasePool.release();
}

void peak_detect::get_param(float &attack_time, float &release_time) {
    std::shared_ptr<peak_detect_t> data_current = std::atomic_load(&currentParam);

    attack_time = data_current->attack_time_;
    release_time = data_current->release_time_;
}

void peak_detect::get_spl(float *data_in, uint in_len, float *pdb_out) {

    float curr_inp, temp;
    float peak_out[in_len];
    std::shared_ptr<peak_detect_t> data_current = std::atomic_load(&currentParam);
    float alpha_ = data_current->alpha_;
    float beta_ = data_current->beta_;

    peak_out[0] = prev_peak;
    // Loop to peak detect the signal
    for (uint i = 1; i < in_len; i++) {
        temp = data_in[i];
        curr_inp = temp > 0 ? temp : -temp; // Get the rectified signal
        if (curr_inp >= peak_out[i - 1]) {
            peak_out[i] = alpha_ * peak_out[i - 1] + (1 - alpha_) * curr_inp;
        } else {
            peak_out[i] = beta_ * peak_out[i - 1];
        }
    }

    // Setting prev_peak to be the peak of last sample of this frame for band given by band_num
    prev_peak = peak_out[in_len - 1];
    this->peak_to_spl(peak_out, in_len, pdb_out);
}

void peak_detect::peak_to_spl(float *peak_in, uint in_len, float *pdb_out) {
    for (uint i = 0; i < in_len; i++) {
        pdb_out[i] = DEFAULT_LEVEL + 20 * log10f(peak_in[i] + FP_EPSILON); // floato avoid log of 0
    }
}
