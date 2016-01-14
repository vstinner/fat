TODO
====

* Rename fat.GuardGlobals and fat.GuardTypeDict to lower case, they are
  functions
* enhance fat.GuardXXXX() constructors: use ``*keys`` instead of expecting
  a tuple
* arg_type guard: use a weakref or document the strong reference
  to the type
* GuardBuiltins: remember init result to always fail in check?
  it helps at least unit tests
