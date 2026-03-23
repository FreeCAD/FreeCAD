%extend SoBase {
  /* add a public destructor - otherwise refcount of new SoBase
   * derived instances, raised by the autoref feature, never gets
   * decreased */
  ~SoBase() { self->unref();}

size_t __hash__() { return (size_t) $self; }

%pythoncode %{
    def __eq__(self,other):
      return (self.this == other.this) if other is not None else False
    def __ne__(self,other):
      return (self.this != other.this) if other is not None else True
    def __nonzero__(self):
      return True
%}
}
