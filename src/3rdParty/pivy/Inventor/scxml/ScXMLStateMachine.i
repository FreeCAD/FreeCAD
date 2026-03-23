%{
static void
ScXMLStateMachineDeletePythonCB(void * userdata,
				ScXMLStateMachine * statemachine)
{
  PyObject *func, *arglist;
  PyObject *result, *statemachineCB;

  statemachineCB = SWIG_NewPointerObj((void *)statemachine, SWIGTYPE_p_ScXMLStateMachine, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OO)", PyTuple_GetItem((PyObject *)userdata, 1), statemachineCB);

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(statemachineCB);
  Py_XDECREF(result);
}

static void
ScXMLStateChangePythonCB(void * userdata,
			 ScXMLStateMachine * statemachine,
			 const char * stateidentifier,
			 SbBool enterstate,
			 SbBool success)
{
  PyObject *func, *arglist;
  PyObject *result, *statemachineCB;

  statemachineCB = SWIG_NewPointerObj((void *)statemachine, SWIGTYPE_p_ScXMLStateMachine, 0);

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(OOsii)", PyTuple_GetItem((PyObject *)userdata, 1),
			  statemachineCB, stateidentifier,
			  enterstate, success);
  
  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  Py_DECREF(arglist);
  Py_DECREF(statemachineCB);
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
%extend ScXMLStateMachine {
  void addDeleteCallback(PyObject *pyfunc, PyObject *userdata)
  {
    self->addDeleteCallback(ScXMLStateMachineDeletePythonCB,
			    (void *)Py_BuildValue("(OO)",
						  pyfunc,
						  userdata ? userdata : Py_None));
  }
  
  void removeDeleteCallback(PyObject *pyfunc, PyObject *userdata)
  {
    self->removeDeleteCallback(ScXMLStateMachineDeletePythonCB,
                               (void *)Py_BuildValue("(OO)",
                                                     pyfunc,
                                                     userdata ? userdata : Py_None));
  }
  
  void addStateChangeCallback(PyObject *pyfunc, PyObject *userdata)
  {
    self->addStateChangeCallback(ScXMLStateChangePythonCB,
				 (void *)Py_BuildValue("(OO)",
						       pyfunc,
						       userdata ? userdata : Py_None));
  }
  
  void removeStateChangeCallback(PyObject *pyfunc, PyObject *userdata)
  {
    self->removeStateChangeCallback(ScXMLStateChangePythonCB,
				    (void *)Py_BuildValue("(OO)",
							  pyfunc,
							  userdata ? userdata : Py_None));
  }
 }
