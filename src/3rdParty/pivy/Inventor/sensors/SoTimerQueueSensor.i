%rename(SoTimerQueueSensor_scb_v) SoTimerQueueSensor::SoTimerQueueSensor(SoSensorCB * func, void * data);

%feature("shadow") SoTimerQueueSensor::SoTimerQueueSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoTimerQueueSensor *"))
      newobj = _coin.new_SoTimerQueueSensor_scb_v(*args)
   else:
      self.this = _coin.new_SoTimerQueueSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
