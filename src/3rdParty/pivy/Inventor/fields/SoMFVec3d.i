%{
static void
convert_SoMFVec3d_array(PyObject * input, int len, double temp[][3])
{
  int i,j;
  for (i=0; i<len; i++) {
    PyObject * oi = PySequence_GetItem(input, i);
    for (j=0; j<3; j++) {
      PyObject * oj = PySequence_GetItem(oi, j);
      if (PyNumber_Check(oj)) {
        temp[i][j] = PyFloat_AsDouble(oj);
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

%typemap(in) const double xyz[][3] (double (*temp)[3]) {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    temp = (double (*)[3]) malloc(len*3*sizeof(double));
    convert_SoMFVec3d_array($input, len, temp);  
    $1 = temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence.");
    $1 = NULL;
  }
}

/* free the list */
%typemap(freearg) const double xyz[][3] {
  if ($1) { free($1); }
}

%typemap(in) const double xyz[3] (double temp[3]) {
  convert_SbVec3d_array($input, temp);
  $1 = temp;
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const double xyz[3] {
  void * ptr;
  $1 = (PySequence_Check($input) && SWIG_ConvertPtr($input, &ptr, $descriptor(SoMFVec3d *), 0) == -1) ? 1 : 0;
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const double xyz[][3] {
  if(PySequence_Check($input) && PySequence_Size($input) > 0 ){
    PyObject * obj = PySequence_GetItem($input, 0);
    void * ptr;
    if (SWIG_ConvertPtr(obj, &ptr, $descriptor(SbVec3d *), 0) == -1) { $1 = 1; }
    else { $1 = 0; }
    Py_DECREF(obj);
  } else { $1 = 0; }
}

%typemap(in) const SbVec3d * newvals {
  int len;
  if (PySequence_Check($input)) {
    len = PySequence_Length($input);
    if (len > 0) {
      $1 = new SbVec3d[len];
      for (int i = 0; i < len; i++) {
        SbVec3d * VecPtr = NULL;
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
%typemap(freearg) const SbVec3d * newvals {
  if ($1) { delete[] $1; }
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) const SbVec3d * newvals {
  if (PySequence_Check($input)) {
    if (PySequence_Size($input) == 0) { $1 = 1; }
    else {
      PyObject * obj = PySequence_GetItem($input, 0);
      void * ptr;
      if (SWIG_ConvertPtr(obj, &ptr, $descriptor(SbVec3d *), 0) != -1) { $1 = 1; }
      else { $1 = 0; }
      Py_DECREF(obj);
    }
  }
  else { $1 = 0; }
}

%feature("shadow") SoMFVec3d::setValues %{
def setValues(*args):
   if len(args) == 2:
      return _coin.SoMFVec3d_setValues(args[0],0,len(args[1]),args[1])
   elif len(args) == 3:
      return _coin.SoMFVec3d_setValues(args[0],args[1],len(args[2]),args[2])
   return _coin.SoMFVec3d_setValues(*args)
%}

%ignore SoMFVec3d::getValues(const int start) const;

%typemap(in,numinputs=0) int & len (int temp) {
  $1 = &temp;
  *$1 = 0;
}

%typemap(argout) int & len {
  Py_XDECREF($result); /* free up any previous result */
  $result = PyList_New(*$1);
  if (result) {
    for (int i = 0; i < *$1; i++) {
      SbVec3d * VecPtr = new SbVec3d(result[i]);
      PyObject * obj = SWIG_NewPointerObj(VecPtr, $descriptor(SbVec3d *), 1);
      PyList_SetItem($result, i, obj);
    }
  }
}

%rename(getValues) SoMFVec3d::__getValuesHelper__;

%extend SoMFVec3d {
  const SbVec3d & __getitem__(int i) { return (*self)[i]; }
  void  __setitem__(int i, const SbVec3d & value) { self->set1Value(i, value); }  
  void setValue( const SoMFVec3d * other ){ *self = *other; }
  const SbVec3d * __getValuesHelper__(int & len, int i = 0) {
    if (i < 0 || i >= self->getNum()) { return NULL; }
    len = self->getNum() - i;
    return self->getValues(i);
  }
}
