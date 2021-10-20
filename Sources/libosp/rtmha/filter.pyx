# distutils: language=c++
import numpy as np
cimport numpy as np
cimport cython

# Numpy must be initialized. When using numpy from C or Cython you must
# _always_ do that, or you will have segfaults
np.import_array()
cdef extern from "numpy/arrayobject.h":
    void PyArray_ENABLEFLAGS(np.ndarray arr, int flags)

ctypedef unsigned int uint

cdef extern from "OSP/adaptive_filter/adaptive_filter.hpp":
    cdef cppclass adaptive_filter:
        adaptive_filter(float *adaptive_filter_taps, uint num_taps, uint max_frame_size,
                       int adaptation_type, float mu, float delta, float rho, float alpha, float beta, float p,
                       float c, float power_estimate)
        int update_taps(float *u_ref, float *e_ref, uint ref_size)
        uint get_max_frame_size()
        void get_params(float &mu, float &rho, float &delta, float &alpha, float &beta, float &p, float &c, int &adaptation_type)
        void set_params(float mu, float rho, float delta, float alpha, float beta, float p, float c, int adaptation_type)
        # inherited from filter class
        int set_taps(float *taps, uint num_taps)
        int get_taps(float *taps, uint num_taps)
        void cirfir(float *data_in, float *data_out, uint num_samp)

cdef class AdaptiveFilter:
    """ Adaptive filter constructor

        This adaptive filter class implements several popular LMS-based algorithms including Modified LMS
        [Greenberg, 1998], IPNLMS-l_0 [Paleologu et al., 2010] and SLMS [Lee et al., 2017].

        Parameters
        ----------
        taps : array, 1-D
            The coefficients for the taps.
        max_frame_size : int
            The maximum frame size the object can process.
        adaptation_type : int
            0: stop adaptation, 1: Modified LMS, 2: IPNLMS-l_0, 3: SLMS
        mu : float
            The gradient descent step size (learning rate) for LMS-based algorithms
        delta : float
            A small positive number to prevent dividing zero
        rho : float
            The forgetting factor for power estimate
        alpha : float
            A number between -1 to 1 for different degrees of sparsity in IPNLMS-l_0
        beta : float
            A number between 0 to 500 for different degrees of sparsity in IPNLMS-l_0
        p : float
            A number between 0 to 2 for fitting different degrees of sparsity in SLMS
        c : float
            A small positive number for preventing stagnation in SLMS
        power_estimate : float
            An initial power estimate for adaptation
    """
    def __init__(self, taps, max_frame_size, adaptation_type, mu, delta, rho, alpha, beta, p, c, power_estimate):
        pass

    # _thisptr is a pointer that will hold to the instance of the C++ class
    cdef adaptive_filter *afilt

    def __cinit__(self, np.ndarray taps, int max_frame_size, int adaptation_type, float mu, float delta, float rho, float alpha, 
                    float beta, float p, float c, float power_estimate):
        if taps.dtype != np.float32:
            taps = taps.astype(np.float32)
        self.afilt = new adaptive_filter(<float *>taps.data, len(taps), max_frame_size, adaptation_type, mu, delta, rho, alpha, beta, p, c, power_estimate)
        self.num_taps = len(taps)
        if self.afilt == NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self.afilt != NULL:
            del self.afilt

    cpdef update_taps(self, np.ndarray u_ref, np.ndarray e_ref):
        """
        Updates the taps of this adaptive filter based on the reference signals u_ref and e_ref

        Parameters
        ----------
        u_ref : array_like, 1-D
            A reference input signal for adaptation
        e_ref : array_like, 1-D
            A reference error signal for adaptation
        """
        if u_ref.dtype != np.float32:
            u_ref = u_ref.astype(np.float32)
        if e_ref.dtype != np.float32:
            e_ref = e_ref.astype(np.float32)
        self.afilt.update_taps(<float *>u_ref.data, <float *>e_ref.data, u_ref.size)

    cpdef set_taps(self, np.ndarray taps):
        """
        Set the filter taps

        Parameters
        ----------
        taps : 1D numpy array
            The filter taps
        """
        if taps.dtype != np.float32:
            taps = taps.astype(np.float32)
        self.afilt.set_taps(<float *>taps.data, taps.size)

    cpdef get_taps(self):
        """
        Get the filter taps

        Returns a 1D array of taps (filter coefficients)
        """
        cdef np.ndarray taps = np.empty(self.num_taps, np.float32)
        self.afilt.get_taps(<float *>taps.data, taps.size)
        return taps

    cpdef filter(self, np.ndarray inp):
        """Applies the filter to an input

        Parameters
        ----------
        inp : array_like, 1-D
            The input signal as a vector.

        Returns a 1-D array output signal of the filtered data. 
        """
        if inp.dtype != np.float32:
            inp = inp.astype(np.float32)
        cdef np.ndarray out = np.empty_like(inp)
        self.afilt.cirfir(<float *>inp.data, <float *>out.data, len(inp))
        return out


cdef extern from "OSP/filter/fir_formii.h":
    cdef cppclass fir_formii:
        fir_formii(float *taps, unsigned num_taps, unsigned max_frame_size)
        void process(float *data_in, float *data_out, unsigned num_samp)

# @cython.embedsignature(True)
cdef class FirFilter:
    """Creates a filter.  

        Taps are requires for filter creation, but may be
        dynamically modified later.

        Parameters
        ----------
        taps : array, 1-D
              The coefficients for the taps.  
    """
    def __init__(self, taps, max_frame_size):
        pass

    # _thisptr is a pointer that will hold to the instance of the C++ class
    cdef fir_formii *_thisptr

    def __cinit__(self, np.ndarray taps, max_frame_size):
        if taps.dtype != np.float32:
            taps = taps.astype(np.float32)
        self._thisptr = new fir_formii(<float *>taps.data, len(taps), max_frame_size)
        if self._thisptr == NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self._thisptr != NULL:
            del self._thisptr

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cpdef filter(self, np.ndarray inp):
        """Applies the filter to an input

        Parameters
        ----------
        inp : array_like, 1-D
            The input signal as a vector.

        Returns a 1-D array output signal of the filtered data. 
        """
        if inp.dtype != np.float32:
            inp = inp.astype(np.float32)
        cdef np.ndarray out = np.empty_like(inp)
        self._thisptr.process(<float *>inp.data, <float *>out.data, len(inp))
        return out


cdef extern from "OSP/filter/hilbert.hpp":
    cdef cppclass CHilbert "Hilbert":
        CHilbert()
        # void filter(float *inp, float *out_real, float *out_imag, int length)
        void amplitude(float *inp, float *out, int length)
        void reset()

cdef class Hilbert:
    """Creates a Hilbert filter
    
    This is a Hilbert filter tuned specifically for the 11-Band Multirate filterbank.
    """
    def __init__(self):
        pass

    cdef CHilbert *hilb
    def __cinit__(self):
        self.hilb = new CHilbert()

    # cpdef filter(self, np.ndarray inp):
    #     """Applies the filter to an input

    #     Parameters
    #     ----------
    #     inp : array_like, 1-D
    #         The input signal as a vector. Float32 dtype is most efficient.

    #     Returns a 1-D complex array output signal of the filtered data. 
    #     """
    #     if inp.dtype != np.float32:
    #         inp = inp.astype(np.float32)
    #     cdef np.ndarray out = np.empty(len(inp), dtype=np.complex64)
    #     cdef np.ndarray real = np.empty(len(inp), dtype=np.float32)
    #     cdef np.ndarray imag = np.empty(len(inp), dtype=np.float32)
    #     self.hilb.filter(<float *>inp.data, <float *>real.data, <float *>imag.data, len(inp))
    #     out = real + 1j * imag
    #     return out

    cpdef amplitude(self, np.ndarray inp):
        """Applies the filter to an input

        This is equivalent to calling np.abs() on the complex output
        of filter.  If you don't need the complex analytic signal and
        just need the envelope, this is much more efficient.
        
        Parameters
        ----------
        inp : array_like, 1-D
            The input signal as a vector. Float32 dtype is most efficient.

        Returns a 1-D float32 array output of the signal amplitude. 
        """
        if inp.dtype != np.float32:
            inp = inp.astype(np.float32)
        cdef np.ndarray out = np.empty(len(inp), dtype=np.float32)
        self.hilb.amplitude(<float *>inp.data, <float *>out.data, len(inp))
        return out

    cpdef reset(self):
        """Resets the Hilbert Filter

        The Hilbert filter maintains some state (delay blocks) from call 
        to call to allow for splitting a long signal into chunks.  
        Calling reset() clears the state.
        """
        self.hilb.reset()
