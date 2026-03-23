%rename(SoFieldSensor_scb_v) SoFieldSensor::SoFieldSensor(SoSensorCB * func, void * data);

%feature("shadow") SoFieldSensor::SoFieldSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoFieldSensor *"))
      newobj = _coin.new_SoFieldSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoFieldSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
