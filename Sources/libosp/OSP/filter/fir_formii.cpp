
// Based on code from CMSIS
// https://github.com/ARM-software/CMSIS_5/blob/develop/CMSIS/DSP/Source/FilteringFunctions/arm_fir_f32.c

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

#include "fir_formii.h"

fir_formii::fir_formii(const float *const taps_, uint num_taps, uint max_frame_size) : num_taps(num_taps) {
  taps = new float[num_taps];
    // reverse the order of the taps
    for (uint i = 0; i < num_taps; i++)
        taps[i] = taps_[num_taps - i - 1];

    fir_state = new float[max_frame_size + num_taps - 1]();
}


fir_formii::~fir_formii() {
   delete[] taps;
    delete[] fir_state; 
}

#ifdef __ARM_ARCH
#include <arm_neon.h>
void fir_formii::process(const float *pSrc, float *pDst, uint blockSize) {

    float *pState = fir_state;  /* State pointer */
    float *pStateCurnt;         /* Points to the current sample of the state */
    float *px;                  /* Temporary pointers for state buffer */
    const float *pb;            /* Temporary pointers for coefficient buffer */
    uint32_t i, tapCnt, blkCnt; /* Loop counters */

    float32x4_t accv0, accv1, samples0, samples1, x0, x1, x2, xa, xb, b;
    float acc;

    /* fir_state points to state array which contains previous frame (num_taps - 1) samples */
    /* pStateCurnt points to the location where the new input data should be written */
    pStateCurnt = &(fir_state[(num_taps - 1U)]);

    /* Loop unrolling */
    blkCnt = blockSize >> 3;

    while (blkCnt > 0U) {
        /* Copy 8 samples at a time into state buffers */
        samples0 = vld1q_f32(pSrc);
        vst1q_f32(pStateCurnt, samples0);

        pStateCurnt += 4;
        pSrc += 4;

        samples1 = vld1q_f32(pSrc);
        vst1q_f32(pStateCurnt, samples1);

        pStateCurnt += 4;
        pSrc += 4;

        /* Set the accumulators to zero */
        accv0 = vdupq_n_f32(0);
        accv1 = vdupq_n_f32(0);

        /* Initialize state pointer */
        px = pState;

        /* Initialize coefficient pointer */
        pb = taps;

        /* Loop unroling */
        i = num_taps >> 2;

        /* Perform the multiply-accumulates */
        x0 = vld1q_f32(px);
        x1 = vld1q_f32(px + 4);

        while (i > 0) {
            /* acc =  b[num_taps-1] * x[n-num_taps-1] + b[num_taps-2] * x[n-num_taps-2] + b[num_taps-3] * x[n-num_taps-3]
             * +...+ b[0] * x[0] */
            x2 = vld1q_f32(px + 8);
            b = vld1q_f32(pb);
            xa = x0;
            xb = x1;
            accv0 = vmlaq_n_f32(accv0, xa, vgetq_lane_f32(b, 0));
            accv1 = vmlaq_n_f32(accv1, xb, vgetq_lane_f32(b, 0));

            xa = vextq_f32(x0, x1, 1);
            xb = vextq_f32(x1, x2, 1);

            accv0 = vmlaq_n_f32(accv0, xa, vgetq_lane_f32(b, 1));
            accv1 = vmlaq_n_f32(accv1, xb, vgetq_lane_f32(b, 1));

            xa = vextq_f32(x0, x1, 2);
            xb = vextq_f32(x1, x2, 2);

            accv0 = vmlaq_n_f32(accv0, xa, vgetq_lane_f32(b, 2));
            accv1 = vmlaq_n_f32(accv1, xb, vgetq_lane_f32(b, 2));

            xa = vextq_f32(x0, x1, 3);
            xb = vextq_f32(x1, x2, 3);

            accv0 = vmlaq_n_f32(accv0, xa, vgetq_lane_f32(b, 3));
            accv1 = vmlaq_n_f32(accv1, xb, vgetq_lane_f32(b, 3));

            pb += 4;
            x0 = x1;
            x1 = x2;
            px += 4;
            i--;
        }

        /* Tail */
        i = num_taps & 3;
        x2 = vld1q_f32(px + 8);

        /* Perform the multiply-accumulates */
        switch (i) {
        case 3: {
            accv0 = vmlaq_n_f32(accv0, x0, *pb);
            accv1 = vmlaq_n_f32(accv1, x1, *pb);

            pb++;

            xa = vextq_f32(x0, x1, 1);
            xb = vextq_f32(x1, x2, 1);

            accv0 = vmlaq_n_f32(accv0, xa, *pb);
            accv1 = vmlaq_n_f32(accv1, xb, *pb);

            pb++;

            xa = vextq_f32(x0, x1, 2);
            xb = vextq_f32(x1, x2, 2);

            accv0 = vmlaq_n_f32(accv0, xa, *pb);
            accv1 = vmlaq_n_f32(accv1, xb, *pb);

        } break;
        case 2: {
            accv0 = vmlaq_n_f32(accv0, x0, *pb);
            accv1 = vmlaq_n_f32(accv1, x1, *pb);

            pb++;

            xa = vextq_f32(x0, x1, 1);
            xb = vextq_f32(x1, x2, 1);

            accv0 = vmlaq_n_f32(accv0, xa, *pb);
            accv1 = vmlaq_n_f32(accv1, xb, *pb);

        } break;
        case 1: {

            accv0 = vmlaq_n_f32(accv0, x0, *pb);
            accv1 = vmlaq_n_f32(accv1, x1, *pb);

        } break;
        default:
            break;
        }

        /* The result is stored in the destination buffer. */
        vst1q_f32(pDst, accv0);
        pDst += 4;
        vst1q_f32(pDst, accv1);
        pDst += 4;

        /* Advance state pointer by 8 for the next 8 samples */
        pState = pState + 8;

        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 0x7;

    while (blkCnt > 0U) {
        /* Copy one sample at a time into state buffer */
        *pStateCurnt++ = *pSrc++;

        /* Set the accumulator to zero */
        acc = 0.0f;

        /* Initialize state pointer */
        px = pState;

        /* Initialize Coefficient pointer */
        pb = taps;

        i = num_taps;

        /* Perform the multiply-accumulates */
        do {
            /* acc =  b[num_taps-1] * x[n-num_taps-1] + b[num_taps-2] * x[n-num_taps-2] + b[num_taps-3] * x[n-num_taps-3]
             * +...+ b[0] * x[0] */
            acc += *px++ * *pb++;
            i--;

        } while (i > 0U);

        /* The result is stored in the destination buffer. */
        *pDst++ = acc;

        /* Advance state pointer by 1 for the next sample */
        pState = pState + 1;

        blkCnt--;
    }

    /* Processing is complete.
    ** Now copy the last num_taps - 1 samples to the starting of the state buffer.
    ** This prepares the state buffer for the next function call. */

    /* Points to the start of the state buffer */
    pStateCurnt = fir_state;

    /* Copy num_taps number of values */
    tapCnt = num_taps - 1U;

    /* Copy data */
    while (tapCnt > 0U) {
        *pStateCurnt++ = *pState++;

        /* Decrement the loop counter */
        tapCnt--;
    }
}
#else
void fir_formii::process(const float *pSrc, float *pDst, uint blockSize) {
    float *pState = fir_state;  /* State pointer */
    float *pStateCurnt;         /* Points to the current sample of the state */
    float *px;                  /* Temporary pointer for state buffer */
    const float *pb;            /* Temporary pointer for coefficient buffer */
    float acc0;                 /* Accumulator */
    uint32_t i, tapCnt, blkCnt; /* Loop counters */

    /* fir_state points to state array which contains previous frame (num_taps - 1) samples */
    /* pStateCurnt points to the location where the new input data should be written */
    pStateCurnt = &(fir_state[(num_taps - 1U)]);

    /* Initialize blkCnt with number of taps */
    blkCnt = blockSize;

    while (blkCnt-- > 0U) {
        /* Copy one sample at a time into state buffer */
        *pStateCurnt++ = *pSrc++;

        /* Set the accumulator to zero */
        acc0 = 0.0f;

        /* Initialize state pointer */
        px = pState;

        /* Initialize Coefficient pointer */
        pb = taps;

        i = num_taps;

        /* Perform the multiply-accumulates */
        while (i-- > 0U) {
            /* acc =  b[num_taps-1] * x[n-num_taps-1] + b[num_taps-2] * x[n-num_taps-2] + b[num_taps-3] * x[n-num_taps-3]
             * +...+ b[0] * x[0] */
            acc0 += *px++ * *pb++;
        }

        /* Store result in destination buffer. */
        *pDst++ = acc0;

        /* Advance state pointer by 1 for the next sample */
        pState++;
    }

    /* Processing is complete.
       Now copy the last num_taps - 1 samples to the start of the state buffer.
       This prepares the state buffer for the next function call. */

    /* Points to the start of the state buffer */
    pStateCurnt = fir_state;

    /* Initialize tapCnt with number of taps */
    tapCnt = (num_taps - 1U);

    /* Copy remaining data */
    while (tapCnt-- > 0U)
        *pStateCurnt++ = *pState++;
}
#endif
