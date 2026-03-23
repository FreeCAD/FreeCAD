%extend SoSFPlane {
  void setValue(const SoSFPlane * other) { *self = *other; }
}
