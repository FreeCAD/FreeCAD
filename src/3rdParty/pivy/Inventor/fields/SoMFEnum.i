%{
static void
convert_SoMFEnum_array(PyObject * input, int len, int * temp)
{
  int i;
  for (i=0; i<len; i++) {
    PyObject * oi = PySequence_GetItem(input,i);
    if (PyNumber_Check(oi)) {
      temp[i] = (int) PyInt_AsLong(oi);
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

%typemap(in) const int * newvals (int * temp) {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (int*)malloc(len*sizeof(int));
    convert_SoMFEnum_array($input, len, temp);  
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

%ignore SoMFEnum::getValues(const int start) const;

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

%feature("shadow") SoMFEnum::setValues %{
def setValues(*args):
   if len(args) == 2:
      if isinstance(args[1], SoMFEnum):
         val = args[1].getValues()
         return _coin.SoMFEnum_setValues(args[0],0,len(val),val)
      else:
         return _coin.SoMFEnum_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      if isinstance(args[2], SoMFEnum):
         val = args[2].getValues()
         return _coin.SoMFEnum_setValues(args[0],args[1],len(val),val)
      else:
         return _coin.SoMFEnum_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFEnum_setValues(*args)
%}

%rename(getValues) SoMFEnum::__getValuesHelper__;

%extend SoMFEnum {
  const int __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, int value) { self->set1Value(i, value); }
  void setValue(const SoMFEnum * other) { *self = *other; }  
  const int * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i > self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
