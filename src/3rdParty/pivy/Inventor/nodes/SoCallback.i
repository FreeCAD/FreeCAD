%{
static void
SoPythonCallBack(void * userdata, SoAction * action)
{
  PyGILState_STATE gil = PyGILState_Ensure();
  PyObject *func, *arglist;
  PyObject *result, *acCB;

  acCB = SWIG_NewPointerObj((void *) action, SWIGTYPE_p_SoAction, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)userdata, 1), acCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(acCB);
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
%extend SoCallback {
  void setCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->setCallback(SoPythonCallBack, 
                      (void *)Py_BuildValue("(OO)", 
                                            pyfunc, 
                                            userdata ? userdata : Py_None));
  }
}
