/* add a method for wrapping c++ operator[] access */
%extend SoPickedPointList {
  void __setitem__(const int i, SoPickedPoint * value) { self->set(i,value); }
  SoPickedPoint * __getitem__(const int i) { return (*self)[i]; }
  SoPickedPoint * get(const int i) { return (*self)[i]; }
}
