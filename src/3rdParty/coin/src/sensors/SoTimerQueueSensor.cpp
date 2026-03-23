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
  \class SoTimerQueueSensor SoTimerQueueSensor.h Inventor/sensors/SoTimerQueueSensor.h
  \brief The SoTimerQueueSensor class is the abstract base class for sensors triggering on certain timer events.

  \ingroup coin_sensors

  Timer sensors triggers upon specific points in time.

  This class is an abstract superclass which collects the common
  interface of the various non-abstract timer sensor classes. See the
  documentation of the subclasses for information on what ways there
  are to specify base times, intervals, alarm-style single triggering,
  repeated triggers, etc.

  Note that Coin timer sensors should in no way be considered "hard
  real time". That is, you \e cannot expect a timer to always trigger
  at the exact moment it was set up for. Delays in triggering could be
  due to other activities in Coin, a task suspended, or heavy load
  from other applications on the system. These situations could all
  cause the processing of sensor queues (from SoQt / SoWin / SoXt /
  whatever) to be slightly delayed, thereby causing delays in timer
  sensor triggering.

  On modern systems, a timer will usually trigger within a few
  milliseconds of its designated time, though.

  If a timer sensor cannot trigger at the exact moment it has been
  scheduled, it will be triggered at the first opportunity after the
  scheduled time has passed.

  Here's a simple usage example. It's a standalone example, which
  only demonstrates how to set up a repeating timer sensor with a
  callback:

  \code
  #include <Inventor/Xt/SoXt.h>
  #include <Inventor/sensors/SoTimerSensor.h>
  #include <cstdio>
  
  static void
  timeSensorCallback(void * data, SoSensor * sensor)
  {
    SbTime time = SbTime::getTimeOfDay();
    SbString string = time.format("%S.%i");
    (void)printf("%s\n", string.getString());
  }
  
  
  int
  main(int argc, char ** argv)
  { 
    SoXt::init("test");
  
    SoTimerSensor * timeSensor = new SoTimerSensor;
    timeSensor->setFunction(timeSensorCallback);
    timeSensor->setBaseTime(SbTime::getTimeOfDay());
    timeSensor->setInterval(1.0f);
    timeSensor->schedule();
  
    SoXt::mainLoop();
    return 0;
  }
  \endcode
*/

#include <Inventor/sensors/SoTimerQueueSensor.h>
#include <Inventor/SoDB.h>
#include <cassert>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \var SbBool SoTimerQueueSensor::scheduled
  \c TRUE if the sensor is currently scheduled.
*/


/*!
  Default constructor.
 */
SoTimerQueueSensor::SoTimerQueueSensor(void)
  : scheduled(FALSE)
{
}

/*!
  Constructor taking as arguments the sensor callback function and the
  userdata which will be passed the callback.

  \sa setFunction(), setData()
 */
SoTimerQueueSensor::SoTimerQueueSensor(SoSensorCB * func, void * data)
  : inherited(func, data), scheduled(FALSE)
{
}

/*!
  Destructor.
*/
SoTimerQueueSensor::~SoTimerQueueSensor(void)
{
  // Note: it won't work to move this to the SoSensor destructor
  // (where it really belongs), because you can't use pure virtual
  // methods from a destructor.
  if (this->isScheduled()) this->unschedule();
}

/*!
  Returns the time at which the sensor will trigger.

  \sa setTriggerTime()
 */
const SbTime &
SoTimerQueueSensor::getTriggerTime(void) const
{
  return this->triggertime;
}

/*!
  Set absolute time at which to trigger the sensor.

  \sa getTriggerTime()
 */
void
SoTimerQueueSensor::setTriggerTime(const SbTime & time)
{
  if (time != this->triggertime) {
    this->triggertime = time;
    if (this->isScheduled()) {
      SoSensorManager * sm = SoDB::getSensorManager();
      sm->removeTimerSensor(this);
      sm->insertTimerSensor(this);
    }
  }
}

// Documented in superclass.
void
SoTimerQueueSensor::trigger(void)
{
  this->scheduled = FALSE;
  inherited::trigger();
}

/*!
  Put the sensor in the global timer queue.

  \sa unschedule(), isScheduled()
 */
void
SoTimerQueueSensor::schedule(void)
{
#if COIN_DEBUG
  assert(this->scheduled == TRUE || this->scheduled == FALSE);
  if (this->isScheduled()) {
    SoDebugError::postWarning("SoTimerQueueSensor::schedule",
                              "already scheduled!");
    return;
  }
#endif // COIN_DEBUG

  SoDB::getSensorManager()->insertTimerSensor(this);
  this->scheduled = TRUE;
}

/*!
  Remove sensor from the timer queue, without triggering it first.

  \sa schedule(), isScheduled()
 */
void
SoTimerQueueSensor::unschedule(void)
{
#if COIN_DEBUG
  assert(this->scheduled == TRUE || this->scheduled == FALSE);
  if (!this->isScheduled()) {
    SoDebugError::postWarning("SoTimerQueueSensor::unschedule",
                              "not scheduled!");
    return;
  }
#endif // COIN_DEBUG
  SoDB::getSensorManager()->removeTimerSensor(this);
  this->scheduled = FALSE;
}

/*!
  Check if this sensor is scheduled for triggering.

  \sa schedule(), unschedule()
 */
SbBool
SoTimerQueueSensor::isScheduled(void) const
{
  return this->scheduled;
}

SbBool
SoTimerQueueSensor::isBefore(const SoSensor * s) const
{
  return (this->triggertime < ((SoTimerQueueSensor *)s)->triggertime);
}
