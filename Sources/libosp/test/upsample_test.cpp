
//======================================================================================================================
/** @file upsample_test.cpp
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

#include "gtest/gtest.h"

#include "matio.h"
#include <OSP/multirate_filterbank/elevenband.hpp>
#include <iostream>
#include <math.h>
#include <time.h>

// Expected data from matlab test/eleven_test_upsample.mat
// created by python/eleven_test_upsample.ipynb

TEST(libosp, Upsample400_0_0) {
    mat_t *matfp = Mat_Open("test/eleven_test_upsample.mat", MAT_ACC_RDONLY);
    ASSERT_FALSE(NULL == matfp);
    matvar_t *inp_var = Mat_VarRead(matfp, "inp400");
    Mat_VarPrint(inp_var, 0);

    matvar_t *output_var = Mat_VarRead(matfp, "out400_0_0");
    Mat_VarPrint(output_var, 0);


    // create float array
    unsigned inp_size = inp_var->dims[1];
    unsigned out_size = output_var->dims[1];
    ASSERT_EQ(inp_size, out_size);
    float input[inp_size];
    double *imp = static_cast<double*>(inp_var->data);
    double *out = static_cast<double*>(output_var->data);
    for (unsigned i=0; i < inp_size; i++)
            input[i] = imp[i];

    ElevenBand eb(inp_size, false, false);
    eb.create_bands(input, inp_size);
    eb.upsample();

    ASSERT_EQ(out_size, eb.bandlen);
    // for (unsigned j=0; j < out_size; j++)
        // ASSERT_NEAR(out[j], eb.band[i][j], .00001);

    for (unsigned j=0; j < 32; j++){
        float sum=0.0;
        for (unsigned i=0; i < 11; i++)
            sum += eb.band[i][j];
        printf("%f\t%f\n", out[j], sum);
    }
        

    Mat_VarFree(output_var);
    Mat_VarFree(inp_var);
    Mat_Close(matfp);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
