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
  \class SoDelayQueueSensor SoDelayQueueSensor.h Inventor/sensors/SoDelayQueueSensor.h
  \brief The SoDelayQueueSensor class is the abstract base class for priority scheduled sensors.

  \ingroup coin_sensors

  Delay queue sensors are invoked upon various events \e not related
  to time occurrences. See documentation of subclasses to see which
  types of events can be surveilled by the built-in sensor types.

  The priority values can be used to queue events by their importance,
  so the sensors are triggered in the sequence you want.

  The queue of delay sensors (i.e. instances of subclasses of
  SoDelayQueueSensor) will be processed as soon as either the runtime
  system is idle, or if it is continually busy they will be processed
  within a fixed amount of time.

  This time interval is by default 1/12th of a second, but can be
  controlled with the SoSensorManager::setDelaySensorTimeout()
  interface.
*/

#include <cassert>

#include <Inventor/sensors/SoDelayQueueSensor.h>
#include <Inventor/SoDB.h>
#include <Inventor/errors/SoDebugError.h>


/*!
  \var SbBool SoDelayQueueSensor::scheduled
  \c TRUE if the sensor is currently scheduled.
*/

/*!
  Default constructor.
 */
SoDelayQueueSensor::SoDelayQueueSensor(void)
{
  this->scheduled = FALSE;
  this->priority = SoDelayQueueSensor::getDefaultPriority();
}

/*!
  Constructor taking as arguments the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoDelayQueueSensor::SoDelayQueueSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
  this->scheduled = FALSE;
  this->priority = SoDelayQueueSensor::getDefaultPriority();
}

/*!
  Destructor.
*/
SoDelayQueueSensor::~SoDelayQueueSensor(void)
{
  // NB: it won't work to move this to the SoSensor destructor (where
  // it really belongs), because you can't use pure virtual methods
  // from a destructor.
  if (this->isScheduled()) this->unschedule();
}

/*!
  Set this sensor's priority in the scheduling queue. When sensors
  are processed, high priority sensors will trigger before low
  priority sensors. 

  Please note that a low number means a high priority. A sensor with
  priority 5 will trigger before a sensor with priority 6.

  Sensors with priority 0 have a special meaning in Coin/Inventor.
  They are called immediate sensors, and will trigger immediately
  after the current notification chain has finished. Priority 0
  sensors should be used with care, since they might lead to bad
  performance if you do some processing in the sensor callback.

  This setting does not affect SoDataSensor delete-callback
  invocations, which always are immediate.

  \sa getPriority(), getDefaultPriority()
*/
void
SoDelayQueueSensor::setPriority(uint32_t pri)
{
  if (this->priority != pri) {
    this->priority = pri;
    if (this->isScheduled()) {
      this->unschedule();
      this->schedule();
    }
  }
}

/*!
  Returns sensor scheduling priority.

  \sa setPriority(), getDefaultPriority()
 */
uint32_t
SoDelayQueueSensor::getPriority(void) const
{
  return this->priority;
}

/*!
  Returns the default scheduling priority value. The default
  sensor priority is 100.

  \sa getPriority()
 */
uint32_t
SoDelayQueueSensor::getDefaultPriority(void)
{
  return 100;
}

// Documented in superclass.
void
SoDelayQueueSensor::trigger(void)
{
  // Overridden to clear scheduled flag before triggering.

  this->scheduled = FALSE;
  inherited::trigger();
}


/*!
  Put the sensor in the global delay queue. This means it will be
  triggered either when the CPU is idle, or when the specified delay
  queue timeout is reached.

  \sa SoDB::setDelaySensorTimeout(), unschedule(), isScheduled()
 */
void
SoDelayQueueSensor::schedule(void)
{
  if (!this->scheduled) {
    SoDB::getSensorManager()->insertDelaySensor(this);
    this->scheduled = TRUE;
  }
}

/*!
  Remove sensor from the delay queue, without triggering it first.

  \sa schedule(), isScheduled()
 */
void
SoDelayQueueSensor::unschedule(void)
{
#if COIN_DEBUG
  if (!this->isScheduled()) {
    SoDebugError::postWarning("SoDelayQueueSensor::unschedule",
                              "tried to unschedule a sensor which is "
                              "not scheduled");
    return;
  }
#endif // COIN_DEBUG

  SoDB::getSensorManager()->removeDelaySensor(this);
  this->scheduled = FALSE;
}

/*!
  Check if this sensor is scheduled for triggering.

  \sa schedule(), unschedule()
 */
SbBool
SoDelayQueueSensor::isScheduled(void) const
{
  return this->scheduled;
}

/*!
  Returns a flag indicating whether or not the sensor should only be
  triggered if the application is truly idle, and \e not when the
  delay queue is processed because of the delay queue timeout.

  \sa SoDB::setDelaySensorTimeout()
 */
SbBool
SoDelayQueueSensor::isIdleOnly(void) const
{
  return FALSE;
}

SbBool
SoDelayQueueSensor::isBefore(const SoSensor * s) const
{
  return (this->priority < ((SoDelayQueueSensor *)s)->priority);
}
