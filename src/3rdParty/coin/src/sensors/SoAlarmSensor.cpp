/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SoAlarmSensor SoAlarmSensor.h Inventor/sensors/SoAlarmSensor.h
  \brief The SoAlarmSensor class is a sensor which will trigger once at a specified time.

  \ingroup coin_sensors

  SoAlarmSensor provides a convenient way of setting up triggers for
  jobs which should be executed only once when they are scheduled.
 */

#include <Inventor/sensors/SoAlarmSensor.h>
#include <cassert>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  Default constructor.
 */
SoAlarmSensor::SoAlarmSensor(void)
{
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoAlarmSensor::SoAlarmSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
}

/*!
  Destructor.
*/
SoAlarmSensor::~SoAlarmSensor(void)
{
}

/*!
  Set the time at which the sensor will trigger.

  Note that you must manually schedule() the sensor after calling this
  method.

  \sa setTimeFromNow(), getTime()
 */
void
SoAlarmSensor::setTime(const SbTime & abstime)
{
  this->alarm = abstime;
}

/*!
  Set the alarm to be triggered at a specified offset from the current time.

  Note that you must manually schedule() the sensor after calling this
  method.

  \sa setTime(), getTime()
 */
void
SoAlarmSensor::setTimeFromNow(const SbTime & reltime)
{
  this->alarm.setToTimeOfDay();
  this->alarm += reltime;
}

/*!
  Returns the trigger time for the alarm as an absolute value from
  1970-01-01 00:00:00.

  \sa setTime(), setTimeFromNow()
 */
const SbTime &
SoAlarmSensor::getTime(void) const
{
  return this->alarm;
}

// Doc from superclass.
void
SoAlarmSensor::schedule(void)
{
  this->setTriggerTime(this->alarm);
  inherited::schedule();
}
