%{
static void
convert_SoMFShort_array(PyObject * input, int len, short * temp)
{
  int i;
  for (i=0; i<len; i++) {
    PyObject * oi = PySequence_GetItem(input, i);
    if (PyNumber_Check(oi)) {
      temp[i] = (short) PyInt_AsLong(oi);
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
      free(temp);       
      Py_DECREF(oi);
      return;
    }
    Py_DECREF(oi);
  }
  return;
}
%}

%typemap(in) short * (short * temp) {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (short*)malloc(len*sizeof(short));
    convert_SoMFShort_array($input, len, temp);  
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

%feature("shadow") SoMFShort::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFShort_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFShort_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFShort_setValues(*args)
%}

%ignore SoMFShort::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      PyList_SetItem($result, i, PyInt_FromLong((long)result[i]));
    }
  }
}

%rename(getValues) SoMFShort::__getValuesHelper__;

%extend SoMFShort {
  const short __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, short value) { self->set1Value(i, value); }
  void setValue(const SoMFShort * other ) { *self = *other; }
  const short * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i > self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
