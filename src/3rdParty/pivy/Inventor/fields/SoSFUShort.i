%extend SoSFUShort {
  void setValue(const SoSFUShort * other) { *self = *other; }
}
