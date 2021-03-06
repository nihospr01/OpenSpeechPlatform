//======================================================================================================================
/** @file filter.hpp
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

//
// Last Modified by Dhiman Sengupta on 2019-03-08.
//

#ifndef OSP_FILTER_H
#define OSP_FILTER_H

#include <OSP/circular_buffer/circular_buffer.hpp>
#include <atomic>
#include <sys/types.h>

/**
 * @brief Filter Class
 * @details This filter class implements the FIR filter
 */
class filter {

  public:
    /**
     * @brief Filter constructor
     * @param[in] taps The filter taps for this FIR filter
     * @param[in] tap_size The number of taps of this FIR filter
     * @param[in] cir_buf The circular buffer for this FIR filter to perform frame-based convolution
     * @param[in] max_buf_size The maximum size of circular buffer you need to specify if there is no circular buffer
     * given in cir_buf
     */
    explicit filter(float *taps, uint tap_size, circular_buffer *cir_buf, uint max_buf_size);

    /**
     * @brief Filter destructor
     */
    ~filter();

    /**
     * @brief Setting the filter taps
     * @param[in] taps The filter taps (an 1-D array)
     * @param[in] buf_size The size of the filter taps (this should be the same as tap_size passed in constructor)
     * @return A flag indicating the success of setting the filter taps
     */
    int set_taps(const float *taps, uint buf_size);

    /**
     * @brief Getting the filter taps
     * @param[out] taps The filter taps (1-D array)
     * @param[in] buf_size The size of the filter taps (this should be the same as tap_size passed in constructor)
     * @return A flag indicating the success of getting the filter taps
     */
    int get_taps(float *taps, uint buf_size);

    /**
     * @brief Getting the output of this FIR filter by performing frame-based convolution
     * @param[in] data_in The input signal
     * @param[out] data_out The output signal
     * @param num_samp The size of input and output signal (data_in and data_out should have the same size)
     */
    void cirfir(float *data_in, float *data_out, uint num_samp);

    /**
     * @brief Getting the number of taps of this FIR filter
     * @return The number of taps of this FIR filter
     */
    uint get_size();

  private:
    void cirfir(float *data_out, uint num_samp);

    std::atomic<float *> tap_current, tap_next;
    uint size_;
    circular_buffer *cir_buf_;
    bool cir_buf_created_;
};

#endif // OSP_FILTER_H
