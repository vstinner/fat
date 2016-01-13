**********
FAT Python
**********

FAT Python
==========

FAT Python is a static optimizer for Python 3.6 using function specialization
with guards.

The ``fat`` module is the runtime part of the optimizer. It is required to run
optimized code. The optimizer is the ``astoptimizer`` module.

* FAT Python: http://faster-cpython.readthedocs.org/fat_python.html
* fat module: https://github.com/haypo/fat
* astoptimizer module: https://hg.python.org/sandbox/fatpython/


Installation
============

Type::

    pip install fat

Manual installation::

    python3.6 setup.py install


Run tests
=========

Type::

    ./runtests.sh


Changelog
=========

* 2016-01-13: First public release.
