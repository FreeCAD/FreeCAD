%rename(SoDelayQueueSensor_scb_v) SoDelayQueueSensor::SoDelayQueueSensor(SoSensorCB * func, void * data);

%feature("shadow") SoDelayQueueSensor::SoDelayQueueSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoDelayQueueSensor *"))
      newobj = _coin.new_SoDelayQueueSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoDelayQueueSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
