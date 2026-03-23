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
  \class SoFieldSensor SoFieldSensor.h Inventor/sensors/SoFieldSensor.h
  \brief The SoFieldSensor class detects changes to a field.

  \ingroup coin_sensors

  Attach a field to a sensor of this type to put it under
  surveillance, so you can act upon changes to the field.

  An SoFieldSensor can also act for delete-callback purposes alone and
  does not need a regular notification-based callback.
*/

#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/fields/SoField.h>


/*!
  Constructor.
 */
SoFieldSensor::SoFieldSensor(void)
{
  this->convict = NULL;
}

/*!
  Constructor taking as parameters the sensor callback function and
  the userdata which will be passed to the callback.

  \sa setFunction(), setData()
 */
SoFieldSensor::SoFieldSensor(SoSensorCB * func, void * data)
  : inherited(func, data)
{
  this->convict = NULL;
}

/*!
  Destructor.
*/
SoFieldSensor::~SoFieldSensor(void)
{
  if (this->convict) this->detach();
}

/*!
  Attach sensor to a field. Whenever the field's value changes, the
  sensor will be triggered and call the callback function.

  A field sensor can be attached to only a single field at a
  time. When this method is invoked multiple times, each subsequent
  call will replace the field the sensor is monitoring with the new \a
  field.

  When the given field is deleted, the sensor will automatically be
  detached.

  \sa detach()
 */
void
SoFieldSensor::attach(SoField * field)
{
  if (this->convict) this->detach();
  this->convict = field;
  field->addAuditor(this, SoNotRec::SENSOR);
  field->evaluate();
}

/*!
  Detach sensor from field. As long as an SoFieldSensor is detached,
  it will never call its callback function.

  \sa attach()
 */
void
SoFieldSensor::detach(void)
{
  if (this->convict) {
    this->convict->removeAuditor(this, SoNotRec::SENSOR);
    this->convict = NULL;
    if (this->isScheduled()) this->unschedule();
  }
}

/*!
  Returns a pointer to the field connected to the sensor.

  \sa attach(), detach()
 */
SoField *
SoFieldSensor::getAttachedField(void) const
{
  return this->convict;
}

// Doc from superclass.
void
SoFieldSensor::trigger(void)
{
  this->convict->evaluate();
  inherited::trigger();
}

// Doc from superclass.
void
SoFieldSensor::notify(SoNotList * l)
{
  // Overridden to only propagate if the field that caused the
  // notification is the one this sensor is attached to.
  if (l->getLastField() == this->convict) {
    inherited::notify(l);
  }
}

// Doc from superclass.
void
SoFieldSensor::dyingReference(void)
{
  SoFieldContainer * dyingcontainer = this->getAttachedField() ?
    this->getAttachedField()->getContainer() : NULL;

  this->invokeDeleteCallback();

  if (this->getAttachedField() &&
      this->getAttachedField()->getContainer() == dyingcontainer) {
    // sensor is attached, and to the same field-container
    this->detach();
    // FIXME: we could maybe do an exception for the globalfield-container,
    // and for loose fields I assume have NULL for getContainer() - those cases
    // should be checked at the field-pointer level instead.
  }
}
