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
  \class SoCounter SoCounter.h Inventor/engines/SoCounter.h
  \brief The SoCounter class is an integer counter engine.

  \ingroup coin_engines

  The engine counts from its \a min value to its \a max value, adding
  the value of \a step each time \a trigger is touched.

  SoCounter provides a convenient way of keeping track of the number
  of times some event in your application is triggered.
*/

/*!
  \var SoSFShort SoCounter::min
  Minimum value for counter.
*/

/*!
  \var SoSFShort SoCounter::max
  Maximum value for counter.
*/

/*!
  \var SoSFShort SoCounter::step
  Counter step value.
*/

/*!
  \var SoSFTrigger SoCounter::trigger
  Increment counter (using step).
*/

/*!
  \var SoSFShort SoCounter::reset
  Reset counter to this value. The value will be clamped between min
  and max, and step will be accounted for.
*/

/*!
  \var SoEngineOutput SoCounter::output
  (SoSFShort) Output which contains the current counter value.
*/


/*!
  \var SoEngineOutput SoCounter::syncOut
  (SoSFTrigger) Triggers every time counter restarts at SoCounter::min.
*/

#include <Inventor/engines/SoCounter.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoCounter);

/*!
  Default constructor.
*/
SoCounter::SoCounter(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoCounter);

  SO_ENGINE_ADD_INPUT(min, (0));
  SO_ENGINE_ADD_INPUT(max, (1));
  SO_ENGINE_ADD_INPUT(step, (1));
  SO_ENGINE_ADD_INPUT(trigger, ());
  SO_ENGINE_ADD_INPUT(reset, (0));

  SO_ENGINE_ADD_OUTPUT(output, SoSFShort);
  SO_ENGINE_ADD_OUTPUT(syncOut, SoSFTrigger);

  this->syncOut.enable(FALSE);  // Disable notification

  this->value = 0;
  this->numsteps = 0;
}

/*!
  Destructor.

  Protected, as engines are not supposed to be explicitly deleted, but
  rather take care of their own destiny by monitoring their reference
  count.
*/
SoCounter::~SoCounter()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCounter::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoCounter);
}

// doc in parent
void
SoCounter::evaluate()
{
  SO_ENGINE_OUTPUT(output,SoSFShort,setValue(this->value));
}

// doc in parent
void
SoCounter::inputChanged(SoField *which)
{
  // Default to not notifying fields connected to the syncOut output.
  this->syncOut.enable(FALSE);

  if (which == &this->trigger) {
    this->numsteps += 1;
    this->value += this->step.getValue();
    if (this->value > this->max.getValue()) {
      this->value = this->min.getValue();
      this->numsteps = 0;
      this->syncOut.enable(TRUE);
    }
    else if (this->value < this->min.getValue()) {
      this->value = this->max.getValue();
      this->numsteps = 0;
      this->syncOut.enable(TRUE);
    }
  }
  else if (which == &this->reset) {
    short minval = this->min.getValue();
    short maxval = this->max.getValue();
    short resetval = this->reset.getValue();
    if (resetval < minval) {
      this->numsteps = 0;
      this->value = minval;
    }
    else {
      if (resetval > maxval) resetval = maxval;
      short stepval = this->step.getValue();
      this->numsteps = (resetval - minval) / stepval;
      this->value = minval + this->numsteps * stepval;
    }
  }
  else if (which == &this->max) {
    if (this->max.getValue() < this->min.getValue()) {
      this->min.setValue(this->max.getValue());
      this->value = this->max.getValue();
      this->numsteps = 0;
    }
    if (this->max.getValue() < this->value) {
      this->value = this->min.getValue();
      this->numsteps = 0;
    }
  }
  else if (which == &this->min) {
    if (this->max.getValue() < this->min.getValue()) {
      this->max.setValue(this->min.getValue());
      this->numsteps = 0;
    }
    this->value = this->min.getValue() + this->step.getValue() * this->numsteps;
    if (this->value > this->max.getValue()) {
      this->numsteps = 0;
      this->value = this->min.getValue();
    }
  }
  else if (which == &this->step) {
    this->value = this->min.getValue() + this->step.getValue() * this->numsteps;
    if (this->value > this->max.getValue()) {
      this->numsteps = 0;
      this->value = this->min.getValue();
    }
  }
}
