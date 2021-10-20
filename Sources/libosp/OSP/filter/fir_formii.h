//======================================================================================================================
/** @file fir_formii.h
 *  @author Open Speech Platform (OSP) Team, UCSD
 *  @copyright Copyright (C) 2020 Regents of the University of California Redistribution and use in source and binary
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

#ifndef OSP_FIR_FORMII_H
#define OSP_FIR_FORMII_H

#include <cstring>
#include <memory>
#include <sys/types.h>

/**
 * @brief Filter Class
 * @details This filter class implements the FIR filter
 */
class fir_formii {

  public:
    /**
     * @brief Filter constructor
     * @param[in] taps The filter taps for this FIR filter
     * @param[in] num_taps The number of taps of this FIR filter
     * @param[in] max_frame_size The maximum frame size
     */
    explicit fir_formii(const float *const taps, uint num_taps, uint max_frame_size);

    /**
     * @brief Filter destructor
     */
    ~fir_formii();

    /**
     * @brief Getting the output of this FIR filter by performing frame-based convolution
     * @param[in] data_in The input signal
     * @param[out] data_out The output signal
     * @param frame_size The size of input and output signal (data_in and data_out should have the same size)
     */
    void process(const float *data_in, float *data_out, uint frame_size);

  private:
    float *taps;         /**< points to the coefficient array. The array is of length numTaps. */
    float *fir_state;    /**< points to the state variable array. The array is of length numTaps+max_frame_size-1. */
    const uint num_taps; /**< number of filter coefficients in the filter. */
};

#endif // OSP_FIR_FORMII_H
