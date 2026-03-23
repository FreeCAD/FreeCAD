%typemap(in) float xyz[3] (float temp[3]) {
  convert_SbVec3f_array($input, temp);
  $1 = temp;
}

%typemap(typecheck) float xyz[3] {
  $1 = PySequence_Check($input) ? 1 : 0;
}

%extend SoSFVec3f {
  void setValue(const SoSFVec3f * other){ *self = *other; }
}
