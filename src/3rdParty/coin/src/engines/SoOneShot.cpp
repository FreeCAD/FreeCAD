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
  \class SoOneShot SoOneShot.h Inventor/engines/SoOneShot.h
  \brief The SoOneShot class is a timer that runs for a configurable time and then stops.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoOneShot.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/SoDB.h>

#include <Inventor/fields/SoSFFloat.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoSubEngineP.h"

/*!
  \var SoSFTime SoOneShot::timeIn

  Input time source. Connected by default to the realTime global
  field, but the application programmer is free to change this to any
  other time source.
*/
/*!
  \var SoSFTime SoOneShot::duration

  The time the engine should run for when it is triggered. Defaults to
  1 second.
*/
/*!
  \var SoSFTrigger SoOneShot::trigger

  A pulse on this input field starts the engine.
*/
/*!
  \var SoSFBitMask SoOneShot::flags

  Control flags. See SoOneShot::Flags.
*/
/*!
  \enum SoOneShot::Flags

  There are two flags available: \c RETRIGGERABLE will cause the
  engine to restart at 0 if the SoOneShot::trigger field is activated
  during a run (otherwise the trigger is ignored during runs).

  \c HOLD_FINAL will make the engine outputs keep their values after a
  run, instead of resetting them.
*/

/*!
  \var SoSFBool SoOneShot::disable

  Set to \c TRUE to disable the engine completely.
*/
/*!
  \var SoEngineOutput SoOneShot::timeOut

  (SoSFTime) Output time. Will run from 0 to the value of
  SoOneShot::duration.
*/
/*!
  \var SoEngineOutput SoOneShot::ramp
  (SoSFFloat) Will run from 0 to 1 during the active period.
*/
/*!
  \var SoEngineOutput SoOneShot::isActive
  (SoSFBool) \c TRUE while the engine is running, \c FALSE otherwise.
*/


SO_ENGINE_SOURCE(SoOneShot);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoOneShot::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoOneShot);
}

/*!
  Default constructor.
*/
SoOneShot::SoOneShot(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoOneShot);

  SO_ENGINE_ADD_INPUT(timeIn, (SbTime::zero()));
  SO_ENGINE_ADD_INPUT(duration, (SbTime(1.0)));
  SO_ENGINE_ADD_INPUT(trigger, ());
  SO_ENGINE_ADD_INPUT(flags, (0));
  SO_ENGINE_ADD_INPUT(disable, (FALSE));

  SO_ENGINE_ADD_OUTPUT(timeOut, SoSFTime);
  SO_ENGINE_ADD_OUTPUT(isActive, SoSFBool);
  SO_ENGINE_ADD_OUTPUT(ramp, SoSFFloat);

  SO_ENGINE_DEFINE_ENUM_VALUE(Flags, RETRIGGERABLE);
  SO_ENGINE_DEFINE_ENUM_VALUE(Flags, HOLD_FINAL);
  SO_ENGINE_SET_SF_ENUM_TYPE(flags, Flags);


  SoField * realtime = SoDB::getGlobalField("realTime");
  this->timeIn.connectFrom(realtime);

  this->running = FALSE;
  this->starttime = SbTime::zero();
  this->holdramp = 0.0f;
  this->holdduration = SbTime::zero();
}

/*!
  Destructor is protected to avoid explicit destruction.
*/
SoOneShot::~SoOneShot()
{
}

// Documented in superclass.
void
SoOneShot::evaluate(void)
{
  SbTime elapsed = this->timeIn.getValue() - this->starttime;
  SbTime durationval = this->duration.getValue();

  SbTime timeoutval;
  float rampval = -999.0f; // Explicit init to kill bogus egcs-2.91.66 warning.

  if (this->running) {
    if (elapsed < durationval) {
      timeoutval = elapsed;
      rampval = float(elapsed.getValue()) / float(durationval.getValue());
    }
    else {
      this->running = FALSE;

      if (this->flags.getValue() & SoOneShot::HOLD_FINAL) {
        this->holdduration = durationval;
        this->holdramp = 1.0f;
      }
    }
  }

  // Don't use "else" here, as the value of this->running might change
  // in the if-block above.
  if (!this->running) {
    if (this->flags.getValue() & SoOneShot::HOLD_FINAL) {
      timeoutval = this->holdduration;
      rampval = this->holdramp;
    }
    else {
      timeoutval = 0.0;
      rampval = 0.0f;
    }
  }

  // Values should be distributed on evaluate() even though outputs
  // are not initially enabled.
  //
  // enable-settings will be restored again on the next
  // inputChanged().
  this->timeOut.enable(TRUE);
  this->ramp.enable(TRUE);
  this->isActive.enable(TRUE);

  SO_ENGINE_OUTPUT(isActive, SoSFBool, setValue(this->running));
  SO_ENGINE_OUTPUT(timeOut, SoSFTime, setValue(timeoutval));
  SO_ENGINE_OUTPUT(ramp, SoSFFloat, setValue(rampval));
}

// Documented in superclass.
void
SoOneShot::inputChanged(SoField * which)
{
  SbBool do_evaluate = FALSE;

  if (which == &this->trigger) {
    if ((!this->running ||
         this->flags.getValue() & SoOneShot::RETRIGGERABLE) &&
        !this->disable.getValue()) {
      this->starttime = this->timeIn.getValue();
      this->running = TRUE;
    }
  }
  else if (which == &this->disable) {
    if (this->disable.getValue() && this->running) {
      this->holdduration = this->timeIn.getValue() - this->starttime;
      this->holdramp = static_cast<float>
        (this->holdduration.getValue() / this->duration.getValue().getValue());
      this->running = FALSE;
      // We need one more evaluation to send correct outputs.
      do_evaluate = TRUE;
    }
  }

  // Only enabled when running (as an optimization to avoid continuous
  // notification).
  this->timeOut.enable(this->running || do_evaluate);
  this->ramp.enable(this->running || do_evaluate);
  this->isActive.enable(this->running || do_evaluate);
}

// Documented in superclass. Overridden to not write connection to
// realTime global field.
void
SoOneShot::writeInstance(SoOutput * out)
{
  // Note: the code in this method matches that of SoElapsedTime and
  // SoTimeCounter, so if any bugs are found and corrected, remember
  // to pass on the updates.

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
