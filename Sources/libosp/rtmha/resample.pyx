# distutils: language=c++
import numpy as np
cimport numpy as np
cimport cython

np.import_array()
cdef extern from "numpy/arrayobject.h":
    void PyArray_ENABLEFLAGS(np.ndarray arr, int flags)


cdef extern from "OSP/resample/resample.hpp":
    cdef cppclass resample:
        resample(float *filter_taps, unsigned num_taps, unsigned max_frame_size, unsigned interp_factor, unsigned deci_factor)
        void resamp(float *data_in, float *data_out, unsigned in_len)

# @cython.embedsignature(True)
cdef class Resample:
    """Resampling class.

        Implements L/M-fold resampling

        Parameters
        ----------
        taps : array, 1-D
                The filter taps of the lowpass filter (to reject images and prevent aliasing)
        max_frame_size : int
            The maximum input frame size
        interp_factor : int
            The interpolation factor L (to implement L-fold expander)
        deci_factor : int
            The decimation factor M (to implement M-fold decimator)
    """
    def __init__(self, taps, max_frame_size, interp_factor, deci_factor):
        pass

    # _thisptr is a pointer that will hold to the instance of the C++ class
    cdef resample *_thisptr
    cdef factor

    def __cinit__(self, np.ndarray taps, int max_frame_size, int interp_factor, int deci_factor):
        if taps.dtype != np.float32:
            taps = taps.astype(np.float32)
        self._thisptr = new resample(<float *>taps.data, taps.size, max_frame_size, interp_factor, deci_factor)
        if self._thisptr == NULL:
            raise MemoryError()
        self.factor = interp_factor / deci_factor

    def __dealloc__(self):
        if self._thisptr != NULL:
            del self._thisptr


    cpdef resamp(self, np.ndarray inp):
        """Resamples an input

        Parameters
        ----------
        inp : array_like, 1-D
            The input signal as a vector.

        Returns a 1-D array output signal of the resampled data.
        """
        if inp.dtype != np.float32:
            inp = inp.astype(np.float32)
        cdef np.ndarray out = np.empty(int(self.factor * inp.size), np.float32)
        cdef unsigned out_len=0
        self._thisptr.resamp(<float *>inp.data, <float *>out.data, inp.size)
        return out

cdef extern from "OSP/resample/32_48_filter.h":
    cdef float filter_32_48[25]
cdef extern from "OSP/resample/48_32_filter.h":
    cdef float filter_48_32[25]

filter32_48 = np.PyArray_SimpleNewFromData(1, [25], np.NPY_FLOAT32, filter_32_48)
filter48_32 = np.PyArray_SimpleNewFromData(1, [25], np.NPY_FLOAT32, filter_48_32)

