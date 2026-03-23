%rename(SoPathSensor_scb_v) SoPathSensor::SoPathSensor(SoSensorCB * func, void * data);

%feature("shadow") SoPathSensor::SoPathSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoPathSensor *"))
      newobj = _coin.new_SoPathSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoPathSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
