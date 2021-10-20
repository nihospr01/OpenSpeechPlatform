//======================================================================================================================
/** @file resample_down.cpp
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

// Based on code from CMSIS
// https://github.com/ARM-software/CMSIS_5/blob/develop/CMSIS/DSP/Source/FilteringFunctions

/*
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "resample_up.h"
#include <cassert>

ResampleUp::ResampleUp(const float *filter_taps, const uint numTaps, const uint max_frame_size, const uint interp_factor)
    : num_taps(numTaps), L(interp_factor) {
    
    use_polyphase = false;
#ifdef __ARM_ARCH
    if (((num_taps+1) % L) == 0U) {
        use_polyphase = true;
        num_taps++;
        taps = new float[num_taps];
        // reverse the order of the taps
        for (uint i = 1; i < num_taps; i++)
            taps[i] = filter_taps[num_taps - i - 1];
        taps[0] = 0.0;
        phaseLength = num_taps / L;
        fir_state = new float[max_frame_size + phaseLength - 1]();
        return;
    }
#endif
    filter = new fir_formii(filter_taps, num_taps, max_frame_size * interp_factor);
    up_sample_buf = new float[max_frame_size * interp_factor]();
}

ResampleUp::~ResampleUp() {
    if (use_polyphase) {
        delete[] taps;
        delete[] fir_state;
    } else {
        delete[] up_sample_buf;
        delete filter;
    }
}


void ResampleUp::up(float *in, float *out, uint frame_size) {
#ifdef __ARM_ARCH
    if (use_polyphase)
        return this->up_arm(in, out, frame_size);
#endif

    for (uint i = 0; i < frame_size; i++) {
        up_sample_buf[i*L] = in[i];
    }
    filter->process(up_sample_buf, out, frame_size * L);
}

#ifdef __ARM_ARCH
#include <arm_neon.h>
void ResampleUp::up_arm(float *pSrc, float *pDst, uint blockSize) {
    float *pState = fir_state; /* State pointer */
    float *pStateCurnt;        /* Points to the current sample of the state */
    float *ptr1;               /* Temporary pointers for state buffer */
    const float *ptr2;         /* Temporary pointers for coefficient buffer */
    float sum0;                /* Accumulators */
    float c0;                  /* Temporary variables to hold state and coefficient values */
    uint i, blkCnt, j;         /* Loop counters */
    uint tapCnt;               /* Length of each polyphase filter component */
    uint blkCntN4;
    float c1, c2, c3;

    float32x4_t sum0v;
    float32x4_t accV0, accV1;
    float32x4_t x0v, x1v, x2v, xa, xb;
    float32x2_t tempV;

    /* fir_state buffer contains previous frame (phaseLength - 1) samples */
    /* pStateCurnt points to the location where the new input data should be written */
    pStateCurnt = fir_state + (phaseLength - 1U);

    /* Initialise  blkCnt */
    blkCnt = blockSize >> 3;
    blkCntN4 = blockSize & 7;

    /* Loop unrolling */
    while (blkCnt > 0U) {
        /* Copy new input samples into the state buffer */
        sum0v = vld1q_f32(pSrc);
        vst1q_f32(pStateCurnt, sum0v);
        pSrc += 4;
        pStateCurnt += 4;

        sum0v = vld1q_f32(pSrc);
        vst1q_f32(pStateCurnt, sum0v);
        pSrc += 4;
        pStateCurnt += 4;

        /* Address modifier index of coefficient buffer */
        j = 1U;

        /* Loop over the Interpolation factor. */
        i = (L);

        while (i > 0U) {
            /* Set accumulator to zero */
            accV0 = vdupq_n_f32(0.0);
            accV1 = vdupq_n_f32(0.0);

            /* Initialize state pointer */
            ptr1 = pState;

            /* Initialize coefficient pointer */
            ptr2 = taps + (L - j);

            /* Loop over the polyPhase length. Unroll by a factor of 4.
             ** Repeat until we've computed numTaps-(4*L) coefficients. */
            tapCnt = phaseLength >> 2U;

            x0v = vld1q_f32(ptr1);
            x1v = vld1q_f32(ptr1 + 4);

            while (tapCnt > 0U) {
                /* Read the input samples */
                x2v = vld1q_f32(ptr1 + 8);

                /* Read the coefficients */
                c0 = *(ptr2);

                /* Perform the multiply-accumulate */
                accV0 = vmlaq_n_f32(accV0, x0v, c0);
                accV1 = vmlaq_n_f32(accV1, x1v, c0);

                /* Read the coefficients, inputs and perform multiply-accumulate */
                c1 = *(ptr2 + L);

                xa = vextq_f32(x0v, x1v, 1);
                xb = vextq_f32(x1v, x2v, 1);

                accV0 = vmlaq_n_f32(accV0, xa, c1);
                accV1 = vmlaq_n_f32(accV1, xb, c1);

                /* Read the coefficients, inputs and perform multiply-accumulate */
                c2 = *(ptr2 + L * 2);

                xa = vextq_f32(x0v, x1v, 2);
                xb = vextq_f32(x1v, x2v, 2);

                accV0 = vmlaq_n_f32(accV0, xa, c2);
                accV1 = vmlaq_n_f32(accV1, xb, c2);

                /* Read the coefficients, inputs and perform multiply-accumulate */
                c3 = *(ptr2 + L * 3);

                xa = vextq_f32(x0v, x1v, 3);
                xb = vextq_f32(x1v, x2v, 3);

                accV0 = vmlaq_n_f32(accV0, xa, c3);
                accV1 = vmlaq_n_f32(accV1, xb, c3);

                /* Upsampling is done by stuffing L-1 zeros between each sample.
                 * So instead of multiplying zeros with coefficients,
                 * Increment the coefficient pointer by interpolation factor times. */
                ptr2 += 4 * L;
                ptr1 += 4;
                x0v = x1v;
                x1v = x2v;

                /* Decrement the loop counter */
                tapCnt--;
            }

            /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
            tapCnt = phaseLength % 0x4U;

            x2v = vld1q_f32(ptr1 + 8);

            switch (tapCnt) {
            case 3:
                c0 = *(ptr2);
                accV0 = vmlaq_n_f32(accV0, x0v, c0);
                accV1 = vmlaq_n_f32(accV1, x1v, c0);
                ptr2 += L;

                c0 = *(ptr2);

                xa = vextq_f32(x0v, x1v, 1);
                xb = vextq_f32(x1v, x2v, 1);

                accV0 = vmlaq_n_f32(accV0, xa, c0);
                accV1 = vmlaq_n_f32(accV1, xb, c0);
                ptr2 += L;

                c0 = *(ptr2);

                xa = vextq_f32(x0v, x1v, 2);
                xb = vextq_f32(x1v, x2v, 2);

                accV0 = vmlaq_n_f32(accV0, xa, c0);
                accV1 = vmlaq_n_f32(accV1, xb, c0);
                ptr2 += L;

                break;

            case 2:
                c0 = *(ptr2);
                accV0 = vmlaq_n_f32(accV0, x0v, c0);
                accV1 = vmlaq_n_f32(accV1, x1v, c0);
                ptr2 += L;

                c0 = *(ptr2);

                xa = vextq_f32(x0v, x1v, 1);
                xb = vextq_f32(x1v, x2v, 1);

                accV0 = vmlaq_n_f32(accV0, xa, c0);
                accV1 = vmlaq_n_f32(accV1, xb, c0);
                ptr2 += L;

                break;

            case 1:
                c0 = *(ptr2);
                accV0 = vmlaq_n_f32(accV0, x0v, c0);
                accV1 = vmlaq_n_f32(accV1, x1v, c0);
                ptr2 += L;

                break;

            default:
                break;
            }

            /* The result is in the accumulator, store in the destination buffer. */
            *pDst = vgetq_lane_f32(accV0, 0);
            *(pDst + L) = vgetq_lane_f32(accV0, 1);
            *(pDst + 2 * L) = vgetq_lane_f32(accV0, 2);
            *(pDst + 3 * L) = vgetq_lane_f32(accV0, 3);

            *(pDst + 4 * L) = vgetq_lane_f32(accV1, 0);
            *(pDst + 5 * L) = vgetq_lane_f32(accV1, 1);
            *(pDst + 6 * L) = vgetq_lane_f32(accV1, 2);
            *(pDst + 7 * L) = vgetq_lane_f32(accV1, 3);

            pDst++;

            /* Increment the address modifier index of coefficient buffer */
            j++;

            /* Decrement the loop counter */
            i--;
        }

        /* Advance the state pointer by 1
         * to process the next group of interpolation factor number samples */
        pState = pState + 8;

        pDst += L * 7;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
     ** No loop unrolling is used. */

    while (blkCntN4 > 0U) {
        /* Copy new input sample into the state buffer */
        *pStateCurnt++ = *pSrc++;

        /* Address modifier index of coefficient buffer */
        j = 1U;

        /* Loop over the Interpolation factor. */
        i = L;

        while (i > 0U) {
            /* Set accumulator to zero */
            sum0v = vdupq_n_f32(0.0);

            /* Initialize state pointer */
            ptr1 = pState;

            /* Initialize coefficient pointer */
            ptr2 = taps + (L - j);

            /* Loop over the polyPhase length. Unroll by a factor of 4.
             ** Repeat until we've computed numTaps-(4*L) coefficients. */
            tapCnt = phaseLength >> 2U;

            while (tapCnt > 0U) {
                /* Read the coefficient */
                x1v = vsetq_lane_f32(*(ptr2), x1v, 0);

                /* Upsampling is done by stuffing L-1 zeros between each sample.
                 * So instead of multiplying zeros with coefficients,
                 * Increment the coefficient pointer by interpolation factor times. */
                ptr2 += L;

                /* Read the input sample */
                x0v = vld1q_f32(ptr1);
                ptr1 += 4;

                /* Read the coefficient */
                x1v = vsetq_lane_f32(*(ptr2), x1v, 1);

                /* Increment the coefficient pointer by interpolation factor times. */
                ptr2 += L;

                /* Read the coefficient */
                x1v = vsetq_lane_f32(*(ptr2), x1v, 2);

                /* Increment the coefficient pointer by interpolation factor times. */
                ptr2 += L;

                /* Read the coefficient */
                x1v = vsetq_lane_f32(*(ptr2), x1v, 3);

                /* Increment the coefficient pointer by interpolation factor times. */
                ptr2 += L;

                sum0v = vmlaq_f32(sum0v, x0v, x1v);

                /* Decrement the loop counter */
                tapCnt--;
            }

            tempV = vpadd_f32(vget_low_f32(sum0v), vget_high_f32(sum0v));
            sum0 = vget_lane_f32(tempV, 0) + vget_lane_f32(tempV, 1);

            /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
            tapCnt = phaseLength % 0x4U;

            while (tapCnt > 0U) {
                /* Perform the multiply-accumulate */
                sum0 += *(ptr1++) * (*ptr2);

                /* Increment the coefficient pointer by interpolation factor times. */
                ptr2 += L;

                /* Decrement the loop counter */
                tapCnt--;
            }

            /* The result is in the accumulator, store in the destination buffer. */
            *pDst++ = sum0;

            /* Increment the address modifier index of coefficient buffer */
            j++;

            /* Decrement the loop counter */
            i--;
        }

        /* Advance the state pointer by 1
         * to process the next group of interpolation factor number samples */
        pState = pState + 1;

        /* Decrement the loop counter */
        blkCntN4--;
    }

    /* Processing is complete.
     ** Now copy the last phaseLength - 1 samples to the satrt of the state buffer.
     ** This prepares the state buffer for the next function call. */

    /* Points to the start of the state buffer */
    pStateCurnt = fir_state;

    tapCnt = (phaseLength - 1U) >> 2U;

    /* Copy data */
    while (tapCnt > 0U) {
        sum0v = vld1q_f32(pState);
        vst1q_f32(pStateCurnt, sum0v);
        pState += 4;
        pStateCurnt += 4;

        /* Decrement the loop counter */
        tapCnt--;
    }

    tapCnt = (phaseLength - 1U) % 0x04U;

    /* copy data */
    while (tapCnt > 0U) {
        *pStateCurnt++ = *pState++;

        /* Decrement the loop counter */
        tapCnt--;
    }
}
#endif
