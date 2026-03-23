%{
static void
SoSceneManagerPythonCB(void * userdata, SoSceneManager * mgr)
{
  PyObject *func, *arglist;
  PyObject *result, *mgrCB;

  mgrCB = SWIG_NewPointerObj((void *)mgr, SWIGTYPE_p_SoSceneManager, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("OO", PyTuple_GetItem((PyObject *)userdata, 1), mgrCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }  

  Py_DECREF(arglist);
  Py_DECREF(mgrCB);
  Py_XDECREF(result);
}
%}

/* add python specific callback functions */
%extend SoSceneManager {
  void setRenderCallback(PyObject * pyfunc, PyObject * userData = NULL) {
    self->setRenderCallback(SoSceneManagerPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  userData ? userData : Py_None));
  }
}
