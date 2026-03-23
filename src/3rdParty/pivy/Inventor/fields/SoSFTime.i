%extend SoSFTime {
#if COIN_MAJOR_VERSION > 2
  void setValue(SbTime & other) { *self = other; }
#endif
  void setValue(const SoSFTime * other) { *self = *other; }
}
