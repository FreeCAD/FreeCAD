/* add a method for wrapping c++ operator[] access */
%extend SoEngineList {
  void __setitem__(const int i, SoEngine * value) { self->set(i,value); }
  SoEngine * __getitem__(const int i) { return (*self)[i]; }
  SoEngine * get(const int i) { return (*self)[i]; }
}
