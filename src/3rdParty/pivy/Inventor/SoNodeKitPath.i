%extend SoNodeKitPath {      

/* extend __iter__ to return a new iterator object */
%pythoncode %{
   def __iter__(self):
      for i in range(self.getLength()):
         yield self.getNode(i)
         
   def index(self):
      for i in range(self.getLength()):
         yield self.getIndex(i)
%}

  int __eq__(const SoNodeKitPath &u) { return *self == u; }
  int __nq__(const SoNodeKitPath &u) { return !(*self == u); }
}
