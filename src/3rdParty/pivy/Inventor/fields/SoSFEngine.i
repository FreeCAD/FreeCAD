%extend SoSFEngine {
  void setValue(const SoSFEngine * other) { *self = *other; }
}
