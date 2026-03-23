%{
static void
SoSelectionPathPythonCB(void * data, SoPath * path)
{
  PyGILState_STATE gil = PyGILState_Ensure();
  PyObject *func, *arglist;
  PyObject *result, *pathCB;

  pathCB = SWIG_NewPointerObj((void *) path, SWIGTYPE_p_SoPath, 0);

  /* the first item in the data sequence is the python callback
   * function; the second is the supplied data python object */
  func = PyTuple_GetItem((PyObject *)data, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)data, 1), pathCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(pathCB);
  Py_XDECREF(result);
  PyGILState_Release(gil);
}

static void
SoSelectionClassPythonCB(void * data, SoSelection * sel)
{
  PyObject *func, *arglist;
  PyObject *result, *selCB;

  selCB = SWIG_NewPointerObj((void *) sel, SWIGTYPE_p_SoSelection, 0);

  /* the first item in the data sequence is the python callback
   * function; the second is the supplied data python object */
  func = PyTuple_GetItem((PyObject *)data, 0);
  arglist = Py_BuildValue("OO", PyTuple_GetItem((PyObject *)data, 1), selCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(selCB);
  Py_XDECREF(result);
}

static SoPath *
SoSelectionPickPythonCB(void * data, const SoPickedPoint * pick)
{
  PyObject *func, *arglist;
  PyObject *result, *pickCB;
  SoPath *resultobj;

  pickCB = SWIG_NewPointerObj((void *) pick, SWIGTYPE_p_SoPickedPoint, 0);

  /* the first item in the data sequence is the python callback
   * function; the second is the supplied data python object */
  func = PyTuple_GetItem((PyObject *)data, 0);
  arglist = Py_BuildValue("OO", PyTuple_GetItem((PyObject *)data, 1), pickCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }
  else {
    SWIG_ConvertPtr(result, (void **) &resultobj, SWIGTYPE_p_SoPath, 1);
  }

  Py_DECREF(arglist);
  Py_DECREF(pickCB);
  Py_XDECREF(result);

  return resultobj;
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
%extend SoSelection {
  void addSelectionCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->addSelectionCallback(SoSelectionPathPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     userdata ? userdata : Py_None));
  }

  void removeSelectionCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->removeSelectionCallback(SoSelectionPathPythonCB,
                                  (void *)Py_BuildValue("(OO)",
                                                        pyfunc,
                                                        userdata ? userdata : Py_None));
  }

  void addDeselectionCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->addDeselectionCallback(SoSelectionPathPythonCB,
                                 (void *)Py_BuildValue("(OO)",
                                                       pyfunc,
                                                       userdata ? userdata : Py_None));
  }

  void removeDeselectionCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->removeDeselectionCallback(SoSelectionPathPythonCB,
                                    (void *)Py_BuildValue("(OO)",
                                                          pyfunc,
                                                          userdata ? userdata : Py_None));
  }

  void addStartCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->addStartCallback(SoSelectionClassPythonCB,
                           (void *)Py_BuildValue("(OO)",
                                                 pyfunc,
                                                 userdata ? userdata : Py_None));
  }

  void removeStartCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->removeStartCallback(SoSelectionClassPythonCB,
                              (void *)Py_BuildValue("(OO)",
                                                    pyfunc,
                                                    userdata ? userdata : Py_None));
  }

  void addFinishCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->addFinishCallback(SoSelectionClassPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  userdata ? userdata : Py_None));
  }

  void removeFinishCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->removeFinishCallback(SoSelectionClassPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     userdata ? userdata : Py_None));
  }

  void setPickFilterCallback(PyObject *pyfunc, PyObject *userdata = NULL, int callOnlyIfSelectable = 1) {
    self->setPickFilterCallback(SoSelectionPickPythonCB,
                                (void *)Py_BuildValue("(OO)",
                                                      pyfunc,
                                                      userdata ? userdata : Py_None),
                                callOnlyIfSelectable);
  }

  void addChangeCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->addChangeCallback(SoSelectionClassPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  userdata ? userdata : Py_None));
  }

  void removeChangeCallback(PyObject *pyfunc, PyObject *userdata = NULL) {
    self->removeChangeCallback(SoSelectionClassPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     userdata ? userdata : Py_None));
  }
}
