%{
static SoVolumeRender::AbortCode
SoVolumeRenderAbortPythonCB(int totalslices, int thisslice, void * userdata)
{
  PyObject *func, *arglist;
  PyObject *result;
  SoVolumeRender::AbortCode res;
  
  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("iiO", PyInt_FromLong(totalslices),
                          PyInt_FromLong(thisslice),
                          PyTuple_GetItem((PyObject *)userdata, 1));

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }

  res = (SoVolumeRender::AbortCode)PyInt_AsLong(result);

  Py_DECREF(arglist);
  Py_XDECREF(result);

  return res;
}
%}

%typemap(in) PyObject * pyfunc %{
  if (!PyCallable_Check($input)) {
	PyErr_SetString(PyExc_TypeError, "need a callable object!");
	return NULL;
  }
  $1 = $input;
%}

/* add python specific callback functions */
%extend SoVolumeRender {
  void setAbortCallback(PyObject * pyfunc, PyObject * userdata){
    if (userdata == NULL) {
      Py_INCREF(Py_None);
      userdata = Py_None;
    }
	  
    PyObject * t = PyTuple_New(2);
    PyTuple_SetItem(t, 0, pyfunc);
    PyTuple_SetItem(t, 1, userdata);
    Py_INCREF(pyfunc);
    Py_INCREF(userdata);

    self->setAbortCallback(SoVolumeRenderAbortPythonCB, (void *)t);
  }
}
