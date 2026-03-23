%extend SoGroup {
/* extend __iter__ to return a new iterator object */
%pythoncode %{
    def __iter__(self):
        i = 0
        while i < self.getNumChildren():
            yield self.getChild(i)
            i += 1

    def __iadd__(self, other):
        if isinstance(other, (list, tuple)):
            for other_i in other:
                self.__iadd__(other_i)
            return self
        else:
            try:
                self.addChild(other)
                return self
            except TypeError as e:
                raise TypeError(str(self.__class__) + " accepts only objects of type pivy.coin.SoNode")

    def __isub__(self, other):
        if isinstance(other, (list, tuple)):
            for other_i in other:
                self.__isub__(other_i)
            return self
        else:
            try:
                self.removeChild(other)
                return self
            except TypeError as e:
                raise TypeError(str(self.__class__) + " can't remove child of type " + str(type(other)))

    def getByName(self, name):
        for child in self:
            if name == child.getName():
                return child
        return None

    def __bool__(self):
        return True
%}

  /* methods to emulate Python Container object */
  int __len__(void) { return self->getNumChildren(); }
  int __contains__(const SoNode * node ) { return (self->findChild(node) != -1); }
  SoNode * __getitem__(int index) {
        if (index >= 0 && index < self->getNumChildren())
        {
            return self->getChild(index); 
        }
        else{
            PyErr_SetString(PyExc_IndexError,"index out of bounds");
            PyErr_Print();
            return NULL;
        }
    }
}
