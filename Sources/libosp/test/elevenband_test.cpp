//
// Martin Hunt  03/23/2021
//
#include "gtest/gtest.h"

#include <iostream>
#include <OSP/multirate_filterbank/elevenband.hpp>
#include <math.h>
#include <time.h>
#include "matio.h"

// Expected data from matlab test/eleven_test_bands.mat 
// created by python/eleven_test_bands.ipynb

TEST(libosp, ElevenBand_impulse) {
    matvar_t *cell;
    mat_t *matfp = Mat_Open("test/eleven_test_bands.mat", MAT_ACC_RDONLY);
    ASSERT_FALSE(NULL == matfp);
    matvar_t *inp_var = Mat_VarRead(matfp,"inp_imp");
    // Mat_VarPrint(inp_var, 0);
    
    matvar_t *output_var = Mat_VarRead(matfp,"output_impulse");

    // create float array
    unsigned inp_size = inp_var->dims[1];
    float input[inp_size];
    double *imp = static_cast<double*>(inp_var->data);
    for (unsigned i=0; i < inp_size; i++)
            input[i] = imp[i];

    ElevenBand eb(inp_size, true, true);
    eb.create_bands(input, inp_size);

    for (int i=0; i < 11; i++) {
        cell = Mat_VarGetCell(output_var, i);
        unsigned length = (unsigned)(cell->dims[1]);
        double *output_800 = static_cast<double*>(cell->data);
        ASSERT_EQ(length, eb.dbandlen[i]);
        for (unsigned j=0; j < length; j++)
            ASSERT_NEAR(output_800[j], eb.dband[i][j], .00001) << "Band " << i << " Fail\n\n";
    }
    
    Mat_VarFree(output_var);
    Mat_VarFree(inp_var);
    Mat_Close(matfp);
}

TEST(libosp, ElevenBand_impulse_0) {
    matvar_t *cell;
    mat_t *matfp = Mat_Open("test/eleven_test_bands.mat", MAT_ACC_RDONLY);
    ASSERT_FALSE(NULL == matfp);
    matvar_t *inp_var = Mat_VarRead(matfp,"inp_imp");
    // Mat_VarPrint(inp_var, 0);
    
    matvar_t *output_var = Mat_VarRead(matfp,"output_impulse_0");

    // create float array
    unsigned inp_size = inp_var->dims[1];
    float input[inp_size];
    double *imp = static_cast<double*>(inp_var->data);
    for (unsigned i=0; i < inp_size; i++)
            input[i] = imp[i];

    ElevenBand eb(inp_size, false, true);
    eb.create_bands(input, inp_size);

    for (int i=0; i < 11; i++) {
        cell = Mat_VarGetCell(output_var, i);
        unsigned length = (unsigned)(cell->dims[1]);
        double *output_800 = static_cast<double*>(cell->data);
        ASSERT_EQ(length, eb.dbandlen[i]);
        for (unsigned j=0; j < length; j++)
            ASSERT_NEAR(output_800[j], eb.dband[i][j], .00001) << "Band " << i << " Fail\n\n";
    }
    
    Mat_VarFree(output_var);
    Mat_VarFree(inp_var);
    Mat_Close(matfp);
}


TEST(libosp, ElevenBand_800) {
    matvar_t *cell;
    mat_t *matfp = Mat_Open("test/eleven_test_bands.mat", MAT_ACC_RDONLY);
    ASSERT_FALSE(NULL == matfp);
    matvar_t *inp_var = Mat_VarRead(matfp,"inp800");
    // Mat_VarPrint(inp_var, 0);
    
    matvar_t *output_var = Mat_VarRead(matfp,"output_800");

    // create float array
    unsigned inp_size = inp_var->dims[1];
    float input[inp_size];
    double *imp = static_cast<double*>(inp_var->data);
    for (unsigned i=0; i < inp_size; i++)
            input[i] = imp[i];

    ElevenBand eb(inp_size, true, true);
    eb.create_bands(input, inp_size);

    for (int i=0; i < 11; i++) {
        cell = Mat_VarGetCell(output_var, i);
        unsigned length = (unsigned)(cell->dims[1]);
        double *output_800 = static_cast<double*>(cell->data);
        ASSERT_EQ(length, eb.dbandlen[i]);
        for (unsigned j=0; j < length; j++)
            ASSERT_NEAR(output_800[j], eb.dband[i][j], .00001) << "Band " << i << " Fail\n\n";
    }
    
    Mat_VarFree(output_var);
    Mat_VarFree(inp_var);
    Mat_Close(matfp);
}

TEST(libosp, ElevenBand_800_0) {
    matvar_t *cell;
    mat_t *matfp = Mat_Open("test/eleven_test_bands.mat", MAT_ACC_RDONLY);
    ASSERT_FALSE(NULL == matfp);
    matvar_t *inp_var = Mat_VarRead(matfp,"inp800");
    // Mat_VarPrint(inp_var, 0);
    
    matvar_t *output_var = Mat_VarRead(matfp,"output_800_0");

    // create float array
    unsigned inp_size = inp_var->dims[1];
    float input[inp_size];
    double *imp = static_cast<double*>(inp_var->data);
    for (unsigned i=0; i < inp_size; i++)
            input[i] = imp[i];

    ElevenBand eb(inp_size, false, true);
    eb.create_bands(input, inp_size);

    for (int i=0; i < 11; i++) {
        cell = Mat_VarGetCell(output_var, i);
        unsigned length = (unsigned)(cell->dims[1]);
        double *output_800 = static_cast<double*>(cell->data);
        ASSERT_EQ(length, eb.dbandlen[i]);
        for (unsigned j=0; j < length; j++)
            ASSERT_NEAR(output_800[j], eb.dband[i][j], .00001) << "Band " << i << " Fail\n\n";
    }
    
    Mat_VarFree(output_var);
    Mat_VarFree(inp_var);
    Mat_Close(matfp);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
