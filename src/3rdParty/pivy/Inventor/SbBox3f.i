%extend SbBox3f {
  int __eq__(const SbBox3f & u) { return *self == u; }
  int __ne__(const SbBox3f & u) { return *self != u; }
}
