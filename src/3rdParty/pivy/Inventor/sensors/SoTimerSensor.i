%rename(SoTimerSensor_scb_v) SoTimerSensor::SoTimerSensor(SoSensorCB * func, void * data);

%feature("shadow") SoTimerSensor::SoTimerSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoTimerSensor *"))
      newobj = _coin.new_SoTimerSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoTimerSensor(*args)


   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}

