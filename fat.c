#include "Python.h"
#include "frameobject.h"
#include "structmember.h"

#define VERSION "0.1"

static PyObject *init_builtins = NULL;


/* GuardArgType */

typedef struct {
    PyFuncGuardObject base;
    Py_ssize_t arg_index;
    Py_ssize_t nb_arg_type;
    PyObject** arg_types;
} GuardArgTypeObject;

static int
guard_arg_type_check(PyObject *self, PyObject **stack, int na, int nk)
{
    GuardArgTypeObject *guard = (GuardArgTypeObject *)self;
    PyObject *arg;
    PyTypeObject *type;
    Py_ssize_t i;
    int res;

    if (guard->arg_index >= na)
        return 1;

    arg = stack[guard->arg_index];
    type = Py_TYPE(arg);

    res = 1;
    for (i=0; i<guard->nb_arg_type; i++) {
        if (guard->arg_types[i] == (PyObject *)type) {
            res = 0;
            break;
        }
    }

    /* FIXME: implement keywords */

    return res;
}

static void
guard_arg_type_dealloc(GuardArgTypeObject *self)
{
    GuardArgTypeObject *guard = (GuardArgTypeObject *)self;
    Py_ssize_t i;

    for (i=0; i < guard->nb_arg_type; i++)
        Py_CLEAR(guard->arg_types[i]);
    PyMem_Free(guard->arg_types);
}

static int
guard_arg_type_traverse(GuardArgTypeObject *self, visitproc visit, void *arg)
{
    GuardArgTypeObject *guard = (GuardArgTypeObject *)self;
    Py_ssize_t i;

    for (i=0; i < guard->nb_arg_type; i++)
        Py_VISIT(guard->arg_types[i]);
    return 0;
}

static PyObject *
guard_arg_type_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *op;
    GuardArgTypeObject *self;

    op = PyFuncGuard_Type.tp_new(type, args, kwds);
    if (op == NULL)
        return NULL;

    self = (GuardArgTypeObject *)op;
    self->base.check = guard_arg_type_check;
    self->arg_index = 0;
    self->nb_arg_type = 0;
    self->arg_types = NULL;

    return op;
}

static int
guard_arg_type_init(PyObject *op, PyObject *args, PyObject *kwargs)
{
    GuardArgTypeObject *self = (GuardArgTypeObject *)op;
    static char *keywords[] = {"arg_index", "arg_types", NULL};
    int arg_index;
    PyObject *arg_types_obj;
    PyObject *seq = NULL;
    int nb_arg_type = 0;
    PyObject** arg_types = NULL;
    Py_ssize_t n, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO", keywords,
                                     &arg_index, &arg_types_obj))
        return -1;

    seq = PySequence_Fast(arg_types_obj, "arg_types must be a type or an iterable");
    if (seq == NULL)
        goto error;

    n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "need at least one argument type");
        goto error;
    }
    if (n >= PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(arg_types[0])) {
        PyErr_NoMemory();
        goto error;
    }

    arg_types = PyMem_Malloc(n * sizeof(arg_types[0]));
    if (arg_types == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    for (i=0; i<n; i++) {
        PyObject *type = PySequence_Fast_GET_ITEM(seq, i);
        if (!PyType_Check(type)) {
            PyErr_Format(PyExc_TypeError,
                         "arg_type must be a type, got %s",
                         Py_TYPE(type)->tp_name);
            goto error;
        }

        Py_INCREF(type);
        arg_types[i] = type;
        nb_arg_type = i+1;
    }

    Py_CLEAR(seq);

    self->arg_index = arg_index;
    self->nb_arg_type = nb_arg_type;
    self->arg_types = arg_types;
    return 0;

error:
    for (i=0; i<nb_arg_type; i++)
        Py_DECREF(arg_types[i]);
    PyMem_Free(arg_types);
    Py_XDECREF(seq);
    return -1;
}

static PyObject*
guard_arg_type_get_arg_types(GuardArgTypeObject *self)
{
    PyObject *list;
    Py_ssize_t i;

    list = PyTuple_New(self->nb_arg_type);
    if (list == NULL)
        return NULL;

    for (i=0; i < self->nb_arg_type; i++) {
        PyObject *type = self->arg_types[i];
        Py_INCREF(type);
        PyTuple_SET_ITEM(list, i, type);
    }
    return list;
}

static PyGetSetDef guard_arg_type_getsetlist[] = {
    {"arg_types", (getter)guard_arg_type_get_arg_types},
    {NULL} /* Sentinel */
};

static PyMemberDef guard_arg_type_members[] = {
    {"arg_index",   T_INT,   offsetof(GuardArgTypeObject, arg_index),
     RESTRICTED|READONLY},
    {NULL}  /* Sentinel */
};

static PyTypeObject GuardArgType_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fat.GuardArgType",
    sizeof(GuardArgTypeObject),
    0,
    (destructor)guard_arg_type_dealloc,         /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)guard_arg_type_traverse,      /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    guard_arg_type_members,                     /* tp_members */
    guard_arg_type_getsetlist,                  /* tp_getset */
    &PyFuncGuard_Type,                      /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    guard_arg_type_init,                        /* tp_init */
    0,                                          /* tp_alloc */
    guard_arg_type_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/* GuardFunc */

typedef struct {
    PyFuncGuardObject base;
    PyObject *func;
    PyObject *code;
} GuardFuncObject;

static int
guard_func_check(PyObject *self, PyObject** stack, int na, int nk)
{
    GuardFuncObject *guard = (GuardFuncObject *)self;
    PyFunctionObject *func;

    assert(Py_TYPE(guard->func) == &PyFunction_Type);
    func = (PyFunctionObject *)guard->func;

    if (((PyFunctionObject *)func)->func_code != guard->code)
        return 2;

    return 0;
}

static void
guard_func_dealloc(GuardFuncObject *self)
{
    GuardFuncObject *guard = (GuardFuncObject *)self;

    Py_XDECREF(guard->func);
    Py_XDECREF(guard->code);
}

static int
guard_func_traverse(GuardFuncObject *self, visitproc visit, void *arg)
{
    GuardFuncObject *guard = (GuardFuncObject *)self;

    Py_VISIT(guard->func);
    return 0;
}

static PyObject *
guard_func_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *op;
    GuardFuncObject *self;

    op = PyFuncGuard_Type.tp_new(type, args, kwds);
    if (op == NULL)
        return NULL;

    self = (GuardFuncObject *)op;
    self->base.check = guard_func_check;
    self->func = NULL;
    self->code = NULL;

    return op;
}

static int
guard_func_init(PyObject *op, PyObject *args, PyObject *kwargs)
{
    GuardFuncObject *self = (GuardFuncObject *)op;
    static char *keywords[] = {"func", NULL};
    PyObject *func;
    PyObject *code;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords,
                                     &func))
        return -1;

    if (!PyFunction_Check(func)) {
        PyErr_Format(PyExc_TypeError,
                     "func must be a function, not %s",
                     Py_TYPE(func)->tp_name);
        return -1;
    }

    /* FIXME: reimplement this check in guard_func_guard_init */
#if 0
    if ((PyObject *)func == spefunc) {
        /* spefunc_set_code() disables optimizations, no need to add a guard */
        PyErr_SetString(PyExc_ValueError,
                        "useless func guard, a function already watch itself");
        return -1;
    }
#endif

    code = ((PyFunctionObject*)func)->func_code;

    Py_INCREF(func);
    self->func = func;
    Py_INCREF(code);
    self->code = code;
    return 0;
}

static PyMemberDef guard_func_members[] = {
    {"func",   T_OBJECT,   offsetof(GuardFuncObject, func),
     RESTRICTED|READONLY},
    {"code",   T_OBJECT,   offsetof(GuardFuncObject, code),
     RESTRICTED|READONLY},
    {NULL}  /* Sentinel */
};

static PyTypeObject GuardFunc_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fat.GuardFunc",
    sizeof(GuardFuncObject),
    0,
    (destructor)guard_func_dealloc,             /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)guard_func_traverse,          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    guard_func_members,                         /* tp_members */
    0,                                          /* tp_getset */
    &PyFuncGuard_Type,                      /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    guard_func_init,                            /* tp_init */
    0,                                          /* tp_alloc */
    guard_func_new,                             /* tp_new */
    0,                                          /* tp_free */
};


/* GuardDict */

typedef struct {
    PyObject *key;
    PyObject *value;
} GuardDictPair;

typedef struct {
    PyFuncGuardObject base;
    PyObject *dict;
    size_t dict_version;
    Py_ssize_t npair;
    GuardDictPair *pairs;
} GuardDictObject;

static void
guard_dict_pair_dealloc(GuardDictPair *pair)
{
    Py_CLEAR(pair->key);
    Py_CLEAR(pair->value);
}

static void
guard_dict_clear(GuardDictObject *guard)
{
    Py_ssize_t i;

    Py_CLEAR(guard->dict);
    for (i=0; i < guard->npair; i++)
        guard_dict_pair_dealloc(&guard->pairs[i]);
    guard->npair = 0;
    PyMem_Free(guard->pairs);
    guard->pairs = NULL;
}

static int
check_dict_pair_guard(PyObject *dict, GuardDictPair *pair)
{
    PyObject *current_value;

    current_value = PyObject_GetItem(dict, pair->key);
    if (current_value == NULL && PyErr_Occurred()) {
        if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
            /* lookup faileds */
            return -1;
        }
        /* key doesn't exist */
        PyErr_Clear();
    }

    /* we only care of the value pointer, not its content,
       so it is safe to use the pointer after Py_DECREF */
    Py_XDECREF(current_value);

    if (current_value == pair->value) {
        /* another key was modified, but the watched key is unchanged */
        return 0;
    }

    /* the key was modified (removed or new value) */
    return 2;
}

static PyObject*
fat_get_builtins_dict(void)
{
    PyThreadState* tstate;
    PyObject *builtins;
    PyFrameObject *frame;

    tstate = PyThreadState_Get();
    if (tstate == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to get the current Python thread state");
        return NULL;
    }

    frame = tstate->frame;
    if (frame == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "the current Python thread state has no frame");
        return NULL;
    }

    builtins = frame->f_builtins;
    if (builtins == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "frame has no builtins");
        return NULL;
    }
    if (!PyDict_Check(builtins)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "frame builtins is not a dict");
        return NULL;
    }
    return builtins;
}

static int
guard_dict_check(PyObject *self, PyObject **stack, int na, int nk)
{
    GuardDictObject *guard = (GuardDictObject *)self;
    size_t dict_version;
    PyObject *dict;
    Py_ssize_t i;

    dict = guard->dict;
    assert(PyDict_Check(dict));

    dict_version = (((PyDictObject*)(dict))->ma_version);
    if (dict_version != guard->dict_version) {
        assert(guard->npair >= 1);

        for (i=0; i < guard->npair; i++) {
            int res = check_dict_pair_guard(dict, &guard->pairs[i]);
            if (res)
                return res;
        }

        guard->dict_version = dict_version;
    }

    return 0;
}

static void
guard_dict_dealloc(GuardDictObject *self)
{
    guard_dict_clear(self);
}

static int
guard_dict_traverse(GuardDictObject *guard, visitproc visit, void *arg)
{
    Py_ssize_t i;

    Py_VISIT(guard->dict);
    for (i=0; i < guard->npair; i++) {
        Py_VISIT(guard->pairs[i].key);
        Py_VISIT(guard->pairs[i].value);
    }
    return 0;
}

static PyObject *
guard_dict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *op;
    GuardDictObject *self;

    op = PyFuncGuard_Type.tp_new(type, args, kwds);
    if (op == NULL)
        return NULL;

    self = (GuardDictObject *)op;
    self->base.check = guard_dict_check;
    self->dict = NULL;
    self->dict_version = 0;
    self->npair = 0;
    self->pairs = NULL;
    return op;
}

static int
guard_dict_init_keys(PyObject *op, PyObject *dict, PyObject *keys)
{
    GuardDictObject *self = (GuardDictObject *)op;
    GuardDictPair *pairs = NULL;
    Py_ssize_t nkeys, i, npair = 0;

    /* FIXME: PyDict_CheckExact(dict)? */

    if (!PyTuple_Check(keys)) {
        PyErr_Format(PyExc_TypeError,
                     "keys must be a tuple of str, not %s",
                     Py_TYPE(keys)->tp_name);
        goto error;
    }

    nkeys = PyTuple_GET_SIZE(keys);
    if (!nkeys) {
        PyErr_SetString(PyExc_TypeError,
                        "keys must at least contain one key");
        goto error;
    }

    if (nkeys >  PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(GuardDictPair)) {
        PyErr_NoMemory();
        goto error;
    }
    pairs = PyMem_Malloc(sizeof(GuardDictPair) * nkeys);
    if (pairs == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    for (i=0; i < nkeys; i++) {
        PyObject *key, *value;

        key = PyTuple_GET_ITEM(keys, i);

        if (!PyUnicode_Check(key)) {
            PyErr_Format(PyExc_TypeError,
                         "key must be str, not %s",
                         Py_TYPE(key)->tp_name);
            goto error;
        }

        value = PyObject_GetItem(dict, key);
        if (value == NULL && PyErr_Occurred()) {
            if (!PyErr_ExceptionMatches(PyExc_KeyError))
                goto error;
            /* key doesn't exist */
            PyErr_Clear();
        }

        Py_INCREF(key);
        pairs[i].key = key;
        pairs[i].value = value;
        npair = i + 1;
    }

    guard_dict_clear(self);

    Py_INCREF(dict);
    self->dict = dict;
    self->dict_version = (((PyDictObject*)(dict))->ma_version);
    self->npair = npair;
    self->pairs = pairs;
    return 0;

error:
    for (i=0; i < npair; i++)
        guard_dict_pair_dealloc(&pairs[i]);
    PyMem_Free(pairs);
    return -1;
}

static int
guard_dict_init(PyObject *op, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"dict", "keys", NULL};
    PyObject *dict, *keys;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O", keywords,
                                     &PyDict_Type, &dict, &keys))
        return -1;

    return guard_dict_init_keys(op, dict, keys);
}

static PyObject*
guard_dict_get_keys(GuardDictObject *self)
{
    PyObject *tuple;
    Py_ssize_t i;

    tuple = PyTuple_New(self->npair);
    if (tuple == NULL)
        return NULL;

    for (i=0; i < self->npair; i++) {
        GuardDictPair *pair = &self->pairs[i];

        Py_INCREF(pair->key);
        PyTuple_SET_ITEM(tuple, i, pair->key);
    }
    return tuple;
}

static PyGetSetDef guard_dict_getsetlist[] = {
    {"keys", (getter)guard_dict_get_keys},
    {NULL} /* Sentinel */
};

static PyMemberDef guard_dict_members[] = {
    {"dict",   T_OBJECT,   offsetof(GuardDictObject, dict),
     RESTRICTED|READONLY},
    {NULL}  /* Sentinel */
};

static PyTypeObject GuardDict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fat.GuardDict",
    sizeof(GuardDictObject),
    0,
    (destructor)guard_dict_dealloc,             /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)guard_dict_traverse,          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    guard_dict_members,                         /* tp_members */
    guard_dict_getsetlist,                      /* tp_getset */
    &PyFuncGuard_Type,                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    guard_dict_init,                            /* tp_init */
    0,                                          /* tp_alloc */
    guard_dict_new,                             /* tp_new */
    0,                                          /* tp_free */
};


/* GuardBuiltins */

typedef struct {
    GuardDictObject base;
    PyObject *extra_guard;
} GuardBuiltinsObject;

static void
guard_builtins_dealloc(GuardBuiltinsObject *self)
{
    guard_dict_dealloc(&self->base);
    Py_CLEAR(self->extra_guard);
}

static int
guard_builtins_init_guard(PyObject *self, PyObject *func)
{
    GuardBuiltinsObject *guard = (GuardBuiltinsObject *)self;
    Py_ssize_t i;
    PyObject *init_value;
    GuardDictObject *globals_guard;

    assert(init_builtins != NULL);
    for (i=0; i < guard->base.npair; i++) {
        PyObject *name = guard->base.pairs[i].key;

        init_value = PyDict_GetItem(init_builtins, name);
        if (init_value != NULL) {
            if (guard->base.pairs[i].value != init_value) {
                /* builtin was modified since Python initialization:
                   don't specialize the function */
                return 1;
            }
        }
        PyErr_Clear();
    }

    globals_guard = (GuardDictObject *)guard->extra_guard;
    for (i=0; i < globals_guard->npair; i++) {
        if (globals_guard->pairs[i].value != NULL) {
            /* if name already exists in global, the guard must fail */
            return 1;
        }
    }
    return 0;
}

static int
guard_builtins_check(PyObject *self, PyObject **stack, int na, int nk)
{
    GuardBuiltinsObject *guard = (GuardBuiltinsObject *)self;
    PyFuncGuardObject *extra_guard = (PyFuncGuardObject *)guard->extra_guard;
    int res;

    res = guard_dict_check(self, stack, na, nk);
    if (res)
        return res;

    return extra_guard->check(guard->extra_guard, stack, na, nk);
}

static PyObject *
guard_builtins_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *op;
    GuardBuiltinsObject *self;

    op = GuardDict_Type.tp_new(type, args, kwds);
    if (op == NULL)
        return NULL;

    self = (GuardBuiltinsObject *)op;
    self->base.base.init = guard_builtins_init_guard;
    self->base.base.check = guard_builtins_check;

    return op;
}

static int
guard_builtins_init(PyObject *op, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"keys", NULL};
    PyObject *builtins, *globals, *keys;
    PyObject *extra_guard;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords,
                                     &keys))
        return -1;

    builtins = fat_get_builtins_dict();
    if (builtins == NULL)
        return -1;

    globals = PyEval_GetGlobals();
    if (globals == NULL)
        return -1;

    extra_guard = PyObject_CallFunction((PyObject *)&GuardDict_Type, "OO", globals, keys);
    if (extra_guard == NULL)
        return -1;

    if (guard_dict_init_keys(op, builtins, keys) < 0) {
        Py_DECREF(extra_guard);
        return -1;
    }

    ((GuardBuiltinsObject *)op)->extra_guard = extra_guard;

    return 0;
}


static PyTypeObject GuardBuiltins_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fat.GuardBuiltins",
    sizeof(GuardBuiltinsObject),
    0,
    (destructor)guard_builtins_dealloc,         /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    &GuardDict_Type,                            /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    guard_builtins_init,                        /* tp_init */
    0,                                          /* tp_alloc */
    guard_builtins_new,                         /* tp_new */
    0,                                          /* tp_free */
};


static PyObject*
fat_guard_globals(PyObject *self, PyObject *args)
{
    PyObject *globals, *keys;

    if (!PyArg_ParseTuple(args, "O:guard_globals", &keys))
        return NULL;

    globals = PyEval_GetGlobals();
    if (globals == NULL)
        return NULL;

    return PyObject_CallFunction((PyObject *)&GuardDict_Type, "OO", globals, keys);
}

PyDoc_STRVAR(guard_globals_doc,
"guard_globals(keys) -> GuardDict\n"
"\n"
"Guard on globals()[key] for all keys.");


static PyObject*
fat_guard_type_dict(PyObject *self, PyObject *args)
{
    PyObject *type, *type_dict, *keys;

    if (!PyArg_ParseTuple(args, "O!O:guard_type_dict", &PyType_Type, &type, &keys))
        return NULL;

    type_dict = ((PyTypeObject*)type)->tp_dict;
    assert(type_dict != NULL);

    return PyObject_CallFunction((PyObject *)&GuardDict_Type, "OO", type_dict, keys);
}

PyDoc_STRVAR(guard_type_dict_doc,
"guard_type_dict(attrs) -> GuardDict\n"
"\n"
"Guard on type.attr (type.__dict__[attr]) for all attrs.");


static PyObject*
replace_consts(PyObject *consts, PyObject *mapping)
{
    PyObject *new_consts, *value, *new_value;
    Py_ssize_t i, size;

    assert(PyTuple_CheckExact(consts));
    assert(PyDict_Check(mapping));
    size = PyTuple_GET_SIZE(consts);

    new_consts = PyTuple_New(size);
    for (i=0; i<size; i++) {
        value = PyTuple_GET_ITEM(consts, i);

        new_value = PyDict_GetItem(mapping, value);
        if (new_value == NULL && PyErr_Occurred()) {
            /* truncate the tuple to not read unitilized memory in
               the tuple destructor */
            Py_SIZE(new_consts) = i;
            Py_DECREF(new_consts);
            return NULL;
        }

        if (new_value != NULL)
            value = new_value;

        Py_INCREF(value);
        PyTuple_SET_ITEM(new_consts, i, value);
    }

    return new_consts;
}

static PyObject *
fat_replace_consts(PyObject *self, PyObject *args)
{
    PyCodeObject *code;
    PyObject *mapping;
    PyObject *new_consts, *new_code;

    if (!PyArg_ParseTuple(args, "O!O!:replace_consts",
                          &PyCode_Type, &code,
                          &PyDict_Type, &mapping))
        return NULL;

    new_consts = replace_consts(code->co_consts, mapping);
    if (new_consts == NULL)
        return NULL;

    new_code = (PyObject *)PyCode_New(
        code->co_argcount,
        code->co_kwonlyargcount,
        code->co_nlocals,
        code->co_stacksize,
        code->co_flags,
        code->co_code,
        new_consts,                /* replace constants */
        code->co_names,
        code->co_varnames,
        code->co_freevars,
        code->co_cellvars,
        code->co_filename,
        code->co_name,
        code->co_firstlineno,
        code->co_lnotab);
    Py_DECREF(new_consts);

    return new_code;
}

PyDoc_STRVAR(patch_constants_doc,
"replace_constants(code, mapping) -> code\n"
"\n"
"Create a new code object with new constants using the constant mapping:\n"
"old constant value => new constant value.");


static PyObject *
fat_specialize(PyObject *self, PyObject *args)
{
    PyObject *func, *code, *guards;
    int res;

    if (!PyArg_ParseTuple(args, "O!OO:specialize",
                          &PyFunction_Type, &func, &code, &guards))
        return NULL;

    res = PyFunction_Specialize(func, code, guards);
    if (res < 0)
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(specialize_doc,
"specialize(func, code, guards) -> bool\n"
"\n"
"Specialize a function: add a specialized code with guards.");


static PyObject *
fat_get_specialized(PyObject *self, PyObject *args)
{
    PyObject *func;

    if (!PyArg_ParseTuple(args, "O!:get_specialized",
                          &PyFunction_Type, &func))
        return NULL;

    return PyFunction_GetSpecializedCodes(func);
}

PyDoc_STRVAR(get_specialized_doc,
"get_specialized(func) -> list\n"
"\n"
"Get the list of specialized codes as a list of (code, guards)\n"
"tuples where code is a callable or code object and guards is a list\n"
"of guards.");

static struct PyMethodDef fat_methods[] = {
    {"specialize", (PyCFunction)fat_specialize, METH_VARARGS,
     specialize_doc},
    {"get_specialized", (PyCFunction)fat_get_specialized, METH_VARARGS,
     get_specialized_doc},
    {"replace_consts", (PyCFunction)fat_replace_consts, METH_VARARGS,
     patch_constants_doc},
    {"guard_globals", (PyCFunction)fat_guard_globals, METH_VARARGS,
     guard_globals_doc},
    {"guard_type_dict", (PyCFunction)fat_guard_type_dict, METH_VARARGS,
     guard_type_dict_doc},
    {NULL, NULL}                /* sentinel */
};


PyDoc_STRVAR(fat_doc,
"fat module.");

static struct PyModuleDef fatmodule = {
    PyModuleDef_HEAD_INIT,
    "fat",               /* m_name */
    fat_doc,           /* m_doc */
    0,                    /* m_size */
    fat_methods,          /* m_methods */
    NULL,                 /* m_reload */
    NULL,                 /* m_traverse */
    NULL,                 /* m_clear */
    NULL                  /* m_free */
};

int
fat_init_builtins(void)
{
    PyThreadState* tstate;
    PyObject *builtins;

    if (init_builtins != NULL)
        /* already initialized */
        return 0;

    tstate = PyThreadState_Get();
    if (tstate == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to get the current Python thread state");
        return -1;
    }

    builtins = tstate->interp->builtins;
    if (builtins == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "interpreter builtins are unset");
        return -1;
    }

    init_builtins = PyDict_Copy(builtins);
    if (init_builtins == NULL)
        return -1;

    return 0;
}

PyMODINIT_FUNC
PyInit_fat(void)
{
    PyObject *mod, *value;

    if (fat_init_builtins() < 0)
        return NULL;

    mod = PyModule_Create(&fatmodule);
    if (mod == NULL)
        return NULL;

    if (PyType_Ready(&GuardFunc_Type) < 0)
        return NULL;

    if (PyType_Ready(&GuardArgType_Type) < 0)
        return NULL;

    if (PyType_Ready(&GuardDict_Type) < 0)
        return NULL;

    if (PyType_Ready(&GuardBuiltins_Type) < 0)
        return NULL;

    value = PyUnicode_FromString(VERSION);
    if (value == NULL)
        return NULL;
    if (PyModule_AddObject(mod, "__version__", value) < 0)
        return NULL;

    Py_INCREF(&PyFuncGuard_Type);
    if (PyModule_AddObject(mod, "_Guard",
                           (PyObject *)&PyFuncGuard_Type) < 0)
        return NULL;

    Py_INCREF(&GuardFunc_Type);
    if (PyModule_AddObject(mod, "GuardFunc",
                           (PyObject *)&GuardFunc_Type) < 0)
        return NULL;

    Py_INCREF(&GuardArgType_Type);
    if (PyModule_AddObject(mod, "GuardArgType",
                           (PyObject *)&GuardArgType_Type) < 0)
        return NULL;

    Py_INCREF(&GuardDict_Type);
    if (PyModule_AddObject(mod, "GuardDict",
                           (PyObject *)&GuardDict_Type) < 0)
        return NULL;

    Py_INCREF(&GuardBuiltins_Type);
    if (PyModule_AddObject(mod, "GuardBuiltins",
                           (PyObject *)&GuardBuiltins_Type) < 0)
        return NULL;

    return mod;
}
