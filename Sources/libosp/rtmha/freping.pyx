# distutils: language=c++
import numpy as np
cimport numpy as np
cimport cython
from libcpp cimport bool

cdef extern from "OSP/freping/freping.hpp":
    cdef cppclass freping:
        freping(int allpass_chain_len,int frame_size, float alpha, int freping_on_off)
        void get_params(float &alpha, int &freping_on_off)
        void set_params(float alpha, int freping_on_off)
        void freping_proc(float* inp, float* out)

# @cython.embedsignature(True)
cdef class Freping:
    """Freping class.  

        This freping class provides frequency warping feature in real-time. Please refer to the following paper for details.
        Ching-Hua Lee, Kuan-Lin Chen, fred harris, Bhaskar D. Rao, and Harinath Garudadri,
        "On mitigating acoustic feedback in hearing aids with frequency warping by all-pass networks,"
        in Annual Conference of the International Speech Communication Association (Interspeech), 2019.

        Parameters
        ----------
        window_length : int
            The length of the Hamming window to use. Must be multiple of 2*frame_size
        frame_size : int
            The frame size of the caller (the real-time system)
        alpha : float
            The parameter to control the degree of frequency warping (1.0>=alpha>=-1.0)
        enable : int
            Enable freping.
    """
    def __init__(self, window_length, frame_size, alpha=0, enable=0):
        pass

    # frep is a pointer that will hold to the instance of the C++ class
    cdef freping *frep

    def __cinit__(self, int window_length, int frame_size, float alpha, int enable):
        self.frep = NULL
        self.frep = new freping(window_length, frame_size, alpha, enable)

    def __dealloc__(self):
        if self.frep != NULL:
            del self.frep

    cpdef proc(self, np.ndarray inp):
        """Get the output signal of freping, the output signal is warped according to the alpha parameter

        Parameters
        ----------
        inp : array_like, 1-D
            The input signal as a vector. Length must be `frame_size` (from constructor)

        Returns a 1-D array output signal. 
        """
        if inp.dtype != np.float32:
            inp = inp.astype(np.float32)
        cdef np.ndarray out = np.empty(inp.size, np.float32)
        self.frep.freping_proc(<float *>inp.data, <float *>out.data)
        return out

    cpdef get(self):
        "Returns a tuple containing the alpha and enable value."
        cdef float alpha
        cdef int enable
        self.frep.get_params(alpha, enable)
        return  alpha, True if enable else False

    cpdef set(self, float alpha,  bool enable):
        "Set Freping alpha and enable/disable"
        self.frep.set_params(alpha, 1 if enable else 0)
