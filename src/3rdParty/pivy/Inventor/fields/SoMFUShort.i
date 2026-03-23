%{
static void
convert_SoMFUShort_array(PyObject *input, int len, unsigned short *temp)
{
  int i;
  for (i=0; i<len; i++) {
    PyObject *oi = PySequence_GetItem(input, i);
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

%typemap(in) unsigned short * (unsigned short *temp) {
  int len;

  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (unsigned short*)malloc(len*sizeof(unsigned short));
    convert_SoMFUShort_array($input, len, temp);
  
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

%feature("shadow") SoMFUShort::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFUShort_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFUShort_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFUShort_setValues(*args)
%}

%ignore SoMFUShort::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++){
      PyList_SetItem($result, i, PyInt_FromLong((long)result[i]));
    }
  }
}

%rename(getValues) SoMFUShort::__getValuesHelper__;

%extend SoMFUShort {
  const short __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, short value) { self->set1Value(i, value); }
  void setValue( const SoMFUShort * other ) { *self = *other; }
  const unsigned short * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i > self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
