%{
static SoCallbackAction::Response
SoIntersectionVisitationPythonCB(void * closure, 
                                 const SoPath * where)
{
  PyObject *func, *arglist;
  PyObject *result, *path;
  int iresult = 0;

  path = SWIG_NewPointerObj((void *) where, SWIGTYPE_p_SoPath, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)closure, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)closure, 1), path);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }
  else {
    iresult = PyInt_AsLong(result);
  }
  
  Py_DECREF(arglist);
  Py_DECREF(path);
  Py_XDECREF(result);

  return (SoCallbackAction::Response)iresult;
}

static SbBool
SoIntersectionFilterPythonCB(void * closure,
                             const SoPath * p1,
                             const SoPath * p2)
{   
  PyObject *func, *arglist;
  PyObject *result, *path1, *path2;
  int iresult = 0;

  path1 = SWIG_NewPointerObj((void *) p1, SWIGTYPE_p_SoPath, 0);
  path2 = SWIG_NewPointerObj((void *) p2, SWIGTYPE_p_SoPath, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)closure, 0);
  arglist = Py_BuildValue("(OOO)", PyTuple_GetItem((PyObject *)closure, 1), path1, path2);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }
  else {
    iresult = PyInt_AsLong(result);
  }
  
  Py_DECREF(arglist);
  Py_DECREF(path1);
  Py_DECREF(path2);
  Py_XDECREF(result);

  return (SbBool)iresult;
}

static SoIntersectionDetectionAction::Resp
SoIntersectionPythonCB(void * closure, 
                       const SoIntersectingPrimitive * p1, 
                       const SoIntersectingPrimitive * p2)
{
  PyObject *func, *arglist;
  PyObject *result, *primitive1, *primitive2;
  int iresult = 0;

  primitive1 = SWIG_NewPointerObj((void *) p1, SWIGTYPE_p_SoIntersectingPrimitive, 0);
  primitive2 = SWIG_NewPointerObj((void *) p2, SWIGTYPE_p_SoIntersectingPrimitive, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)closure, 0);
  arglist = Py_BuildValue("(OOO)", PyTuple_GetItem((PyObject *)closure, 1), primitive1, primitive2);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }
  else {
    iresult = PyInt_AsLong(result);
  }
  
  Py_DECREF(arglist);
  Py_DECREF(primitive1);
  Py_DECREF(primitive2);
  Py_XDECREF(result);

  return (SoIntersectionDetectionAction::Resp)iresult;
}
%}

/* add python specific callback functions */
%extend SoIntersectionDetectionAction {
  void addVisitationCallback(SoType type, PyObject * pyfunc, PyObject * closure) {
    self->addVisitationCallback(type, SoIntersectionVisitationPythonCB,
                                (void *)Py_BuildValue("(OO)",
                                                      pyfunc,
                                                      closure ? closure : Py_None));
  }

  void removeVisitationCallback(SoType type, PyObject * pyfunc, PyObject * closure) {
    self->removeVisitationCallback(type, SoIntersectionVisitationPythonCB,
                                   (void *)Py_BuildValue("(OO)",
                                                         pyfunc,
                                                         closure ? closure : Py_None));
  }

  void setFilterCallback(PyObject * pyfunc, PyObject * closure = NULL) {
    self->setFilterCallback(SoIntersectionFilterPythonCB,
                            (void *)Py_BuildValue("(OO)",
                                                  pyfunc,
                                                  closure ? closure : Py_None));
  }

  void addIntersectionCallback(PyObject * pyfunc, PyObject * closure  = NULL) {
    self->addIntersectionCallback(SoIntersectionPythonCB,
                                  (void *)Py_BuildValue("(OO)",
                                                        pyfunc,
                                                        closure ? closure : Py_None));
  }

  void removeIntersectionCallback(PyObject * pyfunc, PyObject * closure  = NULL) {
    self->removeIntersectionCallback(SoIntersectionPythonCB,
                                     (void *)Py_BuildValue("(OO)",
                                                           pyfunc,
                                                           closure ? closure : Py_None));
  }
}
