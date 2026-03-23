%{
static void
convert_SbVec3s_array(PyObject * input, short temp[3])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 3) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1)) &&
      PyNumber_Check(PySequence_GetItem(input, 2))) {
    temp[0] = PyInt_AsLong(PySequence_GetItem(input, 0));
    temp[1] = PyInt_AsLong(PySequence_GetItem(input, 1));
    temp[2] = PyInt_AsLong(PySequence_GetItem(input, 2));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 3 shorts");
    PyErr_Print();
  } 
}
%}

%typemap(in) short v[3] (short temp[3]) {
  convert_SbVec3s_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) short v[3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* add operator overloading methods instead of the global functions */
%extend SbVec3s {
  SbVec3s __add__(const SbVec3s &u) { return *self + u; }
  SbVec3s __sub__(const SbVec3s &u) { return *self - u; }
  SbVec3s __mul__(const double d) { return *self * d; }
  SbVec3s __rmul__(const double d) { return *self * d; }
  SbVec3s __div__(const double d) { return *self / d; }
  SbVec3s __truediv__(const double d) { return *self / d; }
  int __eq__(const SbVec3s &u) { return *self == u; }
  int __nq__(const SbVec3s &u) { return *self != u; }
  // add a method for wrapping c++ operator[] access
  short __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, short value) { (*self)[i] = value; }

%pythoncode %{
   def __iter__(self):
      for i in range(3):
         yield self[i]
      
   def __len__(self):
         return 3
%}
}

%apply short *OUTPUT { short &x, short &y, short &z };

%ignore SbVec3s::getValue(void) const;
