/* add generic interface to access parts as attributes */
%extend SoBaseKit {
%pythoncode %{
    def __getattr__(self,name):
       if name == 'this':
          return SoNode.__getattr__(self,name)
       c = _coin.SoBaseKit_getNodekitCatalog(self)
       if c.getPartNumber(name) >= 0:
           part = self.getPart(name,1)
           return part
       return SoNode.__getattr__(self, name)

    def __setattr__(self,name,value):
       if name == 'this':
          return SoNode.__setattr__(self,name,value)
       c = _coin.SoBaseKit_getNodekitCatalog(self)
       if c.getPartNumber(name) >= 0:
          return self.setPart(name, value)
       return SoNode.__setattr__(self,name,value)       
%}
}
