%rename(SoDataSensor_scb_v) SoDataSensor::SoDataSensor(SoSensorCB * func, void * data);

%feature("shadow") SoDataSensor::SoDataSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoDataSensor *"))
      newobj = _coin.new_SoDataSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoDataSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
