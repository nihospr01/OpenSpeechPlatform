#!/usr/bin/env python3

import sys
# find rthma from toplevel or test directory
sys.path.append('..')
sys.path.append('.')

import pytest
from rtmha.elevenband import Wdrc11
import numpy as np

# def test_wdrc11_1():
#     "Checks values of interval variables computed at WDRC initialization"
#     inp = np.zeros(128).astype('float32')
#     inp[0] = 1.0

#     g50 = np.array([ 30, 20, 25,2,15,20,0,0,0,0,0], np.float32)
#     g80 = np.array([ 0,-10,-5, 0,0,0,0,0,-.1,-1,-10], np.float32)
#     kneelow = np.ones(11) * 45
#     band_mpo = np.ones(11) * 120
#     AT = np.ones(11) * 10
#     RT = np.ones(11) * 100
#     min_phase=1
#     align=1

#     w = Wdrc11(g50, g80, kneelow, band_mpo, AT, RT, len(inp), min_phase, align)
#     m, b, c, kneeup, alpha1, alpha2 = w.get_param_test()

#     assert np.allclose(m, np.array(
#         [
#             0.0,
#             0.0,
#             0.0,
#             0.9333333969116211,
#             0.5,
#             0.3333333432674408,
#             1.0,
#             1.0,
#             0.9966667294502258,
#             0.9666666984558105,
#             0.6666666865348816
#         ]
#     ))

#     assert np.allclose(b, np.array([80.0,
#         70.0,
#         75.0,
#         5.333330154418945,
#         40.0,
#         53.33333206176758,
#         0.0,
#         0.0,
#         0.1666635274887085,
#         1.6666650772094727,
#         16.66666603088379]))

#     assert np.allclose(c, np.array([35.0,
#         25.0,
#         30.0,
#         2.3333330154418945,
#         17.5,
#         23.33333396911621,
#         0.0,
#         0.0,
#         0.01666635274887085,
#         0.16666650772094727,
#         1.6666669845581055]), atol=.0001)

#     assert np.allclose(kneeup, np.array([
#         200,  # matlab computes this as "inf"
#         200,  # matlab computes this as "inf"
#         200,  # matlab computes this as "inf"
#         122.85715, 
#         160., 
#         200.00002,
#         120., 
#         120.     , 
#         120.23411, 
#         122.41379, 
#         155.]))

#     assert np.allclose(alpha1, np.array([0.11559200286865234,
#         0.11559200286865234,
#         0.11559200286865234,
#         0.059757400304079056,
#         0.043131887912750244,
#         0.025314927101135254,
#         0.029878700152039528,
#         0.014939350076019764,
#         0.014939350076019764,
#         0.007469675038009882,
#         0.0042351484298706055]))

#     assert np.allclose(alpha2, np.array([0.010786652565002441,
#         0.010786652565002441,
#         0.010786652565002441,
#         0.005177592393010855,
#         0.0036829710006713867,
#         0.0022020339965820312,
#         0.0025887961965054274,
#         0.0012943980982527137,
#         0.0012943980982527137,
#         0.0006471990491263568,
#         0.0003344416618347168]))


# def test_wdrc11_2():
#     " Checks values of interval variables computed at WDRC initialization"
#     inp = np.zeros(128).astype('float32')
#     inp[0] = 1.0
    
#     g50 = np.array([ 5, -0.001, 0, 0,0,0,0,0,0,0,0], np.float32)
#     g80 = np.array([ 10,0,-0.001, 0,0,0,0,0,0,0,0], np.float32)
#     kneelow = np.ones(11) * 45
#     band_mpo = np.ones(11) * 120
#     AT = np.ones(11) * 10
#     RT = np.ones(11) * 100
#     min_phase=1
#     align=1

#     w = Wdrc11(g50, g80, kneelow, band_mpo, AT, RT, len(inp), min_phase, align)
#     m, b, c, kneeup, alpha1, alpha2 = w.get_param_test()

#     assert np.allclose(m, np.array([1.1666666, 1.0000333, 0.9999667, 1.       , 1.       , 1.       ,
#         1.       , 1.       , 1.       , 1.       , 1.       ]))

#     assert np.allclose(b, np.array([-3.3333321e+00, -2.6626587e-03,  1.6670227e-03,  0.0000000e+00,
#          0.0000000e+00,  0.0000000e+00,  0.0000000e+00,  0.0000000e+00,
#          0.0000000e+00,  0.0000000e+00,  0.0000000e+00]), atol=.00001)

#     assert np.allclose(c, np.array([ 4.1666679e+00, -1.1672974e-03,  1.6784668e-04,  0.0000000e+00,
#          0.0000000e+00,  0.0000000e+00,  0.0000000e+00,  0.0000000e+00,
#          0.0000000e+00,  0.0000000e+00,  0.0000000e+00]), atol=.00001)

#     assert np.allclose(kneeup, np.array([105.71429 , 119.99867 , 120.002335, 120.      , 120.      ,
#         120.      , 120.      , 120.      , 120.      , 120.      ,
#         120.      ]), atol=.00001)

#     assert np.allclose(alpha1, np.array([
#         0.1195148,  # matlab puts a complex value here
#         0.1195148,
#         0.1195148, 0.0597574,
#         0.0597574, 0.0298787,
#         0.0298787, 0.01493935,
#         0.01493935, 0.00746968,
#         0.00746968]), atol=.00001)

#     assert np.allclose(alpha2, np.array([
#         0.01035518,  # matlab puts a complex value here
#         0.01035518,
#         0.01035518, 0.00517759,
#         0.00517759, 0.0025888,
#         0.0025888, 0.0012944,
#         0.0012944, 0.0006472,
#         0.0006472]), atol=.00001)

# test_wdrc11_2()
