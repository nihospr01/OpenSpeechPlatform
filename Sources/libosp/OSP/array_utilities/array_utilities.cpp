//======================================================================================================================
/** @file array_utilities.cpp
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

#include <OSP/array_utilities/array_utilities.hpp>
#include <cstdio>

/**
 * @brief Add all the numbers in an array
 *
 * @param arr input array
 * @param len sizeof array
 * @return float Sum of the array
 */

float array_sum(const float *arr, unsigned len) {
    float total = 0.0;
    for (unsigned i = 0; i < len; i++)
        total += arr[i];
    return total;
}

float array_dot_product(const float *in1, const float *in2, unsigned len) {
    float res = 0;
    for (unsigned i = 0; i < len; i++)
        res += (in1[i] * in2[i]);
    return res;
}

void array_multiply_const(float *arr, float constant, unsigned len) {
    for (unsigned i = 0; i < len; i++)
        arr[i] *= constant;
}

void array_add_const(float *arr, float constant, unsigned len) {
    for (unsigned i = 0; i < len; i++)
        arr[i] += constant;
}

void array_add_array(float *in1, const float *in2, unsigned len) {
    for (unsigned i = 0; i < len; i++)
        in1[i] += in2[i];
}

void array_element_multiply_array(float *in1, const float *in2, unsigned len) {
    for (unsigned i = 0; i < len; i++)
        in1[i] = in1[i] * in2[i];
}

float array_mean(float *arr, unsigned len) { return array_sum(arr, len) / len; }

void array_square(const float *in, float *out, unsigned len) {
    for (unsigned i = 0; i < len; i++)
        out[i] = in[i] * in[i];
}

float array_mean_square(const float *arr, unsigned len) { return array_dot_product(arr, arr, len) / len; }

void array_print(const char *str, const float *arr, unsigned len) {
    printf("\n%s\n", str);
    for (unsigned i = 0; i < len; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%.5e ", arr[i]);
    }
    printf("\n");
}

#if 0 // UNUSED

void array_flip(float *arr, unsigned len) {
    float temp;
    unsigned start = 0;
    unsigned end = len - 1;
    while (start < end)
    {
        temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;
        start++;
        end--;
    }
}

void array_element_divide_array(float *in1, const float *in2, unsigned len) {
    unsigned i;

    for (i = 0; i < len; i++) {
        in1[i] = in1[i] / in2[i];
    }
}

float array_min(const float *arr, unsigned len) {
    float min;
    unsigned i;

    min = arr[0];
    for (i = 1; i < len; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }

    return min;
}

void array_subtract_array(float *in1, const float *in2, unsigned len) {
    unsigned i;

    for (i = 0; i < len; i++) {
        in1[i] = in1[i] - in2[i];
    }
}

void array_right_shift(float *arr, unsigned len) {
    unsigned i;
    for(i = len-1; i > 0; i--) {
        arr[i] = arr[i-1];
    }
    arr[0] = 0;
}
#endif
