/* add a method for wrapping c++ operator[] access */
%extend SbIntList {
  void __setitem__(const int i, int * value) { self->set(i,value); }
  int & __getitem__(const int i) { return (*self)[i]; }
  int get(const int i) { return (*self)[i]; }
}
