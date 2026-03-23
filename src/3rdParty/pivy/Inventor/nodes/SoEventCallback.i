%{
static void
SoEventPythonCallBack(void * userdata, SoEventCallback * node)
{
  PyGILState_STATE gil = PyGILState_Ensure();
  PyObject *func, *arglist;
  PyObject *result, *evCB;

  evCB = SWIG_NewPointerObj((void *) node, SWIGTYPE_p_SoEventCallback, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)userdata, 1), evCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(evCB);
  Py_XDECREF(result);
  PyGILState_Release(gil);
}
%}

%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "need a callable object!");
    return NULL;
  }
  $1 = $input;
}

%typemap(typecheck) PyObject *pyfunc {
  $1 = PyCallable_Check($input) ? 1 : 0;
}

/* add python specific callback functions */
%extend SoEventCallback {
  PyObject* addEventCallback(SoType eventtype, 
                        PyObject *pyfunc, 
                        PyObject *userdata = NULL) {

    PyObject* tuple = Py_BuildValue("(OO)", pyfunc, userdata ? userdata : Py_None);
    Py_XINCREF(tuple);
    self->addEventCallback(eventtype, SoEventPythonCallBack, (void *)tuple);
    return tuple;
  }
  
  void removeEventCallback(SoType eventtype, 
                           PyObject *tuple) {
    self->removeEventCallback(eventtype, SoEventPythonCallBack,
                              (void *)tuple);
    Py_XDECREF(tuple);
  }
}
