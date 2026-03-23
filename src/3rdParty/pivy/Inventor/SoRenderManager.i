%{
static void
SoRenderManagerRenderPythonCB(void * userdata, class SoRenderManager * mgr)
{
  PyObject *func, *arglist;
  PyObject *result, *mgrCB;

  mgrCB = SWIG_NewPointerObj((void *)mgr, SWIGTYPE_p_SoRenderManager, 0);

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
%extend SoRenderManager {
  void setRenderCallback(PyObject * pyfunc, PyObject * userData = NULL) {
    self->setRenderCallback(SoRenderManagerRenderPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  userData ? userData : Py_None));
  }

#if 0
  static void nodesensorCB(void * data, SoSensor * sensor);
  static void prerendercb(void * userdata, SoGLRenderAction * action);
#endif

  void addPreRenderCallback(PyObject * pyfunc, PyObject * data) {
    self->addPreRenderCallback(SoRenderManagerRenderPythonCB,
			       (void *)Py_BuildValue("(OO)",
						     pyfunc,
						     data ? data : Py_None));
  }

  void removePreRenderCallback(PyObject * pyfunc, PyObject * data) {
    self->removePreRenderCallback(SoRenderManagerRenderPythonCB,
				  (void *)Py_BuildValue("(OO)",
							pyfunc,
							data ? data : Py_None));
  }

  void addPostRenderCallback(PyObject * pyfunc, PyObject * data) {
    self->addPostRenderCallback(SoRenderManagerRenderPythonCB,
				(void *)Py_BuildValue("(OO)",
						      pyfunc,
						      data ? data : Py_None));
  }

  void removePostRenderCallback(PyObject * pyfunc, PyObject * data) {
    self->removePostRenderCallback(SoRenderManagerRenderPythonCB,
				   (void *)Py_BuildValue("(OO)",
							 pyfunc,
							 data ? data : Py_None));
  }
}
