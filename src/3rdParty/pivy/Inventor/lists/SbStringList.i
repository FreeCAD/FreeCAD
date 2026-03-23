/* add a method for wrapping c++ operator[] access */
%extend SbStringList {
  void __setitem__(const int i, SbString * value) { self->set(i,value); }
  SbString * __getitem__(int i) { return (*self)[i]; }
  SbString * get(const int i) { return (*self)[i]; }
}
