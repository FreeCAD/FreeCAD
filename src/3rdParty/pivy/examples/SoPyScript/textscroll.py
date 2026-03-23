from __future__ import print_function
# example that shows a scrolling text using a timer sensor
idx = 0
text = string.getValue().getString()
text_length = len(text)

interval = 0.15

def changeStringSensorCallback(data, sensor):
  global idx
  string.setValue(text[:idx])

  if idx == text_length:
    sensor.setInterval(5.0)
  else:
    sensor.setInterval(interval)

  idx %= text_length
  idx += 1

timersensor = SoTimerSensor(changeStringSensorCallback, None)
timersensor.setInterval(interval)
timersensor.schedule()

wa = SoWriteAction()
wa.apply(self)

string.setValue(text[:idx])

print('== TextScroller script loaded ==')
