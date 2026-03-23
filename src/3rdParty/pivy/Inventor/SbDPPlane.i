%extend SbDPPlane {
  int __eq__(const SbDPPlane & u) { return *self == u; }
  int __ne__(const SbDPPlane & u) { return *self != u; }
}
