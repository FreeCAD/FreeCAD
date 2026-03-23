/* add a method for wrapping c++ operator[] access */
%extend SoPathList {
  void __setitem__(const int i, SoPath * value) { self->set(i,value); }
  SoPath * __getitem__(int i) { return (*self)[i]; }
  SoPath * get(const int i) { return (*self)[i]; }
}
