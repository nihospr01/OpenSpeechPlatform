# Jupyter OSP Notebooks

The notebooks in this directory are divided into three classes:

1. [Notebooks that control (or provide a GUI for) a running OSP Process](osp/README.ipynb)
2. [Notebooks used for development or testing](test) (Not recommended for normal users)
3. Notebooks that use the RTMHA python bindings.  These are located in the current directory and are described below.

## What are the RTMHA Python bindings?

These allow developers to use python to create and interact with the internal RTMHA algorithms.  For example, you can use FIR filters, resample a signal, or use the 11-band multirate filterbank or WDRC to process an audio signal.

## Why Use Them?

1. Education.  Learn about the RTMHA algorithms and create demonstrations of them working.
2. Testing.  Verify the underlying C++ code is working properly.

## How Do You Get The RTMHA Python Bndings?

We do not yet distribute these through PyPI.  Instead you must build the python bindings from the libosp directory.  The Cython package is required.  After you install it, simply type "make python"

```
libosp> make python
python3 setup.py build_ext --inplace
Compiling rtmha/freping.pyx because it changed.
Compiling rtmha/filter.pyx because it changed.
Compiling rtmha/resample.pyx because it changed.
Compiling rtmha/elevenband.pyx because it changed.
[1/4] Cythonizing rtmha/elevenband.pyx
...
```

Cython generates C++ that is compiled into four python modules:

```
> ls -l rtmha/*.so
-rwxrwxr-x 1 mmh mmh 15834664 Jul  6 16:49 rtmha/elevenband.cpython-38-x86_64-linux-gnu.so
-rwxrwxr-x 1 mmh mmh  1442040 Jul  6 16:49 rtmha/filter.cpython-38-x86_64-linux-gnu.so
-rwxrwxr-x 1 mmh mmh   780728 Jul  6 16:49 rtmha/freping.cpython-38-x86_64-linux-gnu.so
-rwxrwxr-x 1 mmh mmh   676800 Jul  6 16:49 rtmha/resample.cpython-38-x86_64-linux-gnu.so
```


You can load these modules into ipython or Jupyter.  The *help()* command will show docs.

```
In [9]: import rtmha.freping
In [10]: help(rtmha.freping)
```

Docs are still in development and may not be complete.  The Jupyter notebooks in this directroy may be helpful examples.
