%rename(SoAlarmSensor_scb_v) SoAlarmSensor::SoAlarmSensor(SoSensorCB * func, void * data);

%feature("shadow") SoAlarmSensor::SoAlarmSensor %{
def __init__(self, *args):
   newobj = None
   if len(args) == 2:
      args = (args[0], (args[0], args[1], "SoAlarmSensor *"))
      newobj = _coin.new_SoAlarmSensor_scb_v(*args)
   else:
      newobj = _coin.new_SoAlarmSensor(*args)
   if newobj:
      self.this = newobj.this
      self.thisown = 1
%}
