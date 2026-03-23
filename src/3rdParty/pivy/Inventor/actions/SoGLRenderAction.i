%{
static void
SoGLRenderPassPythonCB(void * userdata)
{
  PyObject *func, *arglist;
  PyObject *result;

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("O", PyTuple_GetItem((PyObject *)userdata, 1));

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_XDECREF(result);
}

static SoGLRenderAction::AbortCode
SoGLRenderAbortPythonCB(void * userdata)
{
  PyObject *func, *arglist;
  PyObject *result;
  SoGLRenderAction::AbortCode res;

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("O", PyTuple_GetItem((PyObject *)userdata, 1));

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  res = (SoGLRenderAction::AbortCode)PyInt_AsLong(result);

  Py_DECREF(arglist);
  Py_XDECREF(result);

  return res;
}

static void
SoGLPreRenderPythonCB(void * userdata, class SoGLRenderAction * action)
{
  PyObject *func, *arglist;
  PyObject *result, *acCB;

  acCB = SWIG_NewPointerObj((void *) action, SWIGTYPE_p_SoGLRenderAction, 1);

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
%extend SoGLRenderAction {
  void setPassCallback(PyObject *pyfunc, PyObject * userdata){
    self->setPassCallback(SoGLRenderPassPythonCB,
                          (void *)Py_BuildValue("(OO)",
                                                pyfunc,
                                                userdata ? userdata : Py_None));
  }

  void setAbortCallback(PyObject *pyfunc, PyObject * userdata){
    self->setAbortCallback(SoGLRenderAbortPythonCB,
                           (void *)Py_BuildValue("(OO)",
                                                 pyfunc,
                                                 userdata ? userdata : Py_None));
  }
  
  void addPreRenderCallback(PyObject *pyfunc, PyObject * userdata) {
    self->addPreRenderCallback(SoGLPreRenderPythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     userdata ? userdata : Py_None));
  }

  void removePreRenderCallback(PyObject *pyfunc, PyObject * userdata) {
    self->removePreRenderCallback(SoGLPreRenderPythonCB,
                                  (void *)Py_BuildValue("(OO)",
                                                        pyfunc,
                                                        userdata ? userdata : Py_None));
  }
}

%extend SoGLRenderAction{
  static SoGLRenderAction* constructFromAction(SoAction* action)
  {
    return (SoGLRenderAction*) action;
  }
}