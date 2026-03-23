/* add operator overloading methods instead of the global functions */
%extend SbViewportRegion {
  int __eq__(const SbViewportRegion &u) { return *self == u; }
  int __ne__(const SbViewportRegion &u) { return !(*self == u ); }
}
