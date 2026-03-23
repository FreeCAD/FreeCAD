%extend SoSFEnum {
  void setValue(const SoSFEnum * other) { *self = *other; }
}
