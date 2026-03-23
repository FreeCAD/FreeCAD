%extend SoSFString {
  void setValue(const SoSFString * other) { *self = *other; }
}
