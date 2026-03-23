%{
static void
convert_SbVec2s_array(PyObject *input, short temp[2])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 2) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1))) {
    temp[0] = PyInt_AsLong(PySequence_GetItem(input, 0));
    temp[1] = PyInt_AsLong(PySequence_GetItem(input, 1));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 2 shorts");
    PyErr_Print();
  } 
}
%}

%typemap(in) short v[2] (short temp[2]) {
  convert_SbVec2s_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) short v[2] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* add operator overloading methods instead of the global functions */
%extend SbVec2s {
  SbVec2s __add__(const SbVec2s &u) { return *self + u; }
  SbVec2s __sub__(const SbVec2s &u) { return *self - u; }
  SbVec2s __mul__(const double d) { return *self * d; }
  SbVec2s __rmul__(const double d) { return *self * d; }
  SbVec2s __div__(const double d) { return *self / d; }
  SbVec2s __truediv__(const double d) { return *self / d; }
  int __eq__(const SbVec2s &u) { return *self == u; }
  int __nq__(const SbVec2s &u ) { return *self != u; }
  // add a method for wrapping c++ operator[] access
  short __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, short value) { (*self)[i] = value; }

%pythoncode %{
   def __iter__(self):
      for i in range(2):
         yield self[i]
      
   def __len__(self):
         return 2
%}
}

%apply short *OUTPUT { short &x, short &y };

%ignore SbVec2s::getValue() const;
