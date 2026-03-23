%extend SbBox3s {
  int __eq__(const SbBox3s & u) { return *self == u; }
  int __ne__(const SbBox3s & u) { return *self != u; }
}
