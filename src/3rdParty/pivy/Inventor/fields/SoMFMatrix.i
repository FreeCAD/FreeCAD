%typemap(in) const SbMatrix *newvals {
  int len;

  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      $1 = new SbMatrix[len];
      for (int i = 0; i < len; i++) {
        SbMatrix * matPtr = NULL;
        PyObject * item = PyList_GetItem($input,i);
        SWIG_ConvertPtr(item, (void **) &matPtr, $1_descriptor, 1);
        if (matPtr != NULL) { $1[i] = *matPtr; }
      }
    } else { $1 = NULL; }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

/* free the list */
%typemap(freearg) const SbMatrix *newvals {
  if ($1) { delete[] $1; }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const SbMatrix *newvals {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%feature("shadow") SoMFMatrix::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFMatrix_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFMatrix_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFMatrix_setValues(*args)
%}

%ignore SoMFMatrix::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      SbMatrix * matPtr = new SbMatrix( result[i] );
      PyObject * obj = SWIG_NewPointerObj(matPtr, $descriptor(SbMatrix *), 1);
      PyList_SetItem($result, i, obj);
    }
  }
}

%rename(getValues) SoMFMatrix::__getValuesHelper__; 

%extend SoMFMatrix {
  const SbMatrix & __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, const SbMatrix & value) { self->set1Value(i, value); }
  const SbMatrix * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i >= self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
  void setValue(const SoMFMatrix * other){ *self = *other; }
}
