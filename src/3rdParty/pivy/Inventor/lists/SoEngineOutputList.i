/* add a method for wrapping c++ operator[] access */
%extend SoEngineOutputList {
  void __setitem__(const int i, SoEngineOutput * value) { self->set(i,value); }
  SoEngineOutput * __getitem__(const int i) { return (*self)[i]; }
  SoEngineOutput * get(const int i) { return (*self)[i]; }
}
