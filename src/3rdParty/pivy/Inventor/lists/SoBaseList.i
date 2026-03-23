/* add a method for wrapping c++ operator[] access */
%extend SoBaseList {
  void __setitem__(const int i, SoBase * value) { self->set(i,value); }
  SoBase * __getitem__(const int i) { return (*self)[i]; }
  SoBase * get(const int i) { return (*self)[i]; }
}
