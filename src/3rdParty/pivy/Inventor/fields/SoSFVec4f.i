%typemap(in) float xyzw[4] (float temp[4]) {
  convert_SbVec4f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) float xyzw[4] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%extend SoSFVec4f {
  void setValue(const SoSFVec4f * other){ *self = *other; }
}
