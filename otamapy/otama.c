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
    printf("%d\n", PyType_Check(object));
    //switch (PyType_Check(object)) {
    //    case 0: // FIXME
    //        otama_variant_set_null(var);
    //        printf("hoge\n");
    //        break;
    //    case PyBool_Type:
    //        if (PyBool_Check(object)) {
	//	        otama_variant_set_int(var, 1);
    //        }
    //        else {
	//	        otama_variant_set_int(var, 0);
    //        }
    //    default:
    //        printf("other\n");
    //        break;
    //}
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
    OtamaObject *self;
    otama_t *otama = NULL;
    otama_status_t ret = OTAMA_STATUS_OK;

    self = (OtamaObject *)type->tp_alloc(type, 0);
    if (self) {
        Py_INCREF(Py_None);
        self->config = Py_None;

        otama_variant_t *var;
	    otama_variant_pool_t *pool;

		pool = otama_variant_pool_alloc();
        var = otama_variant_new(pool);
        pyobj2variant(args, var);
        ret = otama_open_opt(&otama, var);
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

    if (!PyArg_ParseTuple(args, "O", &config))
        return NULL;

    if (config) {
        if (!PyDict_Check(config)) {
            PyErr_SetString(PyExc_TypeError, "context arg is dict.");
            return NULL;
        }
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
