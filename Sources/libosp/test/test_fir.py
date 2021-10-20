#!/usr/bin/env python3

import sys
# find rthma from toplevel or test directory
sys.path.append('..')
sys.path.append('.')

import pytest
from rtmha.filter import FirFilter
from rtmha.elevenband import elevenband_taps_min
import numpy as np
from scipy.signal import lfilter

def test_impulse():
    inp = np.zeros(128).astype('float32')
    inp[0] = 1.0
    f = FirFilter(elevenband_taps_min[0], 128)
    res = f.filter(inp)

    # compare with scipy lfilter
    out = lfilter(elevenband_taps_min[0], 1.0, inp)

    assert(np.allclose(out, res))

def test_impulse_2():
    inp = np.zeros(128).astype('float32')
    inp[0] = 1.0
    inp[48] = 1.0
    inp[100] = 1.0
    f = FirFilter(elevenband_taps_min[0], 128)
    res = f.filter(inp)

    # compare with scipy lfilter
    out = lfilter(elevenband_taps_min[0], 1.0, inp)

    assert(np.allclose(out, res))

def test_impulse_frame():
    inp = np.zeros(128).astype('float32')
    inp[0] = 1.0
    inp[48] = 1.0
    inp[100] = 1.0
    f = FirFilter(elevenband_taps_min[0], 128)

    # break into 32-byte frames
    res = np.empty((0),dtype=np.float32)
    for start in range(0,len(inp), 32):
        print(start)
        res = np.append(res, f.filter(inp[start:start+32]))    

    # compare with scipy lfilter
    out = lfilter(elevenband_taps_min[0], 1.0, inp)

    assert(np.allclose(out, res))

test_impulse_frame()