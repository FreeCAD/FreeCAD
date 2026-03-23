/* add a method for wrapping c++ operator[] access */
%extend SoTypeList {
  void __setitem__(const int i, SoType value) { self->set(i,value); }
  SoType __getitem__(const int i) { return (*self)[i]; }
  SoType get(const int i) { return (*self)[i]; }
}
