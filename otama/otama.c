#include "Python.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "structmember.h"
#include "otama.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#define PyString_Check PyBytes_Check
#define PyString_GET_SIZE PyUnicode_GET_SIZE
#define PyString_FromString PyUnicode_FromString
#define PyString_AsString PyBytes_AsString
#define OTAMAPY_INIT_ERROR return NULL
#else
#define OTAMAPY_INIT_ERROR return
#endif
#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

static PyObject *PyExc_OtamaError;
static PyTypeObject OtamaObjectType;
static PyTypeObject OtamaFeatureRawObjectType;
static void pyobj2variant(PyObject *object, otama_variant_t *var);

/* Otama Object */
typedef struct {
    PyObject_HEAD
    otama_t *otama;
} OtamaObject;

typedef struct {
    OtamaObject base;
    otama_feature_raw_t *raw;
} OtamaFeatureRawObject;


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
                PyTuple_SetItem(tuple, i, _value);
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
            Py_RETURN_NONE;
        default:
            printf("not implementation\n");
            break;
    }

    Py_RETURN_NONE;
}

static void
pyobj2variant_pair(PyObject *key, PyObject *value, otama_variant_t *var)
{
    const char *key_string;
#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(key);
    key_string = PyBytes_AsString(utf8_item);
#else
    key_string = PyString_AsString(key);
#endif
    pyobj2variant(value, otama_variant_hash_at(var, key_string));
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
        if (strlen(PyString_AsString(object)) == PyString_GET_SIZE(object)) {
            otama_variant_set_string(var, PyString_AsString(object));
        }
        else {
            otama_variant_set_binary(var, PyString_AsString(object),
                                     PyString_GET_SIZE(object));
        }
    }
    else if (PyUnicode_Check(object)) {
#ifdef PY3
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            PyErr_SetString(PyExc_OtamaError, "don't gen utf8 item");
            return NULL;
        }

        if (strlen(PyBytes_AsString(utf8_item)) == PyUnicode_GET_SIZE(object)) {
            const char *_tmp = PyBytes_AsString(utf8_item);
            otama_variant_set_string(var, _tmp);
        }
        else {
            // TODO: not need?
            otama_variant_set_binary(var, PyBytes_AsString(object),
                                     PyString_GET_SIZE(object));
        }

        Py_XDECREF(utf8_item);
#else
        otama_variant_set_string(var, PyBytes_AsString(object));
#endif
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;
        otama_variant_set_array(var);
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyTuple_GetItem(object, i);
            pyobj2variant(elm, otama_variant_array_at(var, i));
        }
    }
    else if (PyList_Check(object)) {
        int len = PyList_Size(object), i;
        otama_variant_set_array(var);
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            pyobj2variant(elm, otama_variant_array_at(var, i));
        }
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
        if (PyObject_IsInstance(object, (PyObject *)&OtamaFeatureRawObjectType)) {
            otama_variant_set_pointer(var, ((OtamaFeatureRawObject *)object)->raw);
        }
        else {
            otama_variant_set_null(var);
        }
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

/*
 * @return PyObject *self or NULL
 */
static PyObject *
setup_config(OtamaObject *self, PyObject *config)
{
    otama_status_t ret = OTAMA_STATUS_OK;

    if (config) {
        if (PyString_Check(config)) {
            ret = otama_open(&self->otama, PyString_AsString(config));
        }
        else if (PyUnicode_Check(config)) {
            PyObject *utf8_item;
            utf8_item = PyUnicode_AsUTF8String(config);
            if (!utf8_item) {
                PyErr_SetString(PyExc_OtamaError, "don't gen utf8 item");
                return NULL;
            }
            ret = otama_open(&self->otama, PyBytes_AsString(utf8_item));
            Py_XDECREF(utf8_item);
        }
        else if (PyDict_Check(config)) {
            otama_variant_t *var;
            otama_variant_pool_t *pool;

            pool = otama_variant_pool_alloc();
            var = otama_variant_new(pool);

            pyobj2variant(config, var);
            ret = otama_open_opt(&self->otama, var);

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

    if (!PyArg_ParseTuple(args, "|O", &config)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    self = (OtamaObject *)type->tp_alloc(type, 0);
    if (self) {
        if (!setup_config(self, config)) {
            return NULL;
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

    if (!PyArg_ParseTuple(args, "|O", &config)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (!self) {
        self = (OtamaObject *)OtamaObject_new(&OtamaObjectType, args);
        if (!self) {
            return NULL;
        }
    }

    return setup_config(self, config);
}

static PyObject *
OtamaObject_close(OtamaObject *self)
{
    if (self->otama) {
        otama_close(&self->otama);
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
OtamaObject_create_database(OtamaObject *self)
{
    otama_status_t ret;

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_create_database(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}
static PyObject *
OtamaObject_drop_database(OtamaObject *self)
{
    otama_status_t ret;

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_drop_database(self->otama);
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

    fprintf(stderr, "This API is deprecated, rename to create_database\n");

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_create_database(self->otama);
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

    fprintf(stderr, "This API is deprecated, rename to drop_database\n");

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_drop_database(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
OtamaObject_drop_index(OtamaObject *self)
{
    otama_status_t ret;

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_drop_index(self->otama);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
OtamaObject_vacuum_index(OtamaObject *self)
{
    otama_status_t ret;

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    ret = otama_vacuum_index(self->otama);
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

    if (!self->otama) {
        PyErr_SetString(PyExc_OtamaError, "not initialize/config error");
        return NULL;
    }

    pool = otama_variant_pool_alloc();
    var = otama_variant_new(pool);
    pyobj2variant(data, var);

    if (PyString_Check(data)) {
        const char *_tmp = PyString_AsString(data);
        char _err_tmp[120] = "not exist file ";
        struct stat st;
        if (stat(_tmp, &st)) {
            otama_variant_pool_free(&pool);
            strcat(_err_tmp, _tmp);
            PyErr_SetString(PyExc_IOError, _err_tmp);
            return NULL;
        }
        ret = otama_search_file(self->otama, &results, num, _tmp);
    }
    else if (PyUnicode_Check(data)) {
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(data);
        if (!utf8_item) {
            PyErr_SetString(PyExc_OtamaError, "don't gen utf8 item");
            return NULL;
        }
        const char *_tmp = PyBytes_AsString(utf8_item);
        char _err_tmp[120] = "not exist file ";
        struct stat st;
        if (stat(_tmp, &st)) {
            otama_variant_pool_free(&pool);
            strcat(_err_tmp, _tmp);
            PyErr_SetString(PyExc_IOError, _err_tmp);
            return NULL;
        }
        ret = otama_search_file(self->otama, &results, num, _tmp);
        Py_XDECREF(utf8_item);
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
OtamaObject_similarity(OtamaObject *self, PyObject *args)
{
    PyObject *data1, *data2;
    otama_status_t ret;
    otama_variant_pool_t *pool;
    otama_variant_t *var1, *var2;
    float similarity = 0.0f;

    if (!PyArg_ParseTuple(args, "OO", &data1, &data2)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (!(PyDict_Check(data1) && PyDict_Check(data2))) {
        PyErr_SetString(PyExc_OtamaError, "invalid argument type");
        return NULL;
    }

    pool = otama_variant_pool_alloc();
    var1 = otama_variant_new(pool);
    var2 = otama_variant_new(pool);

    pyobj2variant(data1, var1);
    pyobj2variant(data2, var2);

    ret = otama_similarity(self->otama, &similarity, var1, var2);
    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }
    otama_variant_pool_free(&pool);

    return PyFloat_FromDouble(similarity);
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
    else if (PyUnicode_Check(data)) {
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(data);
        if (!utf8_item) {
            PyErr_SetString(PyExc_OtamaError, "don't gen utf8 item");
            return NULL;
        }
        const char *_tmp = PyBytes_AsString(utf8_item);
        ret = otama_insert_file(self->otama, &id, _tmp);
        Py_XDECREF(utf8_item);
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

static PyObject *
OtamaObject_exists(OtamaObject *self, PyObject *args)
{
    PyObject *id;
    otama_status_t ret;
    otama_id_t otama_id;
    int result = 0;

    if (!PyArg_ParseTuple(args, "O", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(id);
    if (!utf8_item) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }
    ret = otama_id_hexstr2bin(&otama_id, PyBytes_AsString(utf8_item));
    Py_XDECREF(utf8_item);
#else
    char *hexstr = PyString_AsString(id);
    if (!hexstr) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }
    ret = otama_id_hexstr2bin(&otama_id, hexstr);
#endif
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    ret = otama_exists(self->otama, &result, &otama_id);
    if (ret != OTAMA_STATUS_OK) {
        otamapy_raise(ret);
        return NULL;
    }

    return result == 0 ? PyBool_FromLong(0) : PyBool_FromLong(1);
}

static PyObject *
OtamaObject_invoke(OtamaObject *self, PyObject *args)
{
    const char *_tmp_method;
    PyObject *output, *method, *input;
    otama_status_t ret;
    otama_variant_pool_t *pool = otama_variant_pool_alloc();
    otama_variant_t *input_var = otama_variant_new(pool);
    otama_variant_t *output_var = otama_variant_new(pool);

    if (!PyArg_ParseTuple(args, "OO", &method, &input)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (PyString_Check(method)) {
        _tmp_method = PyString_AsString(method);
    }
    else if (PyUnicode_Check(method)) {
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(method);
        if (!utf8_item) {
            PyErr_SetString(PyExc_OtamaError, "don't gen utf8 item");
            return NULL;
        }
        _tmp_method = PyBytes_AsString(utf8_item);
        Py_XDECREF(utf8_item);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "not support type");
        return NULL;
    }

    pyobj2variant(input, input_var);

    ret = otama_invoke(self->otama, _tmp_method, output_var, input_var);
    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }

    output = variant2pyobj(output_var);
    otama_variant_pool_free(&pool);

    return output;
}

static PyObject *
OtamaObject_feature_raw(OtamaObject *self, PyObject *args)
{
    otama_status_t ret;
    otama_feature_raw_t *raw;
    PyObject *pyraw, *query;
    otama_variant_pool_t *pool;
    otama_variant_t *var;

    if (!PyArg_ParseTuple(args, "O", &query)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (!PyDict_Check(query)) {
        PyErr_SetString(PyExc_TypeError, "invalid argument");
        return NULL;
    }

    pool = otama_variant_pool_alloc();
    var = otama_variant_new(pool);

    pyobj2variant(query, var);

    ret = otama_feature_raw(self->otama, &raw, var);
    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }
    otama_variant_pool_free(&pool);

    pyraw = PyType_GenericNew(&OtamaFeatureRawObjectType, NULL, NULL);
    ((OtamaFeatureRawObject *)pyraw)->raw = raw;

    return pyraw;
}

static PyObject *
OtamaObject_feature_string(OtamaObject *self, PyObject *args)
{
    otama_status_t ret;
    PyObject *pystr, *query;
    otama_variant_pool_t *pool;
    otama_variant_t *var;
    char *feature_string = NULL;

    if (!PyArg_ParseTuple(args, "O", &query)) {
        PyErr_SetString(PyExc_TypeError, "argument error");
        return NULL;
    }

    if (!PyDict_Check(query)) {
        PyErr_SetString(PyExc_TypeError, "invalid argument");
        return NULL;
    }

    pool = otama_variant_pool_alloc();
    var = otama_variant_new(pool);

    pyobj2variant(query, var);

    ret = otama_feature_string(self->otama, &feature_string, var);
    if (ret != OTAMA_STATUS_OK) {
        otama_variant_pool_free(&pool);
        otamapy_raise(ret);
        return NULL;
    }
    otama_variant_pool_free(&pool);

    pystr = PyString_FromString(feature_string);
    // TODO: error handling

    return pystr;
}

static PyObject *
OtamaFeatureRawObject_dispose(OtamaFeatureRawObject *self)
{
    otama_feature_raw_free(&self->raw);
    self->raw = NULL;

    Py_RETURN_NONE;
}

static PyMethodDef OtamaObject_methods[] = {
    {"open", (PyCFunction)OtamaObject_open, METH_VARARGS|METH_STATIC,
     "open Otama"},
    {"close", (PyCFunction)OtamaObject_close, METH_NOARGS,
     "close Otama Object"},
    {"pull", (PyCFunction)OtamaObject_pull, METH_NOARGS,
     "pull to Otama Database"},
    {"create_database", (PyCFunction)OtamaObject_create_database, METH_NOARGS,
     "create Otama Database Table"},
    {"drop_database", (PyCFunction)OtamaObject_drop_database, METH_NOARGS,
     "drop to Otama Database Table"},
    {"create_table", (PyCFunction)OtamaObject_create_table, METH_NOARGS,
     "create Otama Database Table (deprecated)"},
    {"drop_table", (PyCFunction)OtamaObject_drop_table, METH_NOARGS,
     "drop to Otama Database Table (deprecated)"},
    {"drop_index", (PyCFunction)OtamaObject_drop_index, METH_NOARGS,
     "drop to Otama Database Index"},
    {"vacuum_index", (PyCFunction)OtamaObject_vacuum_index, METH_NOARGS,
     "vacuum to Otama Database Index"},
    {"insert", (PyCFunction)OtamaObject_insert, METH_VARARGS,
     "insert image data"},
    {"remove", (PyCFunction)OtamaObject_remove, METH_VARARGS,
     "remove id from Otama Database"},
    {"search", (PyCFunction)OtamaObject_search, METH_VARARGS,
     "search from Otama Database"},
    {"similarity", (PyCFunction)OtamaObject_similarity, METH_VARARGS,
     "check similarity"},
    {"exists", (PyCFunction)OtamaObject_exists, METH_VARARGS,
     "exist image in Otama Database"},
    {"feature_string", (PyCFunction)OtamaObject_feature_string, METH_VARARGS,
     "return feature string value"},
    {"feature_raw", (PyCFunction)OtamaObject_feature_raw, METH_VARARGS,
     "return feature raw value"},
    {"invoke", (PyCFunction)OtamaObject_invoke, METH_VARARGS,
     "invoke Database driver"},
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

static PyMethodDef OtamaFeatureRawObject_methods[] = {
    {"dispose", (PyCFunction)OtamaFeatureRawObject_dispose, METH_NOARGS,
     "free resource"},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef OtamaFeatureRawObject_members[] = {
    {NULL}
};

static PyTypeObject OtamaFeatureRawObjectType = {
#ifdef PY3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
#endif
    "otama.OtamaFeatureRaw",                    /* tp_name */
    sizeof(OtamaFeatureRawObject),              /* tp_basicsize */
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
    "OtamaFeatureRaw objects",                  /* tp_doc */
    0,
    0,
    0,
    0,
    0,
    0,
    OtamaFeatureRawObject_methods,              /* tp_methods */
    OtamaFeatureRawObject_members,              /* tp_members */
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

PyMODINIT_FUNC
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

    if (PyType_Ready(&OtamaFeatureRawObjectType) < 0)
        OTAMAPY_INIT_ERROR;

    PyExc_OtamaError = PyErr_NewException("otama.OtamaError", NULL, NULL);

    Py_INCREF(&OtamaObjectType);
    PyModule_AddObject(module, "Otama", (PyObject *)&OtamaObjectType);

    Py_INCREF(&OtamaFeatureRawObjectType);
    PyModule_AddObject(module, "OtamaFeatureRaw", (PyObject *)&OtamaFeatureRawObjectType);

    Py_INCREF(PyExc_OtamaError);
    PyModule_AddObject(module, "error", (PyObject *)&OtamaObjectType);

    otama_log_set_level(OTAMA_LOG_LEVEL_ERROR);
#ifdef PY3
    return module;
#endif
}
