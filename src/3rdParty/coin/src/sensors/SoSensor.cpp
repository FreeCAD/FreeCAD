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
  \class SoSensor SoSensor.h Inventor/sensors/SoSensor.h
  \brief The SoSensor class is the abstract base class for all sensors.

  \ingroup coin_sensors

  Sensors is a mechanism in Coin for scheduling jobs to be run upon
  specific events. The events in question could be particular points
  in time, or changes to entities in the scene graph.

  See documentation of subclasses for insight into exactly for what
  types of events we can trigger sensor callbacks.

  For most tasks where application programmers can use sensors, it is
  also possible to achieve the same results by using engines. There
  are a few important differences between sensors and engines, though:

  Engines are considered part of the scene graph, and are written to
  file upon SoWriteAction export operations. Sensors, on the other
  hand, are not included in export operations.

  Engines basically connects fields (and come with a lot of built-in
  functionality for combining and converting inputs and outputs), you
  just decide which engine you want, connect inputs and output and
  forget about it. Sensors are a lot more flexible in what you can do
  when they trigger, as control is transferred internally from Coin to
  your registered callback functions.

  \sa SoSensorManager, SoEngine
 */

#include <Inventor/sensors/SoSensor.h>
#include <cstdlib> // NULL
#include <coindefs.h> // COIN_OBSOLETED()
#include "misc/SbHash.h"

/*!
  \var SoSensorCB * SoSensor::func
  Function to be called when a sensor triggers.
*/
/*!
  \var void * SoSensor::funcData
  Data passed to the callback function.
*/

/*!
  \fn virtual void SoSensor::schedule(void)

  Put the sensor in a queue to be triggered at a later time.

  \sa unschedule(), isScheduled()
 */
/*!
  \fn virtual void SoSensor::unschedule(void)

  Remove sensor from queue. The sensor will not be triggered unless it
  is later rescheduled.

  \sa schedule(), isScheduled()
 */
/*!
  \fn virtual SbBool SoSensor::isScheduled(void) const

  Check if this sensor is scheduled for triggering.

  \sa schedule(), unschedule()
 */
/*!
  \fn virtual SbBool SoSensor::isBefore(const SoSensor * s) const

  Returns \c TRUE if this sensor should precede sensor \a s in its
  sensor queue.
 */

/*!
  \typedef void SoSensorCB(void * data, SoSensor * sensor)

  Sensor callback functions must have this signature to be valid for
  registering with SoSensor.
*/

unsigned int SbHashFunc(const SoSensor * key) {
  return SbHashFunc(reinterpret_cast<size_t>(key));
}

/*!
  Constructor.
 */
SoSensor::SoSensor(void)
  : func(NULL), funcData(NULL)
{
}

/*!
  Constructor taking as parameters the sensor callback function \a
  func and the user \a data pointer which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoSensor::SoSensor(SoSensorCB * funcptr, void * data)
{
  this->setData(data);
  this->setFunction(funcptr);
}

/*!
  Destructor.
 */
SoSensor::~SoSensor(void)
{
}

/*!
  Set the callback function pointer which will be used when the sensor
  is triggered.

  \sa getFunction(), setData()
 */
void
SoSensor::setFunction(SoSensorCB * callbackfunction)
{
  this->func = callbackfunction;
}

/*!
  Returns the callback function pointer.

  \sa setFunction()
 */
SoSensorCB *
SoSensor::getFunction(void) const
{
  return this->func;
}

/*!
  Set the user supplied data pointer which will be used as the first
  argument to the sensor callback function.

  \sa getData(), setFunction()
 */
void
SoSensor::setData(void * callbackdata)
{
  this->funcData = callbackdata;
}

/*!
  Returns the user supplied callback function data pointer.

  \sa setData()
 */
void *
SoSensor::getData(void) const
{
  return this->funcData;
}

/*!
  Trigger the sensor's callback function.
 */
void
SoSensor::trigger(void)
{
  if (this->func) this->func(this->funcData, this);
}

/*!
  \COININTERNAL
  Open Inventor function not implemented in Coin.
 */
void
SoSensor::setNextInQueue(SoSensor * COIN_UNUSED_ARG(next))
{
  COIN_OBSOLETED();
}

/*!
  \COININTERNAL
  Open Inventor function not implemented in Coin.
 */
SoSensor *
SoSensor::getNextInQueue(void) const
{
  COIN_OBSOLETED();
  return NULL;
}

/*!
  Sets up initialization for static data for the sensors. Called by
  SoDB::init().
 */
void
SoSensor::initClass(void)
{
}
