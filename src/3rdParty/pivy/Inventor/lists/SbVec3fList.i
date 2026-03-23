/* add a method for wrapping c++ operator[] access */
%extend SbVec3fList {
  void __setitem__(const int i, SbVec3f * value) { self->set(i,value); }
  SbVec3f * __getitem__(int i) { return (*self)[i]; }
  SbVec3f * get(const int i) { return (*self)[i]; }
}
