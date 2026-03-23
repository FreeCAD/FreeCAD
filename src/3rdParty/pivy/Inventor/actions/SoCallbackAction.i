%{
static SoCallbackAction::Response
SoCallbackActionPythonCB(void * userdata,
                         SoCallbackAction * action,
                         const SoNode * node) {
  PyObject *func, *arglist;
  PyObject *result, *acCB, *pynode;
  int iresult = 0;

  acCB = SWIG_NewPointerObj((void *)action, SWIGTYPE_p_SoCallbackAction, 0);
  pynode = autocast_base((SoBase*)node);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OOO)", PyTuple_GetItem((PyObject *)userdata, 1), acCB, pynode);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }
  else {
    iresult = PyInt_AsLong(result);
  }
  
  Py_DECREF(arglist);
  Py_DECREF(acCB);
  Py_XDECREF(result);

  return (SoCallbackAction::Response)iresult;
}

static void
SoTrianglePythonCB(void * userdata, SoCallbackAction * action,
                   const SoPrimitiveVertex * v1,
                   const SoPrimitiveVertex * v2,
                   const SoPrimitiveVertex * v3)
{
  PyObject *func, *arglist;
  PyObject *result, *acCB;
  PyObject *vertex1, *vertex2, *vertex3;

  acCB = SWIG_NewPointerObj((void *) action, SWIGTYPE_p_SoCallbackAction, 0);
  vertex1 = SWIG_NewPointerObj((void *) v1, SWIGTYPE_p_SoPrimitiveVertex, 0);
  vertex2 = SWIG_NewPointerObj((void *) v2, SWIGTYPE_p_SoPrimitiveVertex, 0);
  vertex3 = SWIG_NewPointerObj((void *) v3, SWIGTYPE_p_SoPrimitiveVertex, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OOOOO)", PyTuple_GetItem((PyObject *)userdata, 1), acCB, vertex1, vertex2, vertex3);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(acCB);
  Py_DECREF(vertex1);
  Py_DECREF(vertex2);
  Py_DECREF(vertex3);
  Py_XDECREF(result);
}

static void
SoLineSegmentPythonCB(void * userdata, SoCallbackAction * action,
                const SoPrimitiveVertex * v1,
                const SoPrimitiveVertex * v2)
{
  PyObject *func, *arglist;
  PyObject *result, *acCB;
  PyObject *vertex1, *vertex2;

  acCB = SWIG_NewPointerObj((void *) action, SWIGTYPE_p_SoCallbackAction, 0);
  vertex1 = SWIG_NewPointerObj((void *) v1, SWIGTYPE_p_SoPrimitiveVertex, 0);
  vertex2 = SWIG_NewPointerObj((void *) v2, SWIGTYPE_p_SoPrimitiveVertex, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OOOO)", PyTuple_GetItem((PyObject *)userdata, 1), acCB, vertex1, vertex2);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(acCB);
  Py_DECREF(vertex1);
  Py_DECREF(vertex2);
  Py_XDECREF(result);
}

static void
SoPointPythonCB(void * userdata, SoCallbackAction * action, const SoPrimitiveVertex * v)
{
  PyObject *func, *arglist;
  PyObject *result, *acCB;
  PyObject *vertex;

  acCB = SWIG_NewPointerObj((void *) action, SWIGTYPE_p_SoCallbackAction, 0);
  vertex = SWIG_NewPointerObj((void *) v, SWIGTYPE_p_SoPrimitiveVertex, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OOO)", PyTuple_GetItem((PyObject *)userdata, 1), acCB, vertex);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(acCB);
  Py_DECREF(vertex);
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

%ignore SoCallbackAction::getMaterial(SbColor & ambient, SbColor & diffuse,
                                      SbColor & specular, SbColor & emission,
                                      float & shininess, float & transparency,
                                      const int index = 0) const;

%extend SoCallbackAction {
  /* return a list for the out getMaterial() parameters */
  PyObject * getMaterial(const int index = 0) {
    SbColor * ambient = new SbColor;
    SbColor * diffuse = new SbColor;
    SbColor * specular = new SbColor;
    SbColor * emission = new SbColor;
    float shininess, transparency;

    self->getMaterial(*ambient, *diffuse, *specular, *emission, shininess, transparency, index);
    
    return Py_BuildValue("(OOOOff)", 
                         SWIG_NewPointerObj(ambient, SWIGTYPE_p_SbColor, 1),
                         SWIG_NewPointerObj(diffuse, SWIGTYPE_p_SbColor, 1),
                         SWIG_NewPointerObj(specular, SWIGTYPE_p_SbColor, 1),
                         SWIG_NewPointerObj(emission, SWIGTYPE_p_SbColor, 1),
                         shininess,
                         transparency);
  }

  /* add python specific callback functions */
  void addPreCallback(const SoType type, PyObject *pyfunc, PyObject *userdata) {
    self->addPreCallback(type, SoCallbackActionPythonCB,
                         (void *)Py_BuildValue("(OO)",
                                               pyfunc,
                                               userdata ? userdata : Py_None));
  }

  void addPostCallback(const SoType type, PyObject *pyfunc, PyObject *userdata) {
    self->addPostCallback(type, SoCallbackActionPythonCB,
                          (void *)Py_BuildValue("(OO)",
                                                pyfunc,
                                                userdata ? userdata : Py_None));
  }

  void addPreTailCallback(PyObject *pyfunc, PyObject *userdata) {
    self->addPreTailCallback(SoCallbackActionPythonCB,
                             (void *)Py_BuildValue("(OO)",
                                                   pyfunc,
                                                   userdata ? userdata : Py_None));
  }

  void addPostTailCallback(PyObject *pyfunc, PyObject *userdata) {
    self->addPostTailCallback(SoCallbackActionPythonCB,
                              (void *)Py_BuildValue("(OO)",
                                                    pyfunc,
                                                    userdata ? userdata : Py_None));
  }

  void addTriangleCallback(const SoType type, PyObject *pyfunc, PyObject *userdata) {
    self->addTriangleCallback(type, SoTrianglePythonCB,
                              (void *)Py_BuildValue("(OO)",
                                                    pyfunc,
                                                    userdata ? userdata : Py_None));
  }

  void addLineSegmentCallback(const SoType type, PyObject *pyfunc, PyObject *userdata) {
    self->addLineSegmentCallback(type, SoLineSegmentPythonCB,
                                 (void *)Py_BuildValue("(OO)",
                                                       pyfunc,
                                                       userdata ? userdata : Py_None));
  }

  void addPointCallback(const SoType type, PyObject *pyfunc, PyObject *userdata) {
    self->addPointCallback(type, SoPointPythonCB,
                           (void *)Py_BuildValue("(OO)",
                                                 pyfunc,
                                                 userdata ? userdata : Py_None));
  }
}
