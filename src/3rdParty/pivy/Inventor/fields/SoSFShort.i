%extend SoSFShort {
  void setValue(const SoSFShort * other) { *self = *other; }
}
