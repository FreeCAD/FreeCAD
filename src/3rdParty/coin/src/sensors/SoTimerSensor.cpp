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
  \class SoTimerSensor SoTimerSensor.h Inventor/sensors/SoTimerSensor.h
  \brief The SoTimerSensor class is a sensor which will trigger at given intervals.

  \ingroup coin_sensors

  Use sensors of this class when you want a job repeated at a certain
  interval, without explicitly needing to reschedule the sensor
  (i.e. SoTimerSensor automatically reschedules itself after it has
  been triggered).

  SoTimerSensor instances are commonly used to trigger animation
  updates at a constant frame rate.
*/

#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/SoDB.h>
#include <cassert>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#define DEBUG_TIMERSENSOR_TRACE 0

/*!
  Default constructor. Sets up an interval of 1/30th of a second.
 */
SoTimerSensor::SoTimerSensor(void)
{
  this->interval.setValue(1.0f/30.0f);
  this->setbasetime = FALSE;
  this->istriggering = FALSE;
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoTimerSensor::SoTimerSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
  this->interval.setValue(1.0f/30.0f);
  this->setbasetime = FALSE;
  this->istriggering = FALSE;
}

/*!
  Destructor.
*/
SoTimerSensor::~SoTimerSensor(void)
{
  if (this->isScheduled()) this->unschedule();
}

/*!
  Set the \a base trigger time.

  If you use this method, the trigger times will be on intervals from
  the given value.

  Without an explicitly set base time, the next trigger invocation
  after a trigger has happened will be on the current time plus the
  interval time.  Note that this will of course cause the timer to
  drift.

  \sa getBaseTime()
 */
void
SoTimerSensor::setBaseTime(const SbTime & baseref)
{
  //FIXME: There is some humbug when setting baseTime to 0. Probably a
  //floating point precision thing. Investigate. wiesener 20100810
  this->base = baseref;
  this->setbasetime = TRUE;
}

/*!
  Returns the base trigger time.

  \sa setBaseTime()
 */
const SbTime &
SoTimerSensor::getBaseTime(void) const
{
  return this->base;
}

/*!
  Sets the time interval between each time the sensor triggers.

  \sa getInterval()
 */
void
SoTimerSensor::setInterval(const SbTime & intervalref)
{
  this->interval = intervalref;
}

/*!
  Returns the timer trigger interval.

  \sa setInterval()
 */
const SbTime &
SoTimerSensor::getInterval(void) const
{
  return this->interval;
}

/*!
  Set new trigger time based on the given schedule time,
  and schedules the sensor for triggering.
*/
void
SoTimerSensor::reschedule(const SbTime & schedtime)
{
  this->scheduled = FALSE;
  this->istriggering = FALSE;

  if (!this->setbasetime) {
    this->base = schedtime;
    this->setTriggerTime(this->base + this->interval);
#if DEBUG_TIMERSENSOR_TRACE // debug
    SoDebugError::postInfo("SoTimerSensor::reschedule",
                           "(setbasetime) base: %lf, new trigger time: %lf",
                           this->base.getValue(), this->getTriggerTime().getValue());
#endif // debug
  }
  else {
    int intervals = (int)((schedtime - this->base)/this->interval) + 1;

    if ( intervals < 0 ) 
      intervals = 0;
    
    this->setTriggerTime(this->base + intervals * this->interval);

#if DEBUG_TIMERSENSOR_TRACE
    SoDebugError::postInfo("SoTimerSensor::reschedule",
                           "base: %lf, new trigger time: %lf",
                           this->base.getValue(), this->getTriggerTime().getValue());
#endif
  }

  // don't call this node's schedule as it calls this method
  inherited::schedule();
}

/*!
  Overrides the virtual schedule() method to be able to set up the
  base time, if this was not done by the user.

  If no base time was set, base time will then equal the current time.

  \sa unschedule(), isScheduled()
 */
void
SoTimerSensor::schedule(void)
{
#if DEBUG_TIMERSENSOR_TRACE // debug
  SoDebugError::postInfo("SoTimerSensor::schedule", "");
#endif // debug

#if COIN_DEBUG
  if (this->isScheduled()) {
    SoDebugError::postWarning("SoTimerSensor::schedule",
                              "was already scheduled!");
    return;
  }
#endif // COIN_DEBUG

  // need to handle the case where the callback has unscheduled
  // the timer, and then scheduled it again. Since we are
  // triggering, we can't just reschedule, but need to add
  // it to SoSensorManager's list of rescheduled timers.
  if (this->istriggering) {
    // shouldn't get here if we're scheduled, but test anyway
    if (!this->scheduled) {
      SoDB::getSensorManager()->rescheduleTimer(this);
    }
  }
  else {
    this->reschedule(SbTime::getTimeOfDay());
  }
}

/*!
  Overrides the virtual unschedule() method to handle unschedule()
  calls during triggering.
 */
void
SoTimerSensor::unschedule(void)
{
#if DEBUG_TIMERSENSOR_TRACE // debug
  SoDebugError::postInfo("SoTimerSensor::unschedule", "%p start", this);
#endif // debug

#if COIN_DEBUG
  if (!this->isScheduled()) {
    SoDebugError::postWarning("SoTimerSensor::unschedule",
                              "%p not scheduled (istriggering=%s)",
                              this, this->istriggering ? "TRUE" : "FALSE");
    return;
  }
#endif // COIN_DEBUG

  if (this->istriggering) {
    SoDB::getSensorManager()->removeRescheduledTimer(this);
    this->scheduled = FALSE;
    this->istriggering = FALSE;
  }
  else inherited::unschedule();

#if DEBUG_TIMERSENSOR_TRACE // debug
  SoDebugError::postInfo("SoTimerSensor::unschedule", "%p done", this);
#endif // debug
}

/*!
  Overrides the virtual trigger() method to be able to reschedule
  ourselves after we've been triggered.
*/
void
SoTimerSensor::trigger(void)
{
#if DEBUG_TIMERSENSOR_TRACE // debug
  SoDebugError::postInfo("SoTimerSensor::trigger", "%p start", this);
#endif // debug

  this->istriggering = TRUE;
  // This will cause SoSceneManager to reschedule this timer after
  // the current queue has been processed.
  SoDB::getSensorManager()->rescheduleTimer(this);

  // don't call SoTimerQueueSensor::trigger() as it will clear
  // the scheduled flag.
  SoSensor::trigger();
  
#if DEBUG_TIMERSENSOR_TRACE // debug
  SoDebugError::postInfo("SoTimerSensor::trigger", "%p done", this);
#endif // debug
}
