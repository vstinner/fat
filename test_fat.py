# Disable fatoptimizer on this module
__fatoptimizer__ = {'enabled': False}

import builtins
import fat
import os.path
import sys
import textwrap
import unittest


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        transformers = sys.get_code_transformers()
        self.addCleanup(sys.set_code_transformers, transformers)

        # Disable all code tranformers, we specialized functions
        # manually in tests
        sys.set_code_transformers([])


class GuardsTests(BaseTestCase):
    # fat.GuardFunc is tested in fattester.py

    def test_guard(self):
        guard = fat._Guard()
        self.assertEqual(guard(), 0)

    def test_guard_arg_type(self):
        guard = fat.GuardArgType(2, (int,))
        self.assertEqual(guard.arg_index, 2)
        self.assertEqual(guard.arg_types, (int,))

        self.assertEqual(guard(1, 2, 3), 0)
        self.assertEqual(guard(1, 2, "hello"), 1)

    def test_guard_dict(self):
        ns = {'key': 1}

        guard = fat.GuardDict(ns, ('key',))
        self.assertIs(guard.dict, ns)
        self.assertEqual(guard.keys, ('key',))

        self.assertEqual(guard(), 0)

        ns['key'] = 2
        self.assertEqual(guard(), 2)

    def test_globals(self):
        guard = fat.GuardGlobals(('key',))
        self.assertIs(guard.dict, globals())
        self.assertEqual(guard.keys, ('key',))

        # not enough parameters
        self.assertRaises(TypeError, fat.GuardGlobals)

        # wrong types
        self.assertRaises(TypeError, fat.GuardGlobals, 123)
        self.assertRaises(TypeError, fat.GuardGlobals, (123,))

    def test_globals_replace_globals(self):
        guard = fat.GuardGlobals(('key',))
        self.assertEqual(guard(), 0)

        # check the guard in a global namespace different than the global
        # namespace used to create the guard
        ns = {'guard': guard}
        exec("check = guard()", ns)
        check = ns['check']

        self.assertEqual(check, 2)

    def test_builtins(self):
        guard = fat.GuardBuiltins(('key',))
        self.assertIs(guard.dict, builtins.__dict__)
        self.assertEqual(guard.keys, ('key',))

        guard_globals = guard.guard_globals
        self.assertIs(guard_globals.dict, globals())
        self.assertEqual(guard_globals.keys, ('key',))

        # not enough parameters
        self.assertRaises(TypeError, fat.GuardBuiltins)

        # wrong types
        self.assertRaises(TypeError, fat.GuardBuiltins, 123)
        self.assertRaises(TypeError, fat.GuardBuiltins, (123,))

    def test_builtins_replace_builtins(self):
        ns = {'fat': fat}
        exec("guard = fat.GuardBuiltins(('key',))", ns)
        guard = ns['guard']

        def check():
            exec("check = guard()", ns)
            return ns['check']

        # check the guard with a replaced builtins dictionary
        self.assertEqual(check(), 0)

        # check the guard with a replaced builtins dictionary
        ns['__builtins__'] = ns['__builtins__'].copy()

        self.assertEqual(check(), 2)

    def test_builtins_global_exists(self):
        global global_var

        try:
            global_var = "hello"
            guard = fat.GuardBuiltins(('global_var',))

            # builtin overriden in the global namespace
            self.assertEqual(guard(), 2)
        finally:
            del global_var

        # even if the global is removed, the guard remembers that the
        # global was replaced
        self.assertEqual(guard(), 2)

    def test_builtins_replace_globals(self):
        guard = fat.GuardBuiltins(('key',))
        self.assertEqual(guard(), 0)

        # check the guard in a global namespace different than the global
        # namespace used to create the guard
        ns = {'guard': guard}
        exec("check = guard()", ns)
        check = ns['check']

        self.assertEqual(check, 2)

    def test_guard_func(self):
        def func():
            return 3

        guard = fat.GuardFunc(func)
        self.assertIs(guard.func, func)
        self.assertIs(guard.code, func.__code__)

        self.assertEqual(guard(), 0)

        def func2():
            return 4

        func.__code__ = func2.__code__
        self.assertEqual(guard(), 2)


def guard_dict(ns, key):
    return [fat.GuardDict(ns, (key,))]


class BaseTests(BaseTestCase):
    def check_guard(self, guard, expected):
        guard_type = type(expected)
        self.assertEqual(type(guard), guard_type)

        if guard_type == fat.GuardArgType:
            attrs = ('arg_index', 'arg_types')
        elif guard_type in (fat.GuardDict, fat.GuardBuiltins):
            attrs = ('dict', 'keys')
        elif guard_type == fat.GuardFunc:
            attrs = ('func', 'code')
        else:
            raise NotImplementedError("unknown guard type")

        for attr in attrs:
            self.assertEqual(getattr(guard, attr),
                             getattr(expected, attr),
                             attr)

    def check_guards(self, guards, expected):
        self.assertIsInstance(guards, list)
        self.assertEqual(len(guards), len(expected))
        for guard1, guard2 in zip(guards, expected):
            self.check_guard(guard1, guard2)

    def check_specialized(self, func, *expected):
        specialized = fat.get_specialized(func)
        self.assertEqual(len(specialized), len(expected))

        for item1, item2 in zip(specialized, expected):
            self.assertIsInstance(item1, tuple)
            self.assertEqual(len(item1), 2)
            code1, guards1 = item1
            code2, guards2 = item2
            self.check_guards(guards1, guards2)
            self.assertEqual(code1.co_name, func.__name__)
            self.assertEqual(code1.co_code, code2.co_code)


class GetSpecializedTests(BaseTests):
    """Tests for fat.get_specialized(func)."""

    def test_no_specialized(self):
        def func():
            pass

        self.assertEqual(fat.get_specialized(func), [])

    def test_no_guards(self):
        def func():
            pass

        ns = {}
        guards = guard_dict(ns, 'key')

        def func2():
            pass
        fat.specialize(func, func2, guards)

        def func3():
            pass
        fat.specialize(func, func3, guards)

        self.check_specialized(func,
                               (func2.__code__, guards),
                               (func3.__code__, guards))

        # setting __code__ must remove all specialized functions
        def mock_func():
            return "mock"
        func.__code__ = mock_func.__code__

        self.assertEqual(fat.get_specialized(func), [])

    def test_arg_type_guard(self):
        def func(a, b, c):
            return 'slow'

        def func2(a, b, c):
            return 'fast'
        fat.specialize(func, func2, [fat.GuardArgType(2, [set])])

        guard = fat.GuardArgType(2, (set,))
        self.check_specialized(func,
                               (func2.__code__, [guard]))

        # If the guard fails, the specialized function must no be removed
        self.assertEqual(func(0, 1, "abc"), 'slow')
        self.check_specialized(func,
                               (func2.__code__, [guard]))

    def test_arg_type_type_list_guard(self):
        def func(a, b, c):
            return 'slow'

        def func2(a, b, c):
            return 'fast'
        fat.specialize(func, func2, [fat.GuardArgType(2, [list, set])])

        guard = fat.GuardArgType(2, (list, set))
        self.check_specialized(func,
                               (func2.__code__, [guard]))

        # If the guard fails, the specialized function must no be removed
        self.assertEqual(func(0, 1, "abc"), 'slow')
        self.check_specialized(func,
                               (func2.__code__, [guard]))

    def test_dict_guard(self):
        ns = dict(mykey=4)

        def func():
            return 'slow'

        def func2():
            return 'fast'
        guards = guard_dict(ns, 'mykey')
        fat.specialize(func, func2, guards)

        self.check_specialized(func,
                               (func2.__code__, guards))

        # Modify func, so the guard will fail
        def mock_func():
            return 'mock'
        func.__code__ = mock_func.__code__

        # Calling the function checks the guards and then removed the
        # specialized function since the function was modified
        self.assertEqual(func(), 'mock')
        self.assertEqual(fat.get_specialized(func), [])

    def test_func_guard(self):
        def inlined():
            pass

        def func():
            pass

        def func2():
            pass

        fat.specialize(func, func2, [fat.GuardFunc(inlined)])

        guard = fat.GuardFunc(inlined)
        self.check_specialized(func,
                               (func2.__code__, [guard]))


class BehaviourTests(BaseTestCase):
    """Test behaviour of specialized functions."""

    def test_modify_func_code(self):
        def func():
            return "slow"

        def fast():
            return "fast"

        ns = {}
        guards = guard_dict(ns, 'key')

        fat.specialize(func, fast, guards)
        self.assertEqual(func(), 'fast')
        self.assertEqual(len(fat.get_specialized(func)), 1)

        def mock_func():
            return 'mock'

        # setting __code__ must disable all optimizations
        func.__code__ = mock_func.__code__
        self.assertEqual(func(), 'mock')
        self.assertEqual(len(fat.get_specialized(func)), 0)

    def _exec(self, code):
        ns = {}
        ns['__builtins__'] = dict(builtins.__dict__)
        exec(code, ns, ns)
        return ns

    def specialized_len(self):
        code = textwrap.dedent("""
            import fat

            def func():
                return len("abc")

            def fast():
                return "fast: 3"

            fat.specialize(func, fast, [fat.GuardBuiltins(('len',))])
        """)

        ns = self._exec(code)
        return ns, ns['func']

    def test_builtin_len_mock_globals(self):
        ns, func = self.specialized_len()

        def call():
            ns.pop('res', None)
            exec("res = func()", ns)
            return ns['res']

        self.assertEqual(call(), 'fast: 3')

        # mock len() in the function namespace
        ns['len'] = lambda obj: "mock"

        self.assertEqual(call(), 'mock')

    def test_builtin_len_mock_builtin(self):
        ns, func = self.specialized_len()

        def call():
            ns.pop('res', None)
            exec("res = func()", ns)
            return ns['res']

        self.assertEqual(call(), 'fast: 3')

        len = builtins.len
        try:
            # mock len() in the function namespace
            ns['__builtins__']['len'] = lambda obj: 'mock'

            #builtins.__dict__['len'] = lambda obj: 'mock'
            res = call()
        finally:
            builtins.len = len
        self.assertEqual(res, 'mock')

    def inline(self):
        code = textwrap.dedent("""
            import fat

            def is_python(filename):
                return filename.endswith('.py')

            def func(filename):
                return is_python(filename)

            def fast(filename):
                return "fast: %s" % filename.endswith('.py')

            fat.specialize(func, fast,
                             [fat.GuardGlobals(('is_python',)),
                              fat.GuardFunc(is_python)])
        """)
        ns = self._exec(code)
        return ns, ns['func']

    def test_inline_mock_globals(self):
        ns, func = self.inline()

        def call(arg):
            ns.pop('res', None)
            exec("res = func(%r)" % arg, ns)
            return ns['res']

        self.assertEqual(call('abc'), 'fast: False')
        self.assertEqual(call('abc.py'), 'fast: True')

        # modify is_python() in the module namespace
        def is_python(filename):
            return "mock: %s" % filename
        ns['is_python'] = is_python

        self.assertEqual(call('abc'), 'mock: abc')
        self.assertEqual(call('abc.py'), 'mock: abc.py')

    def test_inline_modify_code(self):
        ns, func = self.inline()

        def call(arg):
            ns.pop('res', None)
            exec("res = func(%r)" % arg, ns)
            return ns['res']

        self.assertEqual(call('abc'), 'fast: False')
        self.assertEqual(call('abc.py'), 'fast: True')

        # modify is_python() code
        def mock_is_python(filename):
            return 'mock: %s' % filename
        ns['is_python'].__code__ = mock_is_python.__code__

        self.assertEqual(call('abc'), 'mock: abc')
        self.assertEqual(call('abc.py'), 'mock: abc.py')

    def test_arg_type_int(self):
        def func(obj):
            return 'slow'

        def fast(obj):
            return 'fast'

        fat.specialize(func, fast, [fat.GuardArgType(0, (int,))])

        self.assertEqual(func(5), 'fast')
        self.assertEqual(func("abc"), 'slow')

    def test_arg_type_class(self):
        class MyClass:
            pass

        def func(obj):
            return 'slow'

        def fast(obj):
            return 'fast'

        fat.specialize(func, fast, [fat.GuardArgType(0, (MyClass,))])

        obj = MyClass()
        self.assertEqual(func(obj), 'fast')
        self.assertEqual(func("test"), 'slow')

    def test_arg_type(self):
        def func(x):
            return "slow: %s" % x

        def fast(x):
            return "fast: %s" % x

        fat.specialize(func, fast, [fat.GuardArgType(0, (int,))])

        self.assertEqual(func(3), 'fast: 3')

        # FIXME: implement keywords
        #self.assertEqual(func(x=4), 'fast: 4')

        # calling with the wrong number of parameter must not disable the
        # optimization
        with self.assertRaises(TypeError):
            func()
        with self.assertRaises(TypeError):
            func(1, 2,)
        self.assertEqual(func(5), 'fast: 5')

        # wrong type, skip optimization
        self.assertEqual(func(6.0), 'slow: 6.0')

        # optimization must not be disabled after call with wrong types
        self.assertEqual(func(7), 'fast: 7')

    def test_builtin_guard_builtin_replaced(self):
        code = textwrap.dedent("""
            import fat

            __builtins__['chr'] = lambda obj: "mock"

            def func():
                return chr(65)

            def fast():
                return "fast: A"

            guard = fat.GuardBuiltins(('chr',))
            fat.specialize(func, fast, [guard])
        """)

        ns = self._exec(code)
        func = ns['func']
        guard = ns['guard']

        # chr() was replaced: the specialization must be ignored
        self.assertEqual(len(fat.get_specialized(func)), 0)

        # guard init failed: it must always fail
        self.assertEqual(guard(), 2)

    def test_builtin_guard_global_exists(self):
        code = textwrap.dedent("""
            import fat

            chr = lambda obj: "mock"

            def func():
                return chr(65)

            def fast():
                return "fast: A"

            guard = fat.GuardBuiltins(('chr',))
            fat.specialize(func, fast, [guard])
        """)

        ns = self._exec(code)
        func = ns['func']
        guard = ns['guard']

        # chr() is overriden in the global namespace: the specialization must
        # be ignored
        self.assertEqual(len(fat.get_specialized(func)), 0)

        # guard init failed: it must always fail
        self.assertEqual(guard(), 2)


class MethodTests(BaseTestCase):
    """Test specialized methods."""

    def basic(self):
        ns = {}
        guards = guard_dict(ns, 'key')

        class MyClass:
            def meth(self):
                return 'slow'

            def _fast(self):
                return 'fast'

            fat.specialize(meth, _fast, guards)
            del _fast

        obj = MyClass()

        def invalidate_guard():
            ns['key'] = 2

        return (MyClass, obj, invalidate_guard)

    def test_basic_invalidate_guard(self):
        MyClass, obj, invalidate_guard = self.basic()
        self.assertEqual(obj.meth(), 'fast')

        invalidate_guard()
        self.assertEqual(obj.meth(), 'slow')

    def test_basic_mock_class_method(self):
        MyClass, obj, invalidate_guard = self.basic()
        self.assertEqual(obj.meth(), 'fast')

        old_meth = MyClass.meth

        MyClass.meth = lambda obj: 'mock'
        self.assertEqual(obj.meth(), 'mock')

        MyClass.meth = old_meth
        self.assertEqual(obj.meth(), 'fast')

    def test_basic_mock_obj_method(self):
        MyClass, obj, invalidate_guard = self.basic()
        self.assertEqual(obj.meth(), 'fast')

        obj.meth = lambda: 'mock'
        self.assertEqual(obj.meth(), 'mock')

        del obj.meth
        self.assertEqual(obj.meth(), 'fast')


class SpecializeTests(BaseTests):
    """Test func.specialize() function."""

    def test_duplicated(self):
        def func():
            pass

        # register the same function twice
        def func2():
            pass

        ns = {}
        guards = guard_dict(ns, 'key')

        fat.specialize(func, func2, guards)
        fat.specialize(func, func2, guards)
        self.check_specialized(func,
                                (func2.__code__, guards),
                                (func2.__code__, guards))

    def test_over_specialized(self):
        def func():
            pass

        def func2():
            pass

        def func3():
            pass

        ns = {}
        guards = guard_dict(ns, 'key')

        fat.specialize(func2, func3, guards)

        with self.assertRaises(ValueError) as cm:
            fat.specialize(func, func2, guards)
        self.assertEqual(str(cm.exception),
                         'cannot specialize a function with another function '
                         'which is already specialized')

    def test_specialize_error(self):
        def func():
            pass

        with self.assertRaises(ValueError) as cm:
            fat.specialize(func, func, [{'guard_type': 'func', 'func': func}])
        self.assertEqual(str(cm.exception),
                         "a function cannot specialize itself")

    def test_cellvars(self):
        def func(data, cb):
            return [cb(item) for item in data]

        def fast(data, cb):
            return 3

        self.assertNotEqual(func.__code__.co_cellvars, ())
        self.assertEqual(fast.__code__.co_cellvars, ())

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode uses different cell variables")

    def test_specialize_with_cellvars(self):
        def func(data, cb):
            return 3

        def fast(data, cb):
            return [cb(item) for item in data]

        self.assertEqual(func.__code__.co_cellvars, ())
        self.assertNotEqual(fast.__code__.co_cellvars, ())

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode uses different cell variables")

    def test_freevars(self):
        def create_func():
            x = 1
            def func():
                return x
            return func
        func = create_func()

        def fast():
            return 1

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode uses different free variables")

    def test_specialize_with_freevars(self):
        def func():
            return 1

        def create_fast():
            x = 1
            def fast():
                return x
            return fast
        fast = create_fast()

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode uses different free variables")

    def test_argcount(self):
        def func(x):
            return 1

        def fast(x, y):
            return 2

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode doesn't have the same "
                             "number of parameters")

    def test_kwonlyargcount(self):
        def func(*, x=1):
            return 1

        def fast(*, x=1, y=1):
            return 2

        ns = {}
        guards = guard_dict(ns, 'key')

        for bytecode in (False, True):
            with self.assertRaises(ValueError) as cm:
                if bytecode:
                    fat.specialize(func, fast.__code__, guards)
                else:
                    fat.specialize(func, fast, guards)
            self.assertEqual(str(cm.exception),
                             "specialized bytecode doesn't have the same "
                             "number of parameters")

    def test_defaults(self):
        def func(x=1):
            return 1

        def fast(x=2):
            return 2

        ns = {}
        guards = guard_dict(ns, 'key')

        with self.assertRaises(ValueError) as cm:
            fat.specialize(func, fast, guards)
        self.assertEqual(str(cm.exception),
                         "specialized function doesn't have "
                         "the same parameter defaults")

    def test_kwdefaults(self):
        def func(*, x=1):
            return 1

        def fast(*, x=2):
            return 2

        ns = {}
        guards = guard_dict(ns, 'key')

        with self.assertRaises(ValueError) as cm:
            fat.specialize(func, fast, guards)
        self.assertEqual(str(cm.exception),
                         "specialized function doesn't have "
                         "the same keyword parameter defaults")

    def test_invalid_guard_type(self):
        def func():
            pass

        def func2():
            pass

        with self.assertRaises(TypeError):
            fat.specialize(func, func2, ['xxx'])

    def test_add_func_guard_error(self):
        with self.assertRaises(TypeError) as cm:
            fat.GuardFunc()

        with self.assertRaises(TypeError) as cm:
            # invalid function type
            fat.GuardFunc('abc')
        self.assertEqual(str(cm.exception),
                         "func must be a function, not str")

        # FIXME: reimplement this test
        #with self.assertRaises(ValueError) as cm:
        #    # must not watch itself
        #    fat.specialize(func, func2,
        #                    [{"guard_type": "func", "func": func}])
        #self.assertEqual(str(cm.exception),
        #                 "useless func guard, a function already watch itself")

    def test_add_builtins_guard_error(self):
        with self.assertRaises(TypeError) as cm:
            # missing 'keys' parameter
            fat.GuardBuiltins()

        with self.assertRaises(TypeError) as cm:
            # invalid 'name' type
            fat.GuardBuiltins(123)
        self.assertEqual(str(cm.exception),
                         "keys must be a tuple of str, not int")

        with self.assertRaises(TypeError) as cm:
            # invalid 'name' type
            fat.GuardBuiltins((123,))
        self.assertEqual(str(cm.exception),
                         "key must be str, not int")

    def test_add_dict_guard_error(self):
        d = {"key": 3}

        # missing 'dict' and/or 'key' keys
        with self.assertRaises(TypeError):
            fat.GuardDict()
        with self.assertRaises(TypeError):
            fat.GuardDict(d)

        with self.assertRaises(TypeError):
            # type argument is not a type
            fat.GuardDict(123, ("key",))

        with self.assertRaises(TypeError) as cm:
            # key argument is not a str
            fat.GuardDict(d, (123,))
        self.assertEqual(str(cm.exception),
                         "key must be str, not int")

    def test_add_type_dict_guard_error(self):
        def func():
            pass

        def func2():
            pass

        self.assertRaises(TypeError, fat.guard_type_dict)
        self.assertRaises(TypeError, fat.guard_type_dict, str)
        self.assertRaises(TypeError, fat.guard_type_dict, 123, ('attr',))
        self.assertRaises(TypeError, fat.guard_type_dict, 123, (123,))

    def test_add_arg_type_guard_error(self):
        # missing 'arg_index' and/or 'type' keys
        with self.assertRaises(TypeError):
            fat.GuardArgType()
        with self.assertRaises(TypeError):
            fat.GuardArgType(0)

        with self.assertRaises(TypeError) as cm:
            fat.GuardArgType("abc", (str,))

        with self.assertRaises(TypeError) as cm:
            # arg_type argument is not a type
            fat.GuardArgType(2, 123)
        self.assertEqual(str(cm.exception),
                         "arg_types must be a type or an iterable")

        with self.assertRaises(TypeError) as cm:
            # arg_type items are not types
            fat.GuardArgType(2, (123,))
        self.assertEqual(str(cm.exception),
                         "arg_type must be a type, got int")


class MiscTests(BaseTestCase):
    def test_replace_constants(self):
        def func():
            return 3

        code = func.__code__
        self.assertEqual(code.co_consts, (None, 3))

        code2 = fat.replace_consts(code, {3: 'new constant'})
        self.assertEqual(code2.co_consts, (None, 'new constant'))

        code3 = fat.replace_consts(code, {'unknown': 7})
        self.assertEqual(code3.co_consts, (None, 3))

    def test_version(self):
        import setup
        self.assertEqual(fat.__version__, setup.VERSION)



if __name__ == "__main__":
    unittest.main()
