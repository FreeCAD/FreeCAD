%extend SbBox2f {
  int __eq__(const SbBox2f & u) { return *self == u; }
  int __ne__(const SbBox2f & u) { return *self != u; }
}
