#include "Python.h"

#include <stdio.h>
#include <string.h>

#include "structmember.h"
#include "otama.h"

#define MAX_BUFSIZE (1024 * 1024)
#define TMP_BUFFER_SIZE (1024 * 8)


/* Otama Object */
typedef struct {
    PyObject_HEAD
    PyObject *config;
    otama_t *otama;
} OtamaObject;


static void
pyobj2variant(PyObject *object, otama_variant_t *var)
{
    if (PyBool_Check(object)) {
        if (Py_True == object) {
            printf("true\n");
	        otama_variant_set_int(var, 1);
        }
        else {
            printf("false\n");
	        otama_variant_set_int(var, 0);
        }
    }
    else if (Py_None == object) {
        printf("None\n");
        otama_variant_set_null(var);
    }
    else if (PyInt_Check(object)) {
        printf("int\n");
    }
    else if (PyString_Check(object)) {
        printf("string\n");
        otama_variant_set_string(var, PyString_AsString(object));
    }
    else if (PyTuple_Check(object)) {
        printf("tuple\n");
    }
    else if (PyDict_Check(object)) {
        printf("dict\n");
        otama_variant_set_hash(var);
    }
    else {
        printf("not support\n");
    }
}

static void
Otama_dealloc(OtamaObject *self)
{
    Py_XDECREF(self->config);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OtamaObject_new(PyTypeObject *type, PyObject *args)
{
    PyObject *config = NULL;
    OtamaObject *self;
    otama_t *otama = NULL;
    otama_status_t ret = OTAMA_STATUS_OK;

    if (!PyArg_ParseTuple(args, "|O", &config)) {
        return NULL;
    }

    self = (OtamaObject *)type->tp_alloc(type, 0);
    if (self) {
        Py_INCREF(Py_None);
        self->config = Py_None;

        if (config) {
            if (PyString_Check(config)) {
                ret = otama_open(&otama, PyString_AsString(config));
            }
            else if (PyDict_Check(config)) {
                otama_variant_t *var;
                otama_variant_pool_t *pool;

                pool = otama_variant_pool_alloc();
                var = otama_variant_new(pool);

                pyobj2variant(config, var);
                ret = otama_open_opt(&otama, var);

                otama_variant_pool_free(&pool);
            }
            else {
                PyErr_SetString(PyExc_TypeError, "context arg is dict.");
                return NULL;
            }
            self->otama = otama;
        }
    }

    return (PyObject *)self;
}

static int
OtamaObject_init(OtamaObject *self, PyObject *args)
{
    return 0;
}

static PyMemberDef OtamaObject_members[] = {
    {NULL}
};

static PyObject *
OtamaObject_open(OtamaObject *self, PyObject *args)
{
    PyObject *config = NULL;
    otama_status_t ret = OTAMA_STATUS_OK;
    otama_t *otama = NULL;

    if (!PyArg_ParseTuple(args, "|O", &config))
        return NULL;

    if (config) {
        if (PyString_Check(config)) {
            ret = otama_open(&otama, PyString_AsString(config));
        }
        else if (PyDict_Check(config)) {
            otama_variant_t *var;
            otama_variant_pool_t *pool;

            pool = otama_variant_pool_alloc();
            var = otama_variant_new(pool);
            pyobj2variant(config, var);
            ret = otama_open_opt(&otama, var);

            otama_variant_pool_free(&pool);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "context arg is dict.");
            return NULL;
        }
        self->otama = otama;
    }

    return Py_None;
}

static PyMethodDef OtamaObject_methods[] = {
    {"open", (PyCFunction)OtamaObject_open, METH_VARARGS | METH_CLASS,
     "open Otama"},
    {NULL, NULL, 0, NULL}
};


static PyTypeObject OtamaObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
    "otama.Otama",                              /* tp_name */
    sizeof(OtamaObject),                        /* tp_basicsize */
    0,
    (destructor)Otama_dealloc,                  /* tp_dealloc */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Otama objects",                            /* tp_doc */
    0,
    0,
    0,
    0,
    0,
    0,
    OtamaObject_methods,                        /* tp_methods */
    OtamaObject_members,                        /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    (initproc)OtamaObject_init,                 /* tp_init */
    0,
    (newfunc)OtamaObject_new,                   /* tp_new */
};


static PyMethodDef OtamaMethods[] = {
    {NULL, NULL, 0, NULL}
};

static char OtamaDoc[] = "otama Python Interface.\n";



PyMODINIT_FUNC
initotama(void)
{
    PyObject *m;

    if (PyType_Ready(&OtamaObjectType) < 0)
        return;

    m = Py_InitModule3("otama", OtamaMethods, OtamaDoc);
    if (!m)
        return;

    Py_INCREF(&OtamaObjectType);
    PyModule_AddObject(m, "Otama", (PyObject *)&OtamaObjectType);
}
