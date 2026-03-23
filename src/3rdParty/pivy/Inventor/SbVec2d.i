%{
static void
convert_SbVec2d_array(PyObject *input, double temp[2])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 2) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1))) {
    temp[0] = PyFloat_AsDouble(PySequence_GetItem(input, 0));
    temp[1] = PyFloat_AsDouble(PySequence_GetItem(input, 1));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 2 floats");
    PyErr_Print();
  } 
}
%}

%typemap(in) double v[2] (double temp[2]) {
  convert_SbVec2d_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) double v[2] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* add operator overloading methods instead of the global functions */
%extend SbVec2d {
  SbVec2d __add__(const SbVec2d &u) { return *self + u; }
  SbVec2d __sub__(const SbVec2d &u) { return *self - u; }
  SbVec2d __mul__(const float d) { return *self * d; }
  SbVec2d __rmul__(const float d) { return *self * d; }
  SbVec2d __div__(const float d) { return *self / d; }
  SbVec2d __truediv__(const float d) { return *self / d; }
  int __eq__(const SbVec2d &u ) { return *self == u; }
  int __nq__(const SbVec2d &u) { return *self != u; }
  // add a method for wrapping c++ operator[] access
  double __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, double value) { (*self)[i] = value; }

%pythoncode %{
   def __iter__(self):
      for i in range(2):
         yield self[i]

   def __len__(self):
         return 2
%}
}

%apply double *OUTPUT { double & x, double & y };

%ignore SbVec2d::getValue() const;
