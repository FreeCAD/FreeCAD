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
  \class SoTimeCounter SoTimeCounter.h Inventor/engines/SoTimeCounter.h
  \brief The SoTimeCounter class is an integer counter engine.

  \ingroup coin_engines

  The SoTimeCounter engine is by default connected to the realTime
  global field, and does stepwise updates on the SoTimeCounter::output
  field according to the field settings.
*/

/*!
  \var SoSFTime SoTimeCounter::timeIn
  Running time. Connected to the \e realTime field by default.
*/

/*!
  \var SoSFShort SoTimeCounter::min

  Minimum counter value. Default value is 0.
*/

/*!
  \var SoSFShort SoTimeCounter::max

  Maximum counter value. Default value is 1.
*/

/*!
  \var SoSFShort SoTimeCounter::step

  Counter step size. Default value is 1.
*/

/*!
  \var SoSFBool SoTimeCounter::on

  Set to \c FALSE to pause the counter. Default value is \c TRUE.
*/

/*!
  \var SoSFFloat SoTimeCounter::frequency

  Number of complete cycles from the min value to the max value per
  second. Default value is 1.0.
*/

/*!
  \var SoMFFloat SoTimeCounter::duty

  Used to weight step times. Supply one weight value per step. Default
  array is a single value of 1.0.
*/

/*!
  \var SoSFShort SoTimeCounter::reset

  Manually set the counter to some value. If SoTimeCounter::reset is
  set below the SoTimeCounter::min value or above the
  SoTimeCounter::max value it will be clamped to the closest boundary
  value.
*/

/*!
  \var SoSFTrigger SoTimeCounter::syncIn
  Restart counter at the minimum value.
*/

/*!
  \var SoEngineOutput SoTimeCounter::output
  (SoSFShort) The counter value.
*/

/*!
  \var SoEngineOutput SoTimeCounter::syncOut
  (SoSFTrigger) Triggers every cycle start.
*/

// *************************************************************************

#include <Inventor/engines/SoTimeCounter.h>

#include "coindefs.h"
#include "SbBasicP.h"
#include "engines/SoSubEngineP.h"

#include <Inventor/SoDB.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SoEngineOutputList.h>

// *************************************************************************

SO_ENGINE_SOURCE(SoTimeCounter);

// *************************************************************************

/*!
  Default constructor.
*/
SoTimeCounter::SoTimeCounter(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoTimeCounter);

  SO_ENGINE_ADD_INPUT(timeIn, (SbTime::zero()));
  SO_ENGINE_ADD_INPUT(min, (0));
  SO_ENGINE_ADD_INPUT(max, (1));
  SO_ENGINE_ADD_INPUT(step, (1));
  SO_ENGINE_ADD_INPUT(on, (TRUE));
  SO_ENGINE_ADD_INPUT(frequency, (1.0f));
  SO_ENGINE_ADD_INPUT(duty, (1.0f));
  SO_ENGINE_ADD_INPUT(reset, (0));
  SO_ENGINE_ADD_INPUT(syncIn, ());

  SO_ENGINE_ADD_OUTPUT(output, SoSFShort);
  SO_ENGINE_ADD_OUTPUT(syncOut, SoSFTrigger);

  this->syncOut.enable(FALSE);

  SoField * realtime = SoDB::getGlobalField("realTime");
  this->starttime = coin_assert_cast<SoSFTime *>(realtime)->getValue().getValue();
  this->firstoutputenable = TRUE;
  this->outputvalue = 0;
  this->cyclelen = 1.0;
  this->numsteps = 2;
  this->stepnum = 0;
  this->ispaused = FALSE;

  this->timeIn.connectFrom(realtime);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTimeCounter::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoTimeCounter);
}

/*!
  Destructor.
 */
SoTimeCounter::~SoTimeCounter()
{
}

// *************************************************************************

// Documented in superclass. Overridden to not write connection to
// realTime global field.
void
SoTimeCounter::writeInstance(SoOutput * out)
{
  // Note: the code in this method matches that of SoElapsedTime and
  // SoOneShot, so if any bugs are found and corrected, remember to
  // pass on the updates.

  // Disconnect from realTime field.
  SoField * connectfield = NULL;
  SbBool connectfromrealTime =
    this->timeIn.getConnectedField(connectfield) &&
    connectfield == SoDB::getGlobalField("realTime");
  SbBool defaultflag = this->timeIn.isDefault();
  if (connectfromrealTime) {
    this->timeIn.disconnect();
    this->timeIn.setDefault(TRUE);
  }

  inherited::writeInstance(out);

  // Re-connect to realTime field.
  if (connectfromrealTime) {
    // Don't send notification when reconnecting to preserve the state
    // of the scene graph between write passes.
    this->timeIn.connectFrom(connectfield, TRUE);
    this->timeIn.setDefault(defaultflag);
  }
}

// doc in parent
void
SoTimeCounter::evaluate(void)
{
  if (!this->ispaused && (this->frequency.getValue() > 0.0f)) {
    // FIXME: the code calculating the output value is a
    // mess. 20000919 mortene.

    double currtime = this->timeIn.getValue().getValue();
    double difftime = currtime - this->starttime;
    if (difftime > this->cyclelen) {
      double num = difftime / this->cyclelen;
      this->starttime += this->cyclelen * floor(num);
      difftime = currtime - this->starttime;
    }
    short value = this->findOutputValue(difftime);

    if (value == this->outputvalue + this->step.getValue()) { // common case
      this->stepnum++;
    }
    else { // either reset, wrap-around or a delay somewhere
      short offset = value - this->min.getValue();
      this->stepnum = offset / this->step.getValue();
    }
    this->outputvalue = value;
  }

  // Force update on slave fields (SO_ENGINE_OUTPUT checks
  // isEnabled()-value, and we want the setValue() to happen anyway).
  this->output.enable(TRUE);
  SO_ENGINE_OUTPUT(output, SoSFShort, setValue(this->outputvalue));
  // The isEnabled() flag will be set back to FALSE again upon the
  // next invocation of SoTimeCounter::inputChanged().
}

// doc in parent
void
SoTimeCounter::inputChanged(SoField * which)
{
  // Default is to not do any notification when we return from this
  // function to SoEngine::notify(). This is an optimization for this
  // engine to avoid transmission of notification to all slave fields
  // each time the timeIn field is updated.
  this->output.enable(FALSE);
  this->syncOut.enable(FALSE);

  // Enable outputs on first call.
  if (this->firstoutputenable) {
    this->firstoutputenable = FALSE;
    this->output.enable(TRUE);
    this->syncOut.enable(TRUE);
  }

  // First handle the case where the timeIn field has been changed, as
  // we need to optimize that to avoid too many notifications on the
  // slave fields (we only want to be notified when the output counter
  // value actually changes).
  if (which == &this->timeIn) {
    if (this->ispaused) return;

    double currtime = this->timeIn.getValue().getValue();
    double difftime = currtime - this->starttime;

    if (difftime > this->cyclelen) {
      // Trigger syncOut once at start of cycle.
      this->syncOut.enable(TRUE);

      double num = difftime / this->cyclelen;
      this->starttime += this->cyclelen * floor(num);
      difftime = currtime - this->starttime;
    }

    // Optimize for the common case. This prevents a notification on
    // each and every trigger for timeIn.
    if (this->findOutputValue(difftime) == this->outputvalue) return;
  }

  // FIXME: the code handling the other fields is a horrible
  // mess. 20000919 mortene.

  if (which == &this->on) {
    if (this->on.getValue() && this->ispaused) {
      this->starttime =
        this->timeIn.getValue().getValue() - this->pausetimeincycle;
      this->ispaused = FALSE;
    }
    else if (!this->on.getValue() && !this->ispaused) {
      this->ispaused = TRUE;
      this->pausetimeincycle =
        this->timeIn.getValue().getValue() - this->starttime;
    }
  }
  else if (which == &this->frequency) {
    float freq = this->frequency.getValue();
    if (freq > 0.0f) {
      this->cyclelen = 1.0f / freq;
    }
    else {
      // if freq == 0.0 the engine is more or less disabled. Just set
      // cyclelen to 1.0
      this->cyclelen = 1.0f;
    }
    this->calcDutySteps();
  }
  else if (which == &this->duty) {
    this->calcDutySteps();
  }
  else if (which == &this->reset) {
    short minval = this->min.getValue();
    short val = SbClamp(this->reset.getValue(),
                        minval,
                        this->max.getValue());
    short offset = val - minval;
    short stepval = this->step.getValue();
    if ((offset % stepval) != 0) {
      val = minval + (offset / stepval) * stepval;
    }
    this->calcStarttime(val);
  }
  else if (which == &this->syncIn) {
    this->starttime = this->timeIn.getValue().getValue();
  }
  else if (which == &this->max) {
    if (this->max.getValue() < this->min.getValue())
      this->min.setValue(this->max.getValue());
    this->calcNumSteps();
    this->calcDutySteps();
    if (this->max.getValue() < this->outputvalue) {
      this->starttime = this->timeIn.getValue().getValue();
    }
  }
  else if (which == &this->min) {
    if (this->max.getValue() < this->min.getValue())
      this->max.setValue(this->min.getValue());
    this->calcNumSteps();
    this->calcDutySteps();
    short value = this->min.getValue() + this->step.getValue() * this->stepnum;
    if (value > this->max.getValue()) {
      this->starttime = this->timeIn.getValue().getValue();
    }
  }
  else if (which == &this->step) {
    this->calcNumSteps();
    this->calcDutySteps();
    short value = this->min.getValue() + this->step.getValue() * this->stepnum;
    if (value > this->max.getValue()) {
      this->starttime = this->timeIn.getValue().getValue();
    }
  }

  // Either the timeIn field changed enough to reach a new step, or
  // some of the control fields changed -- so lets notify the slaves.
  //
  // Could also optimize for the case where changes to the control
  // fields doesn't yield an immediate change to the output value, but
  // that doesn't seem worthwhile.
  this->output.enable(TRUE);
}


// calculates # of steps in cycle
void
SoTimeCounter::calcNumSteps(void)
{
  short stepval = this->step.getValue();
  this->numsteps = (this->max.getValue() - this->min.getValue()) / SbAbs(stepval) + 1;
}

// recalculate duty steps.
void
SoTimeCounter::calcDutySteps(void)
{
  if ((this->frequency.getValue() > 0.0f) && (this->duty.getNum() == this->numsteps)) {
    int i;
    double dutysum = 0.0;
    for (i = 0; i < this->numsteps; i++) {
      dutysum += static_cast<double>(this->duty[i]);
    }
    double currsum = 0.0;
    this->dutylimits.truncate(0);
    for (i = 0; i < this->numsteps; i++) {
      currsum += static_cast<double>(this->duty[i]);
      this->dutylimits.append(currsum/dutysum * this->cyclelen);
    }
  }
  else {
    // ignore duty values
    this->dutylimits.truncate(0);
  }
}

// calculates current output value based on time in cycle
short
SoTimeCounter::findOutputValue(double timeincycle) const
{
  assert(timeincycle <= cyclelen);

  short val;
  short minval = this->min.getValue();
  short maxval = this->max.getValue();
  short stepval = this->step.getValue();

  if (this->dutylimits.getLength()) {
    // FIXME: use binary search if > 64 or something values
    int i = 0;
    for (i = 0; i < this->numsteps; i++) {
      if (timeincycle <= this->dutylimits[i]) break;
    }
    if (stepval >= 0) {
      val = minval + i * stepval;
    }
    else {
      val = maxval + i*stepval;
    }
  }
  else {
    double steptime = this->cyclelen / this->numsteps;
    if (stepval >= 0) {
      val = minval + static_cast<short>((timeincycle / steptime) * stepval);
    }
    else {
      val = maxval + static_cast<short>((timeincycle / steptime) * stepval);
    }
    if (val > maxval) val = maxval;
    if (val < minval) val = minval;
  }
  assert(val >= minval && val <= maxval);
  return val;
}

// calculates cycle starttime based on counter value and timeIn.
// also sets stepnum. value must be in legal range
void
SoTimeCounter::calcStarttime(short value)
{
  this->stepnum = (value - this->min.getValue()) / this->step.getValue();
  if (this->dutylimits.getLength()) {
   this->starttime = this->timeIn.getValue().getValue()
     - this->dutylimits[this->stepnum];
  }
  else {
    this->starttime = this->timeIn.getValue().getValue() -
      double(this->stepnum) * this->cyclelen / double(this->numsteps);
  }
}
