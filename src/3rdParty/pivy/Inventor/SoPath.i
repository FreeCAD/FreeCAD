/* add operator overloading methods instead of the global functions */
%extend SoPath {

/* extend __iter__ to return a new iterator object */
%pythoncode %{
   def __iter__(self):
      for i in range(self.getLength()):
         yield self.getNode(i)

   def index(self):
      for i in range(self.getLength()):
         yield self.getIndex(i)
%}

  int __eq__(const SoPath &u) { return *self == u; }
  int __nq__(const SoPath &u) { return *self != u; }
}
