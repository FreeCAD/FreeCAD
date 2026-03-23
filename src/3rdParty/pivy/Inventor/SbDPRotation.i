%typemap(in) double q[4] (double temp[4]) {
  convert_SbVec4d_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) double q[4] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(out) float * {
  $result = Py_BuildValue("(ffff)",
                          (double)(*($1)),
                          (double)(*($1+1)),
                          (double)(*($1+2)),
                          (double)(*($1+3)));
}

/* add operator overloading methods instead of the global functions */
%extend SbDPRotation {
  SbDPRotation __mul__(const SbDPRotation &u) { return *self * u; }
  SbDPRotation __mul__(const double d) { SbDPRotation res(*self); return (res *= d); }
  SbVec3d __mul__(const SbVec3d & v) { SbVec3d res; self->multVec(v, res); return res; }
  int __eq__(const SbDPRotation &u) { return *self == u; }
  int __nq__(const SbDPRotation &u) { return *self != u; }
}

%apply float * OUTPUT { double & q0, double & q1, double & q2, double & q3, double & radians};

/* the next 2 typemaps handle the return value for getMatrix and getAxisAngle ~ getValue */
%typemap(in,numinputs=0) SbVec3d & axis, SbDPMatrix & matrix {
    $1 = new $1_basetype();
}
%typemap(argout) SbVec3d & axis, SbDPMatrix & matrix {
  $result = SWIG_NewPointerObj((void *) $1, $1_descriptor, 1);
}
/* undo effect of in typemap for setValue calls */
%typemap(in) const SbVec3d & axis = SWIGTYPE &;
%typemap(argout) const SbVec3d & axis {};

%ignore SbDPRotation::getValue(double & q0, double & q1, double & q2, double & q3) const;
%rename(getAxisAngle) SbDPRotation::getValue(SbVec3d & axis, double & radians) const;
%rename(getMatrix) SbDPRotation::getValue(SbDPMatrix & matrix) const;
