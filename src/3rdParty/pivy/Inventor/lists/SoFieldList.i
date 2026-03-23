/* add a method for wrapping c++ operator[] access */
%extend SoFieldList {
  void __setitem__(const int i, SoField * value) { self->set(i,value); }
  SoField * __getitem__(const int i) { return (*self)[i]; }
  SoField * get(const int i) { return (*self)[i]; }
}
