%extend SbName {     
  /* add operator overloading methods instead of the global functions */
  int __eq__(const SbName &u) { return *self == u; }
  int __nq__(const SbName &u) { return *self != u; }
  int __eq__(char * u) { return *self == u; }
  int __nq__(char * u) { return *self != u; }
  int __eq__(const SbString & u) { return *self == u.getString(); }
  int __nq__(const SbString & u) { return *self != u.getString(); }  
  // add a method for wrapping c++ operator[] access
  char __getitem__(int i) { return self->getString()[i]; }
  // iterator for SbName
%pythoncode %{
  def __iter__(self):
    return getString().__iter__()
%}
  const char * __repr__(void) { return self->getString(); }
}

