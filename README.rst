**********
FAT Python
**********

FAT Python
==========

FAT Python is a static optimizer for Python 3.6 using function specialization
with guards.

The ``fat`` module is the runtime part of the optimizer. It is required to run
optimized code.

The optimizer is the ``astoptimizer`` module: `fatpython Mercurial repository
at python.org <https://hg.python.org/sandbox/fatpython/>`_.

Website: http://faster-cpython.readthedocs.org/fat_python.html


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
