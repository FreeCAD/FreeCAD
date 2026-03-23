%{
static void
SoDraggerPythonCB(void * data, SoDragger * dragger)
{
  PyObject *func, *arglist;
  PyObject *result, *dragCB;

  dragCB = SWIG_NewPointerObj((void *) dragger, SWIGTYPE_p_SoDragger, 0);

  /* the first item in the data sequence is the python callback
   * function; the second is the supplied data python object */
  func = PyTuple_GetItem((PyObject *)data, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)data, 1), dragCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(dragCB);
  Py_XDECREF(result);
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
%extend SoDragger {
  void addStartCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->addStartCallback(SoDraggerPythonCB,
                           (void *)Py_BuildValue("(OO)", 
                                                 pyfunc, 
                                                 data ? data : Py_None));
  }

  void removeStartCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->removeStartCallback(SoDraggerPythonCB, 
                              (void *)Py_BuildValue("(OO)", 
                                                    pyfunc, 
                                                    data ? data : Py_None));
  }

  void addMotionCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->addMotionCallback(SoDraggerPythonCB,
                            (void *)Py_BuildValue("(OO)", 
                                                  pyfunc, 
                                                  data ? data : Py_None));
  }

  void removeMotionCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->removeMotionCallback(SoDraggerPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     data ? data : Py_None));
  }

  void addFinishCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->addFinishCallback(SoDraggerPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  data ? data : Py_None));
  }

  void removeFinishCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->removeFinishCallback(SoDraggerPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     data ? data : Py_None));
  }

  void addValueChangedCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->addValueChangedCallback(SoDraggerPythonCB,
                                  (void *)Py_BuildValue("(OO)",
                                                        pyfunc,
                                                        data ? data : Py_None));
  }

  void removeValueChangedCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->removeValueChangedCallback(SoDraggerPythonCB,
                                     (void *)Py_BuildValue("(OO)",
                                                           pyfunc,
                                                           data ? data : Py_None));
  }

  void addOtherEventCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->addOtherEventCallback(SoDraggerPythonCB,
                                (void *)Py_BuildValue("(OO)",
                                                      pyfunc,
                                                      data ? data : Py_None));
  }

  void removeOtherEventCallback(PyObject *pyfunc, PyObject *data = NULL) {
    self->removeOtherEventCallback(SoDraggerPythonCB,
                                   (void *)Py_BuildValue("(OO)",
                                                         pyfunc,
                                                         data ? data : Py_None));
  }
}
