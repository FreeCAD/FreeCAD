/* add a method for wrapping c++ operator[] access */
%extend SoNodeList {
  void __setitem__(const int i, SoNode * value) { self->set(i,value); }
  SoNode * __getitem__(const int i) { return (*self)[i]; }
  SoNode * get(const int i) { return (*self)[i]; }
}
