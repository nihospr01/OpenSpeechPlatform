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

#include "resample_down.h"
#include <cassert>
#include <iostream>

ResampleDown::ResampleDown(const float *filter_taps, const uint num_taps, const uint max_frame_size, const uint deci_factor)
    : num_taps(num_taps), M(deci_factor) {
    assert((max_frame_size % M) == 0U);

    taps = new float[num_taps];
    // reverse the order of the taps
    for (uint i = 0; i < num_taps; i++)
        taps[i] = filter_taps[num_taps - i - 1];

    fir_state = new float[max_frame_size + num_taps - 1]();
}

ResampleDown::~ResampleDown() {
    delete[] taps;
    delete[] fir_state;
}
#ifdef __ARM_ARCH
#include <arm_neon.h>
void ResampleDown::down(float *pSrc, float *pDst, uint blockSize) {
    float *pState = fir_state;                            /* State pointer */
    float *pStateCurnt;                                   /* Points to the current sample of the state */
    float *px;                                            /* Temporary pointer for state buffer */
    const float *pb;                                      /* Temporary pointer for coefficient buffer */
    float sum0;                                           /* Accumulator */
    float x0, c0;                                         /* Temporary variables to hold state and coefficient values */
    uint i, tapCnt, blkCnt, outBlockSize = blockSize / M; /* Loop counters */

    uint blkCntN4;
    float *px0, *px1, *px2, *px3;
    float x1, x2, x3;

    float32x4_t accv, acc0v, acc1v, acc2v, acc3v;
    float32x4_t x0v, x1v, x2v, x3v;
    float32x4_t c0v;
    float32x2_t temp;
    float32x4_t sum0v;

    /* fir_state buffer contains previous frame (num_taps - 1) samples */
    /* pStateCurnt points to the location where the new input data should be written */
    pStateCurnt = fir_state + (num_taps - 1U);

    /* Total number of output samples to be computed */
    blkCnt = outBlockSize / 4;
    blkCntN4 = outBlockSize - (4 * blkCnt);

    while (blkCnt > 0U) {
        /* Copy 4 * decimation factor number of new input samples into the state buffer */
        i = 4 * M;

        do {
            *pStateCurnt++ = *pSrc++;

        } while (--i);

        /* Set accumulators to zero */
        acc0v = vdupq_n_f32(0.0);
        acc1v = vdupq_n_f32(0.0);
        acc2v = vdupq_n_f32(0.0);
        acc3v = vdupq_n_f32(0.0);

        /* Initialize state pointer for all the samples */
        px0 = pState;
        px1 = pState + M;
        px2 = pState + 2 * M;
        px3 = pState + 3 * M;

        /* Initialize coeff pointer */
        pb = taps;

        /* Process 4 taps at a time. */
        tapCnt = num_taps >> 2;

        /* Loop over the number of taps.
         ** Repeat until we've computed num_taps-4 coefficients. */

        while (tapCnt > 0U) {
            /* Read the b[num_taps-1] coefficient */
            c0v = vld1q_f32(pb);
            pb += 4;

            /* Read x[n-num_taps-1] sample for acc0 */
            x0v = vld1q_f32(px0);
            x1v = vld1q_f32(px1);
            x2v = vld1q_f32(px2);
            x3v = vld1q_f32(px3);

            px0 += 4;
            px1 += 4;
            px2 += 4;
            px3 += 4;

            acc0v = vmlaq_f32(acc0v, x0v, c0v);
            acc1v = vmlaq_f32(acc1v, x1v, c0v);
            acc2v = vmlaq_f32(acc2v, x2v, c0v);
            acc3v = vmlaq_f32(acc3v, x3v, c0v);

            /* Decrement the loop counter */
            tapCnt--;
        }

        temp = vpadd_f32(vget_low_f32(acc0v), vget_high_f32(acc0v));
        accv = vsetq_lane_f32(vget_lane_f32(temp, 0) + vget_lane_f32(temp, 1), accv, 0);

        temp = vpadd_f32(vget_low_f32(acc1v), vget_high_f32(acc1v));
        accv = vsetq_lane_f32(vget_lane_f32(temp, 0) + vget_lane_f32(temp, 1), accv, 1);

        temp = vpadd_f32(vget_low_f32(acc2v), vget_high_f32(acc2v));
        accv = vsetq_lane_f32(vget_lane_f32(temp, 0) + vget_lane_f32(temp, 1), accv, 2);

        temp = vpadd_f32(vget_low_f32(acc3v), vget_high_f32(acc3v));
        accv = vsetq_lane_f32(vget_lane_f32(temp, 0) + vget_lane_f32(temp, 1), accv, 3);

        /* If the filter length is not a multiple of 4, compute the remaining filter taps */
        tapCnt = num_taps % 0x4U;

        while (tapCnt > 0U) {
            /* Read coefficients */
            c0 = *(pb++);

            /* Fetch  state variables for acc0, acc1, acc2, acc3 */
            x0 = *(px0++);
            x1 = *(px1++);
            x2 = *(px2++);
            x3 = *(px3++);

            /* Perform the multiply-accumulate */
            accv = vsetq_lane_f32(vgetq_lane_f32(accv, 0) + x0 * c0, accv, 0);
            accv = vsetq_lane_f32(vgetq_lane_f32(accv, 1) + x1 * c0, accv, 1);
            accv = vsetq_lane_f32(vgetq_lane_f32(accv, 2) + x2 * c0, accv, 2);
            accv = vsetq_lane_f32(vgetq_lane_f32(accv, 3) + x3 * c0, accv, 3);

            /* Decrement the loop counter */
            tapCnt--;
        }

        /* Advance the state pointer by the decimation factor
         * to process the next group of decimation factor number samples */
        pState = pState + 4 * M;

        /* The result is in the accumulator, store in the destination buffer. */
        vst1q_f32(pDst, accv);
        pDst += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    while (blkCntN4 > 0U) {
        /* Copy decimation factor number of new input samples into the state buffer */
        i = M;

        do {
            *pStateCurnt++ = *pSrc++;

        } while (--i);

        /* Set accumulator to zero */
        sum0v = vdupq_n_f32(0.0);

        /* Initialize state pointer */
        px = pState;

        /* Initialize coeff pointer */
        pb = taps;

        /* Process 4 taps at a time. */
        tapCnt = num_taps >> 2;

        /* Loop over the number of taps.
         ** Repeat until we've computed num_taps-4 coefficients. */
        while (tapCnt > 0U) {
            c0v = vld1q_f32(pb);
            pb += 4;

            x0v = vld1q_f32(px);
            px += 4;

            sum0v = vmlaq_f32(sum0v, x0v, c0v);

            /* Decrement the loop counter */
            tapCnt--;
        }

        temp = vpadd_f32(vget_low_f32(sum0v), vget_high_f32(sum0v));
        sum0 = vget_lane_f32(temp, 0) + vget_lane_f32(temp, 1);

        /* If the filter length is not a multiple of 4, compute the remaining filter taps */
        tapCnt = num_taps % 0x4U;

        while (tapCnt > 0U) {
            /* Read coefficients */
            c0 = *(pb++);

            /* Fetch 1 state variable */
            x0 = *(px++);

            /* Perform the multiply-accumulate */
            sum0 += x0 * c0;

            /* Decrement the loop counter */
            tapCnt--;
        }

        /* Advance the state pointer by the decimation factor
         * to process the next group of decimation factor number samples */
        pState = pState + M;

        /* The result is in the accumulator, store in the destination buffer. */
        *pDst++ = sum0;

        /* Decrement the loop counter */
        blkCntN4--;
    }

    /* Processing is complete.
     ** Now copy the last num_taps - 1 samples to the satrt of the state buffer.
     ** This prepares the state buffer for the next function call. */

    /* Points to the start of the state buffer */
    pStateCurnt = fir_state;

    i = (num_taps - 1U) >> 2;

    /* Copy data */
    while (i-- > 0U) {
        sum0v = vld1q_f32(pState);
        vst1q_f32(pStateCurnt, sum0v);
        pState += 4;
        pStateCurnt += 4;
    }

    i = (num_taps - 1U) % 0x04U;

    /* Copy data */
    while (i-- > 0U) {
        *pStateCurnt++ = *pState++;
    }
}
#else

void ResampleDown::down(float *pSrc, float *pDst, uint blockSize) {
    float *pState = fir_state;                            /* State pointer */
    float *pStateCur;                                     /* Points to the current sample of the state */
    float *px0;                                           /* Temporary pointer for state buffer */
    const float *pb;                                      /* Temporary pointer for coefficient buffer */
    float x0, c0;                                         /* Temporary variables to hold state and coefficient values */
    float acc0;                                           /* Accumulator */
    uint i, tapCnt, blkCnt, outBlockSize = blockSize / M; /* Loop counters */

    /* fir_state buffer contains previous frame (num_taps - 1) samples */
    /* pStateCur points to the location where the new input data should be written */
    pStateCur = fir_state + (num_taps - 1U);

    /* Initialize blkCnt with number of samples */
    blkCnt = outBlockSize;

    while (blkCnt-- > 0U) {
        /* Copy decimation factor number of new input samples into the state buffer */
        i = M;

        do {
            *pStateCur++ = *pSrc++;

        } while (--i);

        /* Set accumulator to zero */
        acc0 = 0.0f;

        /* Initialize state pointer */
        px0 = pState;

        /* Initialize coeff pointer */
        pb = taps;

        /* Initialize tapCnt with number of taps */
        tapCnt = num_taps;

        while (tapCnt-- > 0U) {
            /* Read coefficients */
            c0 = *pb++;

            /* Fetch 1 state variable */
            x0 = *px0++;

            /* Perform the multiply-accumulate */
            acc0 += x0 * c0;
        }

        /* Advance the state pointer by the decimation factor
         * to process the next group of decimation factor number samples */
        pState += M;

        /* The result is in the accumulator, store in the destination buffer. */
        *pDst++ = acc0;
    }

    /* Processing is complete.
       Now copy the last num_taps - 1 samples to the satrt of the state buffer.
       This prepares the state buffer for the next function call. */

    /* Points to the start of the state buffer */
    pStateCur = fir_state;

    /* Initialize tapCnt with number of taps */
    tapCnt = (num_taps - 1U);

    /* Copy data */
    while (tapCnt-- > 0U) {
        *pStateCur++ = *pState++;
    }
}
#endif  // __ARM_ARCH

