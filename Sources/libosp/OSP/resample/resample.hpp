//======================================================================================================================
/** @file resample.h
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
//======================================================================================================================ifndef

#ifndef OSP_RESAMPLE_H
#define OSP_RESAMPLE_H

#include <OSP/resample/resample_down.h>
#include <OSP/resample/resample_up.h>
#include <cstddef>

/**
 * @brief Resample Class
 * @details Resampling class implements L/M-fold resampling
 */

class resample {

  public:
    /**
     * @brief Resample constructor
     * @param[in] taps The filter taps of the lowpass filter (to reject images and prevent aliasing)
     * @param[in] num_taps The number of taps of the lowpass filter
     * @param[in] max_frame_size The maximum input buffer size
     * @param[in] interp_factor The interpolation factor L (to implement L-fold expander)
     * @param[in] deci_factor The decimation factor M (to implement M-fold decimator)
     */
    resample(const float *filter_taps, const uint num_taps, const uint max_frame_size, const uint interp_factor,
             const uint deci_factor);
    /**
     * @brief Resample destructor
     */
    ~resample();

    /**
     * @brief Getting the resampled signal
     * @param[in] data_in The signal in original sampling rate
     * @param[out] data_out The resampled signal
     * @param[in] in_len The size of the original signal
     */
    void resamp(float *data_in, float *data_out, uint in_len);

  private:
    uint interp_factor;
    uint deci_factor;
    float *buff;           // temporary internal buffer
    ResampleUp *ifilter;   // Upsampler (if necessary)
    ResampleDown *dfilter; // Downsampler (if necessary)

    /**
     * @brief The decimator
     * @param data_in The input signal
     * @param data_out The decimated signal (output of the decimator)
     * @param in_len The size of the input signal
     */
    void decimate(float *data_in, float *data_out, uint in_len);
};

#endif // OSP_RESAMPLE_H
