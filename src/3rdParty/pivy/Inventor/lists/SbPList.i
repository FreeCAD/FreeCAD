%extend SbPList {
  void * __getitem__(int i) { return self->get(i); }
  void __setitem__(int i, void * val) { self->set(i,val); }
/* extend __iter__ to return a new iterator object */
%pythoncode %{
   def __iter__(self):
      for i in range(self.getLength()):
         yield self[i]
%}
}
