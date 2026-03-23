%{
static void
sensorQueueChangedPythonCB(void * userdata)
{
  PyObject *func, *arglist;
  PyObject *result;

  /* the first item in the userdata sequence is the python callback
   * function; the second is the supplied userdata python object */
  func = PyTuple_GetItem((PyObject *)userdata, 0);
  arglist = Py_BuildValue("(O)", PyTuple_GetItem((PyObject *)userdata, 1));

  if ((result = PyObject_CallObject(func, arglist)) == NULL) {
    PyErr_Print();
  }  
  Py_DECREF(arglist);
  Py_XDECREF(result);
}
%}

/* add python specific callback functions */
%extend SoSensorManager {
  void setChangedCallback(PyObject * pyfunc, PyObject * data) {
    self->setChangedCallback(sensorQueueChangedPythonCB, 
                             (void *)Py_BuildValue("(OO)", 
                                                   pyfunc, 
                                                   data ? data : Py_None));
  }

  PyObject * isTimerSensorPending() {
    SbTime tm;
    if (!self->isTimerSensorPending(tm)) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    SbTime * retTm = new SbTime(tm.getValue());
    return SWIG_NewPointerObj((void *)retTm, SWIGTYPE_p_SbTime, 1);
  }
}
