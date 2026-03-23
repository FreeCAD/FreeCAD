%extend SbBox2s {
  int __eq__(const SbBox2s & u) { return *self == u; }
  int __ne__(const SbBox2s & u) { return *self != u; }
}
