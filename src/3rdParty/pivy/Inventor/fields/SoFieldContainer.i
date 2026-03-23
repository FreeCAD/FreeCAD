%extend SoFieldContainer {
    PyObject * getFieldName(SoField * field) {
        SbName name;
        if (!self->getFieldName(field, name)) {
            Py_INCREF(Py_None);
            return Py_None;
        }
    return Py_BuildValue("s", name.getString());
}
  
/* add generic interface to access fields as attributes */
%pythoncode %{
    def __getattr__(self, name):
        try:
            return SoBase.__getattribute__(self, name)
        except AttributeError as e:
            ##############################################################
            if name == "this":
                raise AttributeError
            ##############################################################
            field = self.getField(name)
            if field is None:
                raise e
            return field
          
    def __setattr__(self, name, value):
        # I don't understand why we need this, but otherwise it does not work :/
        if name == 'this':
            return SoBase.__setattr__(self, name, value)
        field = self.getField(name)
        if field is None:
            return SoBase.__setattr__(self, name, value)
        field.setValue(value)
        return field

    def __dir__(self):
        from pivy import coin
        fl = coin.SoFieldList()
        num_fields = self.getAllFields(fl)
        fields = [self.getFieldName(fl[i]) for i in range(num_fields)]
        return super(SoFieldContainer, self).__dir__() + fields


    @property
    def values(self):
        def _values(obj):
            for value in obj:
                if hasattr(value, "__iter__"):
                    yield list(_values(value))
                else:
                    yield value
        out = _values(self)
        return list(out)

    @values.setter
    def values(self, arr):
        self.deleteValues(0)
        self.setValues(0, len(arr), arr)

%}
}
