%extend SoSFName {
  void setValue(const SoSFName * other) { *self = *other; }
}
