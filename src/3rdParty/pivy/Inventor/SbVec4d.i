%{
static void
convert_SbVec4d_array(PyObject * input, double temp[4])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 4) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1)) && 
      PyNumber_Check(PySequence_GetItem(input, 2)) && 
      PyNumber_Check(PySequence_GetItem(input, 3))) {
    temp[0] = PyFloat_AsDouble(PySequence_GetItem(input, 0));
    temp[1] = PyFloat_AsDouble(PySequence_GetItem(input, 1));
    temp[2] = PyFloat_AsDouble(PySequence_GetItem(input, 2));
    temp[3] = PyFloat_AsDouble(PySequence_GetItem(input, 3));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 4 floats");
    PyErr_Print();
  } 
}
%}

%typemap(in) double v[4] (double temp[4]) {
  convert_SbVec4d_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) double v[4] {
  $1 = PySequence_Check($input) ? 1 : 0 ;
}

%ignore SbVec2d::__imul__;

/* for some strange reason the %apply directive below doesn't work 
 * for this class on getValue(f,f,f,f)...
 * created this typemap for getValue(void) instead as a workaround.
 */
%typemap(out) double * {
  $result = Py_BuildValue("(ffff)",
                          (double)(*($1)),
                          (double)(*($1+1)),
                          (double)(*($1+2)),
                          (double)(*($1+3)));
}

/* add operator overloading methods instead of the global functions */
%extend SbVec4d {
  SbVec4d __add__(const SbVec4d &u) { return *self + u; }
  SbVec4d __sub__(const SbVec4d &u) { return *self - u; }
  SbVec4d __mul__(const double d) { return *self * d; }
  SbVec4d __mul__(const SbDPMatrix &m) { SbVec4d res; m.multVecMatrix(*self,res); return res; }
  SbVec4d __rmul__(const double d) { return *self * d; }
  SbVec4d __div__(const double d) { return *self / d; }
  SbVec4d __truediv__(const double d) { return *self / d; }
  int __eq__(const SbVec4d &u ) { return *self == u; }
  int __nq__(const SbVec4d &u) { return *self != u; }
  // swig - add a method for wrapping c++ operator[] access
  double __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, double value) { (*self)[i] = value; }
  
%pythoncode %{
   def __iter__(self):
      for i in range(4):
         yield self[i]

   def __len__(self):
         return 4
%}
}

%apply double *OUTPUT { double& x, double& y, double& z, double& w };

%ignore SbVec4d::getValue(double& x, double& y, double& z, double& w) const;
