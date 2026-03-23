%{
static void
convert_SoMFVec2f_array(PyObject * input, int len, float temp[][2])
{
  int i,j;
  for (i=0; i<len; i++) {
    PyObject * oi = PySequence_GetItem(input, i);
    for (j=0; j<2; j++) {
      PyObject * oj = PySequence_GetItem(oi, j);
      if (PyNumber_Check(oj)) {
        temp[i][j] = (float) PyFloat_AsDouble(oj);
      } else {
        PyErr_SetString(PyExc_ValueError, "Sequence elements must be numbers");
        free(temp);       
        Py_DECREF(oi);
        Py_DECREF(oj);
        return;
      }
      Py_DECREF(oj);
    }
    Py_DECREF(oi);
  }
  return;
}
%}

%typemap(in) const float xy[][2] (float (*temp)[2]) {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (float (*)[2]) malloc(len*2*sizeof(float));
    convert_SoMFVec2f_array($input, len, temp);
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
    $1 = NULL;
  }
}

/* free the list */
%typemap(freearg) const float xy[][2] {
  if($1) free($1);
}

%typemap(in) const float xy[2] (float temp[2]) {
  convert_SbVec2f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const float xy[2] {
  void *ptr;
  $1 = (PySequence_Check($input) && SWIG_ConvertPtr($input, &ptr, $descriptor(SoMFVec2f *), 0) == -1) ? 1 : 0;
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const float xy[][2] {
  if(PySequence_Check($input) && PySequence_Size($input) > 0 ){
    PyObject * obj = PySequence_GetItem($input, 0);
    void * ptr;
    if (SWIG_ConvertPtr(obj, &ptr, $descriptor(SbVec2f *), 0) == -1) { $1 = 1; }
    else { $1 = 0; }
    Py_DECREF(obj);
  }
  else { $1 = 0; }
}

%typemap(in) const SbVec2f *newvals {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      $1 = new SbVec2f[len];
      for (int i = 0; i < len; i++) {
        SbVec2f * VecPtr = NULL;
        PyObject * item = PyList_GetItem($input,i);
        SWIG_ConvertPtr(item, (void **) &VecPtr, $1_descriptor, 1);
        if (VecPtr != NULL) { $1[i] = *VecPtr; }
      }
    } else { $1 = NULL; }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
  }
}

/* free the list */
%typemap(freearg) const SbVec2f *newvals {
  if ($1) { delete[] $1; }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const SbVec2f *newvals {
  if (PySequence_Check($input)) {
    if (PySequence_Size($input) == 0) { $1 = 1; }
    else {
      PyObject * obj = PySequence_GetItem($input, 0);
      void * ptr;
      if (SWIG_ConvertPtr(obj, &ptr, $descriptor(SbVec2f *), 0) != -1) { $1 = 1; }
      else { $1 = 0; }
      Py_DECREF(obj);
    }
  } else { $1 = 0; }
}

%feature("shadow") SoMFVec2f::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFVec2f_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFVec2f_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFVec2f_setValues(*args)
%}

%ignore SoMFVec2f::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) (int & len) {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      SbVec2f * Vec2fPtr = new SbVec2f( result[i] );
      PyObject * obj = SWIG_NewPointerObj(Vec2fPtr, $descriptor(SbVec2f *), 1);
      PyList_SetItem($result, i, obj);
    }
  }
}

%rename(getValues) SoMFVec2f::__getValuesHelper__;

%extend SoMFVec2f {
  const SbVec2f & __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, const SbVec2f & value) { self->set1Value(i, value); }  
  void setValue( const SoMFVec2f * other ){ *self = *other; }
  const SbVec2f * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i >= self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
