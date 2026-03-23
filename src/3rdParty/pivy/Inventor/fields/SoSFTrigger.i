%extend SoSFTrigger {
  void setValue(const SoSFTrigger * other) { *self = *other; }
}
