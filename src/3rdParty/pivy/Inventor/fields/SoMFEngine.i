%typemap(in) SoEngine ** {
  int len;

  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      $1 = (SoEngine **)malloc(len * sizeof(SoEngine *));
      for (int i = 0; i < len; i++ ) {
        SWIG_ConvertPtr(PyList_GetItem($input,i), (void **) $1 + i, $*1_descriptor,1);
      }
    } else { $1 = NULL; }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
    return NULL;
  }
}

/* free the list */
%typemap(freearg) SoEngine ** {
  if ($1) { free($1); }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) SoEngine ** {
  $1 = PySequence_Check($input) ? 1 : 0;
}
%feature("shadow") SoMFEngine::setValues(const int , const int , const SoEngine **) %{
def setValues(*args):
   if len(args) == 2:
     return _coin.SoMFEngine_setValues(args[0], 0, len(args[1]), args[1])
   elif len(args) == 3:
     return _coin.SoMFEngine_setValues(args[0], args[1], len(args[2]), args[2])
   return _coin.SoMFEngine_setValues(*args)
%}

%ignore SoMFEngine::getValues(const int start) const;

%typemap(in, numinputs = 0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      PyList_SetItem($result, i, autocast_base(result[i]));
    }
  }
}

%rename(getValues) SoMFEngine::__getValuesHelper__;

%extend SoMFEngine {
  const SoEngine * __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, SoEngine * value) { self->set1Value(i, value); }
  void setValue(const SoMFEngine * other ) { *self = *other; }
  const SoEngine ** __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i >= self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
