// to support the py limited api, we need to define some functions that are not 
// available in the limited api.

#include <Python.h>

inline const char *PyUnicode_AsUTF8(PyObject *unicode)
{
    Py_ssize_t sz = 0;
    return PyUnicode_AsUTF8AndSize(unicode, &sz);
}

PyObject *
PyRun_String(const char *str, int start, PyObject *globals, PyObject *locals)
{
    PyObject *code = Py_CompileString(str, "pyscript", start);
    PyObject *ret = nullptr;

    if (code != nullptr) {
        ret = PyEval_EvalCode(code, globals, locals);
    }
    Py_XDECREF(code);
    return ret;
}
