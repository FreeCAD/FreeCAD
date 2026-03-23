%typemap(in) const float q[4] (float temp[4]) {
  convert_SbVec4f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) const float q[4] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%extend SoSFRotation {
  void setValue(const SoSFRotation * other) { *self = *other; }
}
