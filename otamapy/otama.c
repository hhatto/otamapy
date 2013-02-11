#include "Python.h"

#include <stdio.h>
#include <string.h>

#include "structmember.h"
#include "otama.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#define PyString_Check PyUnicode_Check
#define PyString_FromString PyUnicode_FromString
#define PyString_AsString PyUnicode_AS_DATA
#define OTAMAPY_INIT_ERROR return NULL
#else
#define OTAMAPY_INIT_ERROR return
#endif
#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

static PyObject *PyExc_OtamaError;
static PyTypeObject OtamaObjectType;
static void pyobj2variant(PyObject *object, otama_variant_t *var);

/* Otama Object */
typedef struct {
    PyObject_HEAD
    otama_t *otama;
} OtamaObject;


static PyObject *
variant2pyobj(otama_variant_t *var)
{
    switch (otama_variant_type(var)) {
        case OTAMA_VARIANT_TYPE_INT:
            return PyLong_FromLong(otama_variant_to_int(var));
        case OTAMA_VARIANT_TYPE_FLOAT:
            return PyFloat_FromDouble(otama_variant_to_float(var));
	    case OTAMA_VARIANT_TYPE_STRING:
            return PyString_FromString(otama_variant_to_string(var));
	    case OTAMA_VARIANT_TYPE_ARRAY: {
		    long count = otama_variant_array_count(var);
            int i;
            PyObject *tuple = PyTuple_New(count);
            for (i = 0; i < count; ++i) {
                PyObject *_value = variant2pyobj(otama_variant_array_at(var, i));
                PyTuple_SetItem(tuple, i,_value);
                Py_DECREF(_value);
            }
            return tuple;
        }
	    case OTAMA_VARIANT_TYPE_HASH: {
		    otama_variant_t *keys = otama_variant_hash_keys(var);
		    long count = otama_variant_array_count(keys);
		    int i;
            PyObject *dict = PyDict_New();
            for (i = 0; i < count; ++i) {
                PyObject *_value = variant2pyobj(otama_variant_hash_at2(var, otama_variant_array_at(keys, i)));
                PyDict_SetItemString(dict,
                        otama_variant_to_string(otama_variant_array_at(keys, i)),
                        _value);
                Py_DECREF(_value);
            }
            return dict;
        }
        case OTAMA_VARIANT_TYPE_NULL:
        default:
            printf("not implementation");
            break;
    }

    Py_RETURN_NONE;
}

static void
pyobj2variant_pair(PyObject *key, PyObject *value, otama_variant_t *var)
{
    pyobj2variant(value, otama_variant_hash_at(var, PyString_AsString(key)));
}

static void
pyobj2variant(PyObject *object, otama_variant_t *var)
{
    if (PyBool_Check(object)) {
        if (Py_True == object) {
	        otama_variant_set_int(var, 1);
        }
        else {
	        otama_variant_set_int(var, 0);
        }
    }
    else if (Py_None == object) {
        otama_variant_set_null(var);
    }
    else if (PyFloat_Check(object)) {
        otama_variant_set_float(var, PyFloat_AsDouble(object));
    }
    else if (PyLong_Check(object)) {
        otama_variant_set_int(var, PyLong_AsLong(object));
    }
    else if (PyString_Check(object)) {
        otama_variant_set_string(var, PyString_AsString(object));
    }
    else if (PyTuple_Check(object)) {
        printf("tuple\n");
    }
    else if (PyDict_Check(object)) {
        otama_variant_set_hash(var);
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(object, &pos, &key, &value)) {
            PyObject *_size = PyLong_FromSsize_t(pos);
            long i = PyLong_AsLong(_size);
            Py_XDECREF(_size);
            if (i == -1) break;

            pyobj2variant_pair(key, value, var);
        }
    }
    else {
        printf("%s not support\n", __FUNCTION__);
    }
}

static PyObject *
make_results(const otama_result_t *results)
{
    PyObject *result_tuple;
    long num = otama_result_count(results);
    int i;

    result_tuple = PyTuple_New(num);
    for (i = 0; i < num; ++i) {
		otama_variant_t *value = otama_result_value(results, i);
        char hexid[OTAMA_ID_HEXSTR_LEN];

		otama_id_bin2hexstr(hexid, otama_result_id(results, i));
        PyObject *_result = variant2pyobj(value);   // return new dict
        PyObject *_hexid = PyString_FromString(hexid);
        PyDict_SetItemString(_result, "id", _hexid);
        // TODO: error handle

        PyTuple_SetItem(result_tuple, i, _result);

        Py_XDECREF(_hexid);
    }

    return result_tuple;
}

static void
otamapy_raise(otama_status_t ret)
{
    switch (ret) {
        case OTAMA_STATUS_OK:
            break;
        case OTAMA_STATUS_NODATA:
	    case OTAMA_STATUS_INVALID_ARGUMENTS:
	    case OTAMA_STATUS_ASSERTION_FAILURE:
	    case OTAMA_STATUS_SYSERROR:
	    case OTAMA_STATUS_NOT_IMPLEMENTED:
            PyErr_SetString(PyExc_OtamaError, otama_status_message(ret));
            break;
        default:
            PyErr_SetString(PyExc_OtamaError, "Unknown Error");
            break;
    }
}

static void
Otama_dealloc(OtamaObject *self)
{
    if (self->otama) {
        otama_close(&(self->otama));
        self->otama = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OtamaObject_new(PyTypeObject *type, PyObject *args)
{
    PyObject *config = NULL;
    OtamaObject *self;
    otama_status_t ret = OTAMA_STATUS_OK;

    if (!PyArg_ParseTuple(args, "|O", &config)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    self = (OtamaObject *)type->tp_alloc(type, 0);
    if (self) {
        if (config) {
            if (PyString_Check(config)) {
                ret = otama_open(&self->otama, PyString_AsString(config));
            }
            else if (PyDict_Check(config)) {
                otama_variant_t *var;
                otama_variant_pool_t *pool;

                pool = otama_variant_pool_alloc();
                var = otama_variant_new(pool);

                pyobj2variant(config, var);
                otama_open_opt(&self->otama, var);

                otama_variant_pool_free(&pool);
            }
            else {
                PyErr_SetString(PyExc_TypeError, "not support type.");
                return NULL;
            }

            if (ret != OTAMA_STATUS_OK) {
                otamapy_raise(ret);
                return NULL;
            }
        }
    }

    return (PyObject *)self;
}

static int
OtamaObject_init(OtamaObject *self, PyObject *args)
{
    return 0;
}

static PyObject *
OtamaObject_open(OtamaObject *self, PyObject *args)
{
    PyObject *config = NULL;
    otama_status_t ret = OTAMA_STATUS_OK;

    if (!PyArg_ParseTuple(args, "|O", &config)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (!self) {
        self = (OtamaObject *)OtamaObject_new(&OtamaObjectType, args);
    }

    if (config) {
        if (PyString_Check(config)) {
            ret = otama_open(&self->otama, PyString_AsString(config));
        }
        else if (PyDict_Check(config)) {
            otama_variant_t *var;
            otama_variant_pool_t *pool;

            pool = otama_variant_pool_alloc();
            var = otama_variant_new(pool);
            pyobj2variant(config, var);
            otama_open_opt(&self->otama, var);

            otama_variant_pool_free(&pool);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "not support type.");
            return NULL;
        }

        if (ret != OTAMA_STATUS_OK) {
            otamapy_raise(ret);
            return NULL;
        }
    }

    return (PyObject *)self;
}

static PyObject *
OtamaObject_close(OtamaObject *self)
{
    if (self->otama) {
        otama_close(&(self->otama));
        self->otama = NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
OtamaObject_pull(OtamaObject *self)
{
	otama_status_t ret;

    ret = otama_pull(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
OtamaObject_create_table(OtamaObject *self)
{
    otama_status_t ret;

    ret = otama_create_table(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}
static PyObject *
OtamaObject_drop_table(OtamaObject *self)
{
    otama_status_t ret;

    ret = otama_drop_table(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
OtamaObject_search(OtamaObject *self, PyObject *args)
{
    int num;
	otama_status_t ret;
	otama_result_t *results = NULL;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
    PyObject *data;
    PyObject *result_tuple;

    if (!PyArg_ParseTuple(args, "iO", &num, &data)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);
    pyobj2variant(data, var);

    if (PyString_Check(data)) {
        const char *_tmp = PyString_AsString(data);
        ret = otama_search_file(self->otama, &results, num, _tmp);
    }
    else {
        // TODO: not implementation
        ret = otama_search(self->otama, &results, num, var);
    }

    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }
    result_tuple = make_results(results);

	otama_result_free(&results);
	otama_variant_pool_free(&pool);

    return result_tuple;
}

static PyObject *
OtamaObject_insert(OtamaObject *self, PyObject *args)
{
	char hexid[OTAMA_ID_HEXSTR_LEN];
	otama_id_t id;
	otama_status_t ret;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
    PyObject *data;
    PyObject *pyobj_id;

    if (!PyArg_ParseTuple(args, "O", &data)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);
	pyobj2variant(data, var);

    if (PyString_Check(data)) {
        const char *_tmp = PyString_AsString(data);
        ret = otama_insert_file(self->otama, &id, _tmp);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "not support type");
        return NULL;
    }

    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }

	otama_id_bin2hexstr(hexid, &id);
	otama_variant_pool_free(&pool);

    pyobj_id = Py_BuildValue("s", hexid);
    return pyobj_id;
}

static PyObject *
OtamaObject_remove(OtamaObject *self, PyObject *args)
{
    PyObject *id;
    otama_status_t ret;
    otama_id_t remove_id;

    if (!PyArg_ParseTuple(args, "O", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    ret = otama_id_hexstr2bin(&remove_id, PyString_AsString(id));
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    ret = otama_remove(self->otama, &remove_id);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef OtamaObject_methods[] = {
    {"open", (PyCFunction)OtamaObject_open, METH_VARARGS|METH_STATIC,
     "open Otama"},
    {"close", (PyCFunction)OtamaObject_close, METH_NOARGS,
     "close Otama Object"},
    {"pull", (PyCFunction)OtamaObject_pull, METH_NOARGS,
     "pull to Otama DataBase"},
    {"create_table", (PyCFunction)OtamaObject_create_table, METH_NOARGS,
     "create Otama DataBase Table"},
    {"drop_table", (PyCFunction)OtamaObject_drop_table, METH_NOARGS,
     "drop to Otama DataBase Table"},
    {"insert", (PyCFunction)OtamaObject_insert, METH_VARARGS,
     "insert image data"},
    {"remove", (PyCFunction)OtamaObject_remove, METH_VARARGS,
     "remove id from Otama DataBase"},
    {"search", (PyCFunction)OtamaObject_search, METH_VARARGS,
     "search from Otama DataBase"},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef OtamaObject_members[] = {
    {NULL}
};


static PyTypeObject OtamaObjectType = {
#ifdef PY3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
#endif
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
    0,                                          /* tp_alloc */
    (newfunc)OtamaObject_new,                   /* tp_new */
};


static PyMethodDef OtamaMethods[] = {
    {NULL, NULL, 0, NULL}
};

static char OtamaDoc[] = "otama Python Interface.\n";


#ifdef PY3
static struct PyModuleDef OtamaModuleDef = {
    PyModuleDef_HEAD_INIT,
    "otamapy",
    OtamaDoc,
    -1,
    OtamaMethods,
};
PyObject *
PyInit_otama(void)

#else

PyMODINIT_FUNC
initotama(void)
#endif
{
    PyObject *module;

#ifdef PY3
    module = PyModule_Create(&OtamaModuleDef);
#else
    module = Py_InitModule3("otama", OtamaMethods, OtamaDoc);
    if (!module)
        return;
#endif

    if (PyModule_AddStringConstant(module,
                                   "__libotama_version__",
                                   otama_version_string()) < 0)
        OTAMAPY_INIT_ERROR;

    if (PyType_Ready(&OtamaObjectType) < 0)
        OTAMAPY_INIT_ERROR;

    PyExc_OtamaError = PyErr_NewException("otama.error", NULL, NULL);

    Py_INCREF(&OtamaObjectType);
    PyModule_AddObject(module, "Otama", (PyObject *)&OtamaObjectType);

    Py_INCREF(PyExc_OtamaError);
    PyModule_AddObject(module, "error", (PyObject *)&OtamaObjectType);

#ifdef PY3
    return module;
#endif
}
