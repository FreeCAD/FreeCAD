%typemap(in) const SbRotation *newvals {
  int len;

  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      $1 = new SbRotation[len];
      for (int i = 0; i < len; i++) {
        SbRotation * RotationPtr = NULL;
        PyObject * item = PyList_GetItem($input,i);
        SWIG_ConvertPtr(item, (void **) &RotationPtr, $1_descriptor, 1);
        if( RotationPtr != NULL )
          $1[i] = *RotationPtr;
      }
    } else { $1 = NULL; }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

/* free the list */
%typemap(freearg) const SbRotation *newvals {
  if ($1) { delete[] $1; }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const SbRotation *newvals {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%feature("shadow") SoMFRotation::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFRotation_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFRotation_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFRotation_setValues(*args)
%}

%ignore SoMFRotation::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      SbRotation * RotationPtr = new SbRotation( result[i] );
      PyObject * obj = SWIG_NewPointerObj(RotationPtr, $descriptor(SbRotation *), 1);
      PyList_SetItem($result, i, obj);
    }
  }
}

%rename(getValues) SoMFRotation::__getValuesHelper__;

%extend SoMFRotation {
  const SbRotation & __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, const SbRotation & value) { self->set1Value(i, value); }
  const SbRotation * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i >= self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
  void setValue(const SoMFRotation * other){ *self = *other; }
}
