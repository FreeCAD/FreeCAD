%extend SoSFInt32 {
  void setValue(const SoSFInt32 * other) { *self = *other; }
}

