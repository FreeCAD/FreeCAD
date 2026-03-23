/* add generic interface to access outputs as attributes */
%extend SoEngine {
%pythoncode %{
    def __getattr__(self, name):
        try:
            return SoFieldContainer.__getattr__(self, name)
        except AttributeError as e:
            ##############################################################
            if name == "this":
                raise AttributeError
            ##############################################################
            out = self.getOutput(SbName(name))
            if out is None:
                raise e
            return out
    
    def __setattr__(self,name,value):
        if name == 'this':
            return SoFieldContainer.__setattr__(self, name, value)
        out = self.getOutput(SbName(name))
        if out is None:
            return SoFieldContainer.__setattr__(self, name, value)
        raise AttributeError('Cannot set output %s on engine %s' %(name,self.__class__.__name__))
        
%}
}
