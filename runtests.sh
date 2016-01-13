#!/bin/sh
set -e -x

PYTHON=~/prog/python/fatpython/python
$PYTHON setup.py build
PYTHONPATH=build/lib.linux-x86_64-3.6-pydebug/ $PYTHON test_fat.py
