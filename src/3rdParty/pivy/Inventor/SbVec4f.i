%{
static void
convert_SbVec4f_array(PyObject * input, float temp[4])
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

%typemap(in) float v[4] (float temp[4]) {
  convert_SbVec4f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) float v[4] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* for some strange reason the %apply directive below doesn't work 
 * for this class on getValue(f,f,f,f)...
 * created this typemap for getValue(void) instead as a workaround.
 */
%typemap(out) float * {
  $result = Py_BuildValue("(ffff)",
                          (double)(*($1)),
                          (double)(*($1+1)),
                          (double)(*($1+2)),
                          (double)(*($1+3)));
}

/* add operator overloading methods instead of the global functions */
%extend SbVec4f {
  SbVec4f __add__(const SbVec4f &u) { return *self + u; }
  SbVec4f __sub__(const SbVec4f &u) { return *self - u; }
  SbVec4f __mul__(const float d) { return *self * d; }
  SbVec4f __mul__(const SbMatrix &m) { SbVec4f res; m.multVecMatrix(*self,res); return res; }
  SbVec4f __rmul__(const float d) { return *self * d; }
  SbVec4f __div__(const float d) { return *self / d; }
  SbVec4f __truediv__(const float d) { return *self / d; }
  int __eq__(const SbVec4f &u) { return *self == u; }
  int __nq__(const SbVec4f &u) { return *self != u; }
  // swig - add a method for wrapping c++ operator[] access
  float __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, float value) { (*self)[i] = value; }
  
%pythoncode %{
   def __iter__(self):
      for i in range(4):
         yield self[i]
      
   def __len__(self):
         return 4
%}
}

%apply float *OUTPUT { float& x, float& y, float& z, float& w };

%ignore SbVec4f::getValue(float& x, float& y, float& z, float& w) const;
