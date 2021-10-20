# Default target executed when no arguments are given to make.
default_target: release 

.PHONY : release clean docs install debug test python python_debug

PREFIX ?= 1
ifeq ($(PREFIX), 1)
	RELEASE_DIR = $(shell realpath ..)/release
	DEBUG_DIR = $(shell realpath ..)/debug
else
	RELEASE_DIR = $(PREFIX)
	DEBUG_DIR = $(PREFIX)
endif


release:
	mkdir -p release
	cd release && cmake -DCMAKE_INSTALL_PREFIX=$(RELEASE_DIR) ..
	$(MAKE) -C release install
	
test:	python
	mkdir -p release
	cd release && cmake -DCMAKE_INSTALL_PREFIX=$(RELEASE_DIR) -DBUILD_TESTING=1 ..
	$(MAKE) -C release
	pytest
    
bench:
	mkdir -p release
	cd release && cmake -DCMAKE_INSTALL_PREFIX=$(RELEASE_DIR) -DBUILD_TESTING=1 ..
	$(MAKE) -C release
	./release/test/array_bench
	
debug: python
	mkdir -p debug
	cd debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$(DEBUG_DIR) -DBUILD_TESTING=1 ..
	$(MAKE) -C debug
	pytest

python: setup.py
	python3 setup.py build_ext --inplace

# Use with LD_PRELOAD=libasan.so
python_debug: clean
	DEBUG=1 python setup.py build_ext --inplace

docs:
	$(MAKE) -C docs

clean:
	$(MAKE) -C docs clean
	$(RM) -rf release debug
	
	# python
	$(RM) -r build dist
	$(RM) -r .pytest_cache
	find . -name __pycache__ -exec rm -r {} +
	find rtmha -name \*.cpp -exec rm -r {} +
	find rtmha -name \*.so -exec rm -r {} +
