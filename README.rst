**********
FAT Python
**********

The FAT Python project was started by Victor Stinner in October 2015 to try to
solve issues of previous attempts of "static optimizers" for Python. The main
feature are efficient guards using versionned dictionaries to check if
something was modified. Guards are used to decide if the specialized bytecode
of a function can be used or not.

Python FAT is expected to be FAT... maybe FAST if we are lucky. FAT because
it will use two versions of some functions where one version is specialised to
specific argument types, a specific environment, optimized when builtins are
not mocked, etc.

FAT Python PEPs:

* PEP 509: `Add a private version to dict
  <https://www.python.org/dev/peps/pep-0509/>`_
* PEP 510: `Specialized functions with guards
  <https://www.python.org/dev/peps/pep-0510/>`_
* PEP 511: `API for AST transformers
  <https://www.python.org/dev/peps/pep-0511/>`_

Announcements and status reports:

* `'FAT' and fast: What's next for Python
  <http://www.infoworld.com/article/3020450/application-development/fat-fast-whats-next-for-python.html>`_:
  Article of InfoWorld by Serdar Yegulalp (January 11, 2016)
* [Python-Dev] `Third milestone of FAT Python
  <https://mail.python.org/pipermail/python-dev/2015-December/142397.html>`_
* `Status of the FAT Python project, November 26, 2015
  <https://haypo.github.io/fat-python-status-nov26-2015.html>`_
* [python-dev] `Second milestone of FAT Python
  <https://mail.python.org/pipermail/python-dev/2015-November/142113.html>`_
  (Nov 2015)
* [python-ideas] `Add specialized bytecode with guards to functions
  <https://mail.python.org/pipermail/python-ideas/2015-October/036908.html>`_
  (Oct 2015)

The project was created in October 2015.

Website: http://faster-cpython.readthedocs.org/fat_python.html
