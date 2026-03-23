/* note that the following works for both SbColor sequences and float
   sequences because SbColor supports item access derived from SbVec3f */
%typemap(in) float [][3] (float (*temp)[3]) {
  int len;

  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      temp = (float (*)[3]) malloc(len*3*sizeof(float));
      convert_SoMFVec3f_array($input, len, temp);
    } else { temp = NULL; }
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

/* free the list; actually freeing is not save, if the typemap is used
   with setValuesPointer */
%typemap(freearg) float [][3] {
  if ($1) { free($1); }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) float [][3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) float hsv[3] (float temp[3]) {
  convert_SbVec3f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) float hsv[3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) float rgb[3] (float temp[3]) {
  convert_SbVec3f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_FLOAT_ARRAY) float [3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SoMFColor::setValues(const int, const int, const SbColor *);

%feature("shadow") SoMFColor::setValues(int start, int num, const float rgb[][3]) %{
def setValues(*args):
  if len(args) == 2:
    return _coin.SoMFColor_setValues(args[0], 0, len(args[1]), args[1])
  elif len(args) == 3:
    return _coin.SoMFColor_setValues(args[0], args[1], len(args[2]), args[2])

  return _coin.SoMFColor_setValues(*args)
%}

%ignore SoMFColor::getValues(const int start) const;

%typemap(in, numinputs = 0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      PyList_SetItem($result, i, 
        SWIG_NewPointerObj((void*)(result+i), SWIGTYPE_p_SbColor, 0));
    }
  }
}

%rename(getValues) SoMFColor::__getValuesHelper__;

%extend SoMFColor {
  const SbColor & __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, const SbColor & value) { self->set1Value(i, value); }
  void __setitem__(int i, const float rgb[3] ) { self->set1Value(i, rgb); }
  void setValue(const SoMFColor * other ) { *self = *other; }
  const SbColor * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i > self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
