%extend SoSFUInt32 {
  void setValue(const SoSFUInt32 * other) { *self = *other; }
}
