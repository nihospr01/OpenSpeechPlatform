import os
from setuptools import find_packages
from distutils.core import setup, Extension
from Cython.Build import cythonize, build_ext
import numpy
incdirs = numpy.get_include()
	
    
DEBUG = os.environ.get('DEBUG')
os.environ["LSAN_OPTIONS"] = "exitcode=0"

def find_pyx(path='.'):
    pyx_files = []
    for root, dirs, filenames in os.walk(path):
        for fname in filenames:
            if fname.endswith('.pyx'):
                pyx_files.append(os.path.join(root, fname))
    return pyx_files

if DEBUG:
    print("DEBUG PYTHON BUILD. use with LD_PRELOAD=libasan.so")
    extra_compile_args = ['-g', '-O0', '-Wno-reorder'
            '-fsanitize=address', 
            '-fno-sanitize=vptr', "-static-libasan",
            '-fno-omit-frame-pointer']
    extra_link_args = ['-g', "-fsanitize=address", "-static-libasan"]

else:
    extra_compile_args = ['-Ofast', '-march=native', '-DNDEBUG', '-Wno-reorder']
    extra_link_args = ['-DNDEBUG']

compiler_directives = {"language_level": 3,
                       "embedsignature": True, "binding": True}
extensions = [
    Extension(name="rtmha.freping", 
              sources=["rtmha/freping.pyx",
                       "OSP/freping/freping.cpp",
                       "OSP/array_utilities/array_utilities.cpp"
                      ],
              extra_compile_args=extra_compile_args,
              extra_link_args=extra_link_args,                     
             ),
    Extension(name="rtmha.filter", 
              sources=["rtmha/filter.pyx",
                       "OSP/filter/fir_formii.cpp",
                       "OSP/filter/filter.cpp",
                       "OSP/adaptive_filter/adaptive_filter.cpp",
                        "OSP/circular_buffer/circular_buffer.cpp",
                       "OSP/array_utilities/array_utilities.cpp"
                      ],
              extra_compile_args=extra_compile_args,
              extra_link_args=extra_link_args,                     
             ),
    Extension(name="rtmha.resample", 
              sources=["rtmha/resample.pyx",
                       "OSP/resample/resample.cpp",
                       "OSP/filter/fir_formii.cpp", 
                       "OSP/resample/resample_down.cpp",
                       "OSP/resample/resample_up.cpp",
                       "OSP/circular_buffer/circular_buffer.cpp",
                       "OSP/array_utilities/array_utilities.cpp"
                      ],
              extra_compile_args=extra_compile_args,
              extra_link_args=extra_link_args,                     
             ),
    Extension(name="rtmha.elevenband",
              sources=[
               "OSP/filter/fir_formii.cpp",
               "OSP/resample/resample.cpp",
               "OSP/resample/resample_down.cpp",
               "OSP/resample/resample_up.cpp",
               "OSP/circular_buffer/circular_buffer.cpp",
               "OSP/array_utilities/array_utilities.cpp"
               ],
              extra_compile_args=extra_compile_args,
              extra_link_args=extra_link_args,
              ),
]
extensions = cythonize(extensions,
                       compiler_directives=compiler_directives)

setup(
    ext_modules=extensions,
    include_dirs=[incdirs, "."]
)