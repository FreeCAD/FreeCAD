%typemap(in) float q[4] (float temp[4]) {
    convert_SbVec4f_array($input, temp);
    $1 = temp;
}

%typemap(typecheck) float q[4] {
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
%extend SbRotation {
    SbRotation __mul__(const SbRotation &u) { return *self * u; }
    SbRotation __mul__(const double d) { SbRotation res(*self); return (res *= d); }
    SbVec3f __mul__(const SbVec3f & v) { SbVec3f res; self->multVec(v, res); return res; }
    int __eq__(const SbRotation &u) { return *self == u; }
    int __nq__(const SbRotation &u) { return *self != u; }
%pythoncode %{
    def __imul__(self, other):
        return self * other
%}
}

%apply float * OUTPUT { float & q0, float & q1, float & q2, float & q3, float & radians};

/* the next 2 typemaps handle the return value for getMatrix and getAxisAngle ~ getValue */
%typemap(in,numinputs=0) SbVec3f & axis, SbMatrix & matrix {
    $1 = new $1_basetype();
}
%typemap(argout) SbVec3f & axis, SbMatrix & matrix {
    $result = SWIG_NewPointerObj((void *) $1, $1_descriptor, 1);
}
/* undo effect of in typemap for setValue calls */
%typemap(in) const SbVec3f & axis = SWIGTYPE &;
%typemap(argout) const SbVec3f & axis {};

%ignore SbRotation::getValue(float & q0, float & q1, float & q2, float & q3) const;
%rename(getAxisAngle) SbRotation::getValue(SbVec3f & axis, float & radians) const;
%rename(getMatrix) SbRotation::getValue(SbMatrix & matrix) const;
