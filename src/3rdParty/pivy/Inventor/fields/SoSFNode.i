%extend SoSFNode {
  void setValue(const SoSFNode * other) { *self = *other; }
}
