%rename(SoNodeSensor_scb_v) SoNodeSensor::SoNodeSensor(SoSensorCB * func, void * data);

%feature("shadow") SoNodeSensor::SoNodeSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoNodeSensor *"))
      newobj = _coin.new_SoNodeSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoNodeSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
