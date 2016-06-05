**********
fat module
**********

The ``fat`` module is a Python extension module (written in C) implementing
fast guards. The fatoptimizer optimizer uses ``fat`` guards to specialize
functions. ``fat`` guards are used to verify assumptions used to specialize the
code. If an assumption is no more true, the specialized code is not used.

The ``fat`` module is required to run code optimized by ``fatoptimizer`` if
at least one function is specialized.

* `fat documentation
  <https://fatoptimizer.readthedocs.io/en/latest/fat.html>`_
* `fat project at GitHub
  <https://github.com/haypo/fat>`_
* `fat project at the Python Cheeseshop (PyPI)
  <https://pypi.python.org/pypi/fat>`_
* `fatoptimizer documentation
  <https://fatoptimizer.readthedocs.io/>`_
* `FAT Python
  <https://faster-cpython.readthedocs.io/fat_python.html>`_

The ``fat`` module requires a Python 3.6 patched with PEP 510 patch.
