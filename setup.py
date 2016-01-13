#!/usr/bin/env python

# Todo list to prepare a release:
#  - git pull --rebase
#  - downloaded latest Python 2.7 and 3.3 releases, patch them, install them
#  - run unit tests with Python 2.7 and 3.3
#  - update VERSION in _tracemalloc.c and setup.py
#  - reset option in setup.py: DEBUG=False
#  - set release date in the doc/changelog.rst file
#  - git commit -a
#  - git tag -a pytracemalloc-VERSION
#  - git push
#  - git push --tags
#  - python setup.py register sdist upload
#
# After the release:
#  - set version to n+1
#  - git commit
#  - git push

from __future__ import with_statement
from distutils.core import setup, Extension
import ctypes
import os
import subprocess
import sys

# Debug pytracemalloc
DEBUG = False

VERSION = '0.0'

CLASSIFIERS = [
    'Development Status :: 4 - Beta',
    'Intended Audience :: Developers',
    'License :: OSI Approved :: MIT License',
    'Natural Language :: English',
    'Operating System :: OS Independent',
    'Programming Language :: C',
    'Programming Language :: Python',
    'Topic :: Software Development :: Libraries :: Python Modules',
]

def main():
    pythonapi = ctypes.cdll.LoadLibrary(None)
    if not hasattr(pythonapi, 'PyFunction_Specialize'):
        print("WARNING: PyFunction_Specialize: missing, %s has not been patched" % sys.executable)
        print("Need Python 3.6 with the PEP 510")
    else:
        print("PyFunction_Specialize: present")

    cflags = []
    if not DEBUG:
        cflags.append('-DNDEBUG')

    with open('README.rst') as f:
        long_description = f.read().strip()

    ext = Extension('fat', ['fat.c'], extra_compile_args = cflags)

    options = {
        'name': 'fat',
        'version': VERSION,
        'license': 'MIT license',
        'description': 'Static optimizer specializing functions with guards',
        'long_description': long_description,
        "url": "http://faster-cpython.readthedocs.org/fat_python.html",
        'author': 'Victor Stinner',
        'author_email': 'victor.stinner@gmail.com',
        'ext_modules': [ext],
        'classifiers': CLASSIFIERS,
    }
    setup(**options)

if __name__ == "__main__":
    main()
