%{
static void
convert_SbVec3f_array(PyObject * input, float temp[3])
{
  if (PySequence_Check(input) && (PySequence_Size(input) == 3) &&
      PyNumber_Check(PySequence_GetItem(input, 0)) && 
      PyNumber_Check(PySequence_GetItem(input, 1)) && 
      PyNumber_Check(PySequence_GetItem(input, 2))) {
    temp[0] = PyFloat_AsDouble(PySequence_GetItem(input, 0));
    temp[1] = PyFloat_AsDouble(PySequence_GetItem(input, 1));
    temp[2] = PyFloat_AsDouble(PySequence_GetItem(input, 2));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a sequence with 3 floats");
    PyErr_Print();
  } 
}
%}

%typemap(in) float v[3] (float temp[3]) {
  convert_SbVec3f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) float v[3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%ignore SbVec2d::__imul__;

/* for some strange reason the %apply directive below doesn't work 
 * for this class on getValue(f,f,f)...
 * created this typemap for getValue(void) instead as a workaround.
 */
%typemap(out) float * {
  $result = Py_BuildValue("(fff)",
                          (double)(*($1)),
                          (double)(*($1+1)),
                          (double)(*($1+2)));
}

/* add operator overloading methods instead of the global functions */
%extend SbVec3f {
  SbVec3f __add__(const SbVec3f &u) { return *self + u; }
  SbVec3f __sub__(const SbVec3f &u) { return *self - u; }
  SbVec3f __mul__(const float d) { return *self * d; }
  SbVec3f __mul__(const SbMatrix &m) { SbVec3f res; m.multVecMatrix(*self,res); return res; }
  SbVec3f __rmul__(const float d) { return *self * d; }
  SbVec3f __div__( const float d) { return *self / d; }
  SbVec3f __truediv__( const float d) { return *self / d; }
  int __eq__(const SbVec3f &u ) { return *self == u; }
  int __nq__(const SbVec3f &u) { return *self != u; }
  // add a method for wrapping c++ operator[] access
  float __getitem__(int i) { return (self->getValue())[i]; }
  void  __setitem__(int i, float value) { (*self)[i] = value; }

%pythoncode %{
   def __iter__(self):
      for i in range(3):
         yield self[i]
      
   def __len__(self):
         return 3
%}
}

%apply float *OUTPUT { float & x, float & y, float & z };

%ignore SbVec3f::getValue(float & x, float & y, float & z) const;
