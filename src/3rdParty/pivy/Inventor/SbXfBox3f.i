%extend SbXfBox3f {
  int __eq__(const SbXfBox3f & u) { return *self == u; }
  int __ne__(const SbXfBox3f & u) { return *self != u; }
}
