#!/bin/sh
set -e -x

rm -rf build/
PYTHON=~/prog/python/fatpython/python
$PYTHON setup.py build
PYTHONPATH=$(ls -d build/lib.linux-x86_64-3.6*/) $PYTHON test_fat.py
