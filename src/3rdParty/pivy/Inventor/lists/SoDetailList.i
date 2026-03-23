/* add a method for wrapping c++ operator[] access */
%extend SoDetailList {
  void __setitem__(const int i, SoDetail * value) { self->set(i,value); }
  SoDetail * __getitem__(const int i) { return (*self)[i]; }
  SoDetail * get(const int i) { return (*self)[i]; }
}
