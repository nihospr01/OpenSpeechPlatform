#include "gtest/gtest.h"
#include <OSP/array_utilities/array_utilities.hpp>
#include <cassert>
#include <iostream>
#include <string>

using namespace std;

TEST(libosp, ArraySum) {
    // create test array
    int num = 128;
    float *arr1 = new float[num];
    for (auto i = 0; i < num; i++)
        arr1[i] = (float)(i + 1);

    ASSERT_EQ(array_sum(arr1, 32), 528.0);
    ASSERT_EQ(array_sum(arr1, 48), 1176.0);
    ASSERT_EQ(array_sum(arr1, 45), 1035.0);
    ASSERT_EQ(array_sum(arr1, 46), 1081.0);
    ASSERT_EQ(array_sum(arr1, 47), 1128.0);
    delete[] arr1;
}

TEST(libosp, ArrayMultiplyConst32) {
    int num = 32;
    float *arr1 = new float[num]();
    float *arr2 = new float[num]();
    for (auto i = 0; i < num; i++) {
        arr1[i] = (float)(i + 1);
        arr2[i] = (float)(i + 1);
    }

    array_multiply_const(arr1, 1, num);
    for (auto i = 0; i < num; i++)
        ASSERT_EQ(arr1[i], arr2[i]);

    array_multiply_const(arr1, 2, num);
    for (auto i = 0; i < num; i++)
        ASSERT_EQ(arr1[i], arr2[i] * 2);

    array_multiply_const(arr1, 0, num);
    for (auto i = 0; i < num; i++)
        ASSERT_EQ(arr1[i], 0);
    delete[] arr1;
    delete[] arr2;
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
