%{
static void
convert_SbVec3d_array(PyObject *input, double temp[3])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 3) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1)) && 
      PyNumber_Check(PySequence_GetItem(input, 2))) {
    temp[0] = PyFloat_AsDouble(PySequence_GetItem(input, 0));
    temp[1] = PyFloat_AsDouble(PySequence_GetItem(input, 1));
    temp[2] = PyFloat_AsDouble(PySequence_GetItem(input, 2));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 3 doubles");
    PyErr_Print();
  } 
}
%}

%typemap(in) double v[3] (double temp[3]) {
  convert_SbVec3d_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) double v[3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* for some strange reason the %apply directive below doesn't work 
 * for this class on getValue(f,f,f)...
 * created this typemap for getValue(void) instead as a workaround.
 */
%typemap(out) double * {
  $result = Py_BuildValue("(fff)",
                          (double)(*($1)),
                          (double)(*($1+1)),
                          (double)(*($1+2)));
}

/* add operator overloading methods instead of the global functions */
%extend SbVec3d {
  SbVec3d __add__(const SbVec3d &u) { return *self + u; }
  SbVec3d __sub__(const SbVec3d &u) { return *self - u; }
  SbVec3d __mul__(const double d) { return *self * d; }
  SbVec3d __mul__(const SbDPMatrix &m) { SbVec3d res; m.multVecMatrix(*self,res); return res; }
  SbVec3d __rmul__(const double d) { return *self * d; }
  SbVec3d __div__(const double d) { return *self / d; }
  SbVec3d __truediv__(const double d) { return *self / d; }
  int __eq__(const SbVec3d &u) { return *self == u; }
  int __nq__(const SbVec3d &u) { return *self != u; }
  // add a method for wrapping c++ operator[] access
  double __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, double value) { (*self)[i] = value; }
  
%pythoncode %{
   def __iter__(self):
      for i in range(3):
         yield self[i]
      
   def __len__(self):
         return 3
%}
}

%apply double *OUTPUT { double & x, double & y, double & z };

%ignore SbVec3d::getValue(double & x, double & y, double & z) const;
