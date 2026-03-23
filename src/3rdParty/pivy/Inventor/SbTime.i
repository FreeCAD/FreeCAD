/* add operator overloading methods instead of the global functions */
%extend SbTime {
  SbTime __add__(const SbTime &u) { return *self + u; }
  SbTime __sub__(const SbTime &u) { return *self - u; }
  SbTime __mul__(const double d) { return *self * d; }
  SbTime __rmul__(const double d) { return *self * d; }
  SbTime __div__(const double d) { return *self / d; }
}
