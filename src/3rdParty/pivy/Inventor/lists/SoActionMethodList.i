/* add a method for wrapping c++ operator[] access */
%extend SoActionMethodList {
  void __setitem__(const int i, SoActionMethod * value) { self->set(i,value); }
  SoActionMethod & __getitem__(const int i) { return (*self)[i]; }
  SoActionMethod get(const int i) { return (*self)[i]; }
}
