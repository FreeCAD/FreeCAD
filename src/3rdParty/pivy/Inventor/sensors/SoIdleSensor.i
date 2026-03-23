%rename(SoIdleSensor_scb_v) SoIdleSensor::SoIdleSensor(SoSensorCB * func, void * data);

%feature("shadow") SoIdleSensor::SoIdleSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoIdleSensor *"))
      newobj = _coin.new_SoIdleSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoIdleSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
