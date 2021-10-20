//======================================================================================================================
/** @file wdrc.cpp
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

#include "wdrc.hpp"
#include "OSP/array_utilities/array_utilities.hpp"
#include <OSP/subband/wdrc.hpp>

wdrc::wdrc(float gain50, float gain80, float knee_low, float mpo_limit, float cal_mic, float cal_rec, float cal_mpo) {

    std::shared_ptr<wdrc_param_t> data_current = std::make_shared<wdrc_param_t>();

    data_current->gain50_ = gain50;
    data_current->gain80_ = gain80;
    data_current->knee_low_ = knee_low;
    data_current->mpo_limit_ = mpo_limit;

    data_current->slope_ = (gain80 - gain50) / 30.0f;                         // Compression Ratio = 1.0/(1+slope)
    data_current->glow_ = gain50 - data_current->slope_ * (50.0f - knee_low); // Gain in dB at lower kneepoint
    releasePool.add(data_current);
    std::atomic_store(&currentParam, data_current);

    std::shared_ptr<wdrc_cal_t> cal_next = std::make_shared<wdrc_cal_t>();
    cal_next->cal_mic_ = cal_mic;
    cal_next->cal_rec_ = cal_rec;
    cal_next->cal_mpo_ = cal_mpo;
    releasePool.add(cal_next);
    std::atomic_store(&currentCal, cal_next);
}

wdrc::~wdrc() { releasePool.release(); }

void wdrc::set_param(float gain50, float gain80, float knee_low, float mpo_limit) {
    std::shared_ptr<wdrc_param_t> data_next = std::make_shared<wdrc_param_t>();
    data_next->gain50_ = gain50;
    data_next->gain80_ = gain80;
    data_next->knee_low_ = knee_low;
    data_next->slope_ = (gain80 - gain50) / 30.0f;                      // Compression Ratio = 1.0/(1+slope)
    data_next->glow_ = gain50 - data_next->slope_ * (50.0f - knee_low); // Gain in dB at lower kneepoint
    data_next->mpo_limit_ = mpo_limit;

    releasePool.add(data_next);
    std::atomic_store(&currentParam, data_next);
    releasePool.release();
}

void wdrc::get_param(float &gain50, float &gain80, float &knee_low, float &mpo_limit) {
    std::shared_ptr<wdrc_param_t> data_current = std::atomic_load(&currentParam);
    gain50 = data_current->gain50_;
    gain80 = data_current->gain80_;
    knee_low = data_current->knee_low_;
    mpo_limit = data_current->mpo_limit_;
}

void wdrc::process(float *input, float *pdb, uint in_len, float *output) {
    float g; // variable to store gain in dB
    std::shared_ptr<wdrc_param_t> data_current = std::atomic_load(&currentParam);
    std::shared_ptr<wdrc_cal_t> cal_current = std::atomic_load(&currentCal);

    array_add_const(pdb, cal_current->cal_mic_, in_len);

    float knee_low_ = data_current->knee_low_;
    float glow_ = data_current->glow_;
    float mpo_limit_ = data_current->mpo_limit_ + cal_current->cal_mpo_;

    // Compute the compression gain in dB using the peak detector dB values and write compressed signal in out
    for (uint i = 0; i < in_len; i++) {
        if (pdb[i] < knee_low_) {
            g = glow_; // Linear gain below lower kneepoint
        } else {
            g = glow_ + data_current->slope_ * (pdb[i] - knee_low_); // Compression in between kneepoints
        }
        g = g + cal_current->cal_rec_;
        if ((pdb[i] + g) >= mpo_limit_) {
            g = mpo_limit_ - pdb[i]; // Above MPO limit
        }

        // Apply the compression gain to the signal in the band
        g = powf(10.0f, g / 20.0f); // Convert the gain to linear scale
        output[i] = input[i] * g;   // Multiply the signal by the gain
    }
}

void wdrc::get_calibration(float &cal_mic, float &cal_rec, float &cal_mpo) {
    std::shared_ptr<wdrc_cal_t> cal_current = std::atomic_load(&currentCal);
    cal_mic = cal_current->cal_mic_;
    cal_rec = cal_current->cal_rec_;
    cal_mpo = cal_current->cal_mpo_;
}

void wdrc::set_calibration(float cal_mic, float cal_rec, float cal_mpo) {
    std::shared_ptr<wdrc_param_t> data_current = std::atomic_load(&currentParam);
    std::shared_ptr<wdrc_cal_t> cal_current = std::atomic_load(&currentCal);
    std::shared_ptr<wdrc_param_t> data_next = std::make_shared<wdrc_param_t>();
    std::shared_ptr<wdrc_cal_t> cal_next = std::make_shared<wdrc_cal_t>();
    cal_next->cal_mic_ = cal_mic;
    cal_next->cal_rec_ = cal_rec;
    cal_next->cal_mpo_ = cal_mpo;

    releasePool.add(cal_next);
    std::atomic_store(&currentCal, cal_next);
    releasePool.release();
}
