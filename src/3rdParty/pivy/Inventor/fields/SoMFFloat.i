/* types like SbVecXf, SbRotation define this typename for fixed size
   arrays which breaks here. Have to clear it before use */
%typemap(out) float *;

%{
static void
convert_SoMFFloat_array(PyObject * input, int len, float * temp)
{
  int i;
  for (i=0; i<len; i++) {
    PyObject * oi = PySequence_GetItem(input, i);
    if (PyNumber_Check(oi)) {
      temp[i] = (float) PyFloat_AsDouble(oi);
    } else {
      PyErr_SetString(PyExc_ValueError, "Sequence elements must be floats");
      Py_DECREF(oi);
      free(temp);       
      return;
    }
    Py_DECREF(oi);
  }
  return;
}
%}

%typemap(in) float * (float * temp) {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (float *)malloc(len*sizeof(float));
    convert_SoMFFloat_array($input, len, temp);
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

%ignore SoMFFloat::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++){
      PyList_SetItem($result, i, PyFloat_FromDouble((double)result[i]));
    }
  }
}

%feature("shadow") SoMFFloat::setValues %{
def setValues(*args):
   if len(args) == 2:
      if isinstance(args[1], SoMFFloat):
         val = args[1].getValues()
         return _coin.SoMFFloat_setValues(args[0],0,len(val),val)
      else:
         return _coin.SoMFFloat_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      if isinstance(args[2], SoMFFloat):
         val = args[2].getValues()
         return _coin.SoMFFloat_setValues(args[0],args[1],len(val),val)
      else:
         return _coin.SoMFFloat_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFFloat_setValues(*args)
%}

%rename(getValues) SoMFFloat::__getValuesHelper__;

%extend SoMFFloat {
  const float __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, float value) { self->set1Value(i, value); }
  void setValue(const SoMFFloat * other){ *self = *other; }  
  const float * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i > self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
