//======================================================================================================================
/** @file wdrc.hpp
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

#ifndef OSP_WDRC_H
#define OSP_WDRC_H

#include <OSP/GarbageCollector/GarbageCollector.hpp>
#include <atomic>
#include <cmath>
#include <sys/types.h>

/**
 * @brief Wide Dynamic Range Compression (WDRC) Class
 * @details Applying WDRC to a subband signal from an analysis filterbank
 */

class wdrc {

  public:
    /**
     * @brief wdrc constructor
     * @param[in] gain50 Gain at 50 dB SPL of input level
     * @param[in] gain80 Gain at 80 dB SPL of input level
     * @param[in] knee_low Lower knee-point
     * @param[in] mpo_limit Maximum power output (MPO)
     * @param[in] cal_mic Input power calibration in dB (Default 0dB)
     * @param[in] cal_rec Output power calibration in dB (Default 0 dB)
     * @param[in] cal_mpo Maximum Power Output calibration in dB (Default 0 dB)
     */
    explicit wdrc(float gain50, float gain80, float knee_low, float mpo_limit, float cal_mic = 0.0f,
                  float cal_rec = 0.0f, float cal_mpo = 0.0f);

    /**
     * @brief wdrc destructor
     */
    ~wdrc();

    /**
     * @brief Setting the calibration to ensure that WDRC is reporting the proper values.
     * @param[in] cal_mic Calibrates for discrepencies due to the microphone
     * @param[in] cal_rec Calibrates for discrepencies due to the receiver
     * @param[in] cal_mpo Maximum Power Output calibration in dB
     */
    void set_calibration(float cal_mic, float cal_rec, float cal_mpo);

    /**
     * @brief Getting the calibration to ensure that WDRC is reporting the proper values.
     * @param[out] cal_mic Calibrates for discrepencies due to the microphone
     * @param[out] rec_cal Calibrates for discrepencies due to the receiver
     * @param[out] cal_mpo Maximum Power Output calibration in dB
     */
    void get_calibration(float &cal_mic, float &rec_cal, float &cal_mpo);

    /**
     * @brief Setting WDRC parameters
     * @param[in] gain50 Gain at 50 dB SPL of input level
     * @param[in] gain80 Gain at 80 dB SPL of input level
     * @param[in] knee_low Lower knee-point
     * @param[in] mpo_limit MPO
     */
    void set_param(float gain50, float gain80, float knee_low, float mpo_limit);

    /**
     * @brief Getting WDRC parameters
     * @param[out] gain50 Gain at 50 dB SPL of input level
     * @param[out] gain80 Gain at 80 dB SPL of input level
     * @param[out] knee_low Lower knee-point
     * @param[out] mpo_limit MPO
     */
    void get_param(float &gain50, float &gain80, float &knee_low, float &mpo_limit);

    /**
     * @brief Perform WDRC
     * @details The peak detector output in dB SPL is needed as one of the inputs.
     * The gain at 50 and 80 dB SPL is specified for the frequency sub-band, along with the lower and upper kneepoints
     * in dB SPL. The compressor is linear below the lower kneepoint and applies compression limiting above the upper
     * kneepoint
     * @param[in] input The input signal (1-D array)
     * @param[in] pdb The output from the peak detector in SPL, i.e., the output from get_spl member function in
     * peak_detect class
     * @param[in] in_len Length of the input signal
     * @param[out] output Pointer to a signal (1-D array) where the compressed output of the subband signal will be
     * written, i.e., the output of WDRC
     * @see peak_detect
     */
    void process(float *input, float *pdb, uint in_len, float *output);

  private:
    struct wdrc_param_t {

        float mpo_limit_;
        float gain50_;
        float gain80_;
        float slope_;
        float glow_;
        float knee_low_;
    };

    struct wdrc_cal_t {

        float cal_mic_;
        float cal_rec_;
        float cal_mpo_;
    };

    std::shared_ptr<wdrc_param_t> currentParam;
    std::shared_ptr<wdrc_cal_t> currentCal;
    GarbageCollector releasePool;
};

#endif // OSP_WDRC_H
