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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLTimeSensor SoVRMLTimeSensor.h Inventor/VRMLnodes/SoVRMLTimeSensor.h
  \brief The SoVRMLTimeSensor class is a multi-purpose time event generator.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  TimeSensor {
    exposedField SFTime   cycleInterval 1       # (0,inf)
    exposedField SFBool   enabled       TRUE
    exposedField SFBool   loop          FALSE
    exposedField SFTime   startTime     0       # (-inf,inf)
    exposedField SFTime   stopTime      0       # (-inf,inf)
    eventOut     SFTime   cycleTime
    eventOut     SFFloat  fraction_changed      # [0, 1]
    eventOut     SFBool   isActive
    eventOut     SFTime   time
  }
  \endverbatim

  TimeSensor nodes generate events as time passes. TimeSensor nodes
  can be used for many purposes including: 

  - driving continuous simulations and animations; 
  - controlling periodic activities (e.g., one per minute);
  - initiating single occurrence events such as an alarm clock.

  The TimeSensor node contains two discrete eventOuts: \e isActive and
  \e cycleTime. The \e isActive eventOut sends TRUE when the TimeSensor
  node begins running, and FALSE when it stops running. The \e cycleTime
  eventOut sends a time event at \e startTime and at the beginning of
  each new cycle (useful for synchronization with other time-based
  objects).  The remaining eventOuts generate continuous events. The
  \e fraction_changed eventOut, an SFFloat in the closed interval [0,1],
  sends the completed fraction of the current cycle. The \e time eventOut
  sends the absolute time for a given simulation tick.  

  If the enabled exposedField is TRUE, the TimeSensor node is enabled
  and may be running. If a set_enabled FALSE event is received while
  the TimeSensor node is running, the sensor performs the following
  actions: 
  
  - evaluates and sends all relevant outputs;
  - sends a FALSE value for isActive;
  - disables itself.

  Events on the exposedFields of the TimeSensor node (e.g., \e
  set_startTime) are processed and their corresponding eventOuts
  (e.g., startTime_changed) are sent regardless of the state of the
  enabled field. The remaining discussion assumes enabled is TRUE.

  The e\ loop, \e startTime, and \e stopTime exposedFields and the
  isActive eventOut and their effects on the TimeSensor node are discussed
  in detail in 4.6.9, Time-dependent nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.9>).
  The "cycle" of a TimeSensor node lasts for cycleInterval
  seconds. The value of cycleInterval shall be greater than zero.  

  A cycleTime eventOut can be used for synchronization purposes such
  as sound with animation. The value of a cycleTime eventOut will be
  equal to the time at the beginning of the current cycle. A cycleTime
  eventOut is generated at the beginning of every cycle, including the
  cycle starting at startTime. The first cycleTime eventOut for a
  TimeSensor node can be used as an alarm (single pulse at a specified
  time).

  When a TimeSensor node becomes active, it generates an isActive =
  TRUE event and begins generating time, fraction_changed, and
  cycleTime events which may be routed to other nodes to drive
  animation or simulated behaviours. The behaviour at read time is
  described below. The time event sends the absolute time for a given
  tick of the TimeSensor node (time fields and events represent the
  number of seconds since midnight GMT January 1, 1970).

  fraction_changed events output a floating point value in the closed
  interval [0, 1]. At startTime the value of fraction_changed is
  0. After startTime, the value of fraction_changed in any cycle will
  progress through the range (0.0, 1.0]. At startTime + N
  cycleInterval, for N = 1, 2, ..., that is, at the end of every
  cycle, the value of fraction_changed is 1.

  Let \e now represent the time at the current simulation tick. Then
  the time and fraction_changed eventOuts can then be computed as:

  \verbatim
  time = now 
  temp = (now - startTime) / cycleInterval 
  f = fractionalPart(temp) 
  if (f == 0.0 && now > startTime) fraction_changed = 1.0 
  else fraction_changed = f 
  \endverbatim
  
  where fractionalPart(x) is a function that returns the fractional
  part, (that is, the digits to the right of the decimal point), of a
  nonnegative floating point number.

  A TimeSensor node can be set up to be active at read time by
  specifying loop TRUE (not the default) and stopTime less than or
  equal to startTime (satisfied by the default values). The time
  events output absolute times for each tick of the TimeSensor node
  simulation.  The time events shall start at the first simulation
  tick greater than or equal to startTime. time events end at
  stopTime, or at startTime + N cycleInterval for some positive
  integer value of N, or loop forever depending on the values of the
  other fields. An active TimeSensor node shall stop at the first
  simulation tick when now >= stopTime > startTime.

  No guarantees are made with respect to how often a TimeSensor node
  generates time events, but a TimeSensor node shall generate events
  at least at every simulation tick. TimeSensor nodes are guaranteed
  to generate final time and fraction_changed events. If loop is FALSE
  at the end of the Nth cycleInterval and was TRUE at startTime + M
  cycleInterval for all 0 < M < N, the final time event will be
  generated with a value of (startTime + N cycleInterval) or
  stopTime (if stopTime > startTime), whichever value is less. If loop
  is TRUE at the completion of every cycle, the final event is
  generated as evaluated at stopTime (if stopTime > startTime) or
  never.

  An active TimeSensor node ignores set_cycleInterval and
  set_startTime events. An active TimeSensor node also ignores
  set_stopTime events for set_stopTime less than or equal to
  startTime.  For example, if a set_startTime event is received while
  a TimeSensor node is active, that set_startTime event is ignored
  (the startTime field is not changed, and a startTime_changed
  eventOut is not generated).  If an active TimeSensor node receives a
  set_stopTime event that is less than the current time, and greater
  than startTime, it behaves as if the stopTime requested is the
  current time and sends the final events based on the current time
  (note that stopTime is set as specified in the eventIn).
  
  A TimeSensor read from a VRML file shall generate isActive TRUE,
  time and fraction_changed events if the sensor is enabled and all
  conditions for a TimeSensor to be active are met.  

*/

/*!
  \var SoSFTime SoVRMLTimeSensor::cycleInterval
  The cycle interval. Default value is 1. Must be > 0.
*/

/*!
  \var SoSFBool SoVRMLTimeSensor::enabled
  Used to enable/disable timer. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLTimeSensor::loop
  TRUE if timer should loop. Default value is FALSE.
*/

/*!
  \var SoSFTime SoVRMLTimeSensor::startTime
  The timer start time. Default value is 0.0.
*/

/*!
  \var SoSFTime SoVRMLTimeSensor::stopTime
  The timer stop time. Default value is 0.0.
*/

/*!
  \var SoEngineOutput SoVRMLTimeSensor::cycleTime
  An eventOut that is sent when a new cycle is started.
*/

/*!
  \var SoEngineOutput SoVRMLTimeSensor::fraction_changed
  An eventOut that is sent for each tick, containing a number between 0 and 1.
*/

/*!
  \var SoEngineOutput SoVRMLTimeSensor::isActive
  An eventOut that is sent when the timer is enabled/disabled.
*/

/*!
  \var SoEngineOutput SoVRMLTimeSensor::time
  An eventOut that is sent for each tick, containing the current time.
*/

#include <Inventor/VRMLnodes/SoVRMLTimeSensor.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/SoDB.h>

#include "engines/SoSubNodeEngineP.h"

#ifndef DOXYGEN_SKIP_THIS

class SoVRMLTimeSensorP {
public:
  double starttime;
  double stoptime;
  double currtime;
  double cycletime;
  double cyclestart;
  float fraction;
  SbBool loop;
  SbBool running;
};

#endif // DOXYGEN_SKIP_THIS

SO_NODEENGINE_SOURCE(SoVRMLTimeSensor);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLTimeSensor::initClass(void) // static
{
  SO_NODEENGINE_INTERNAL_INIT_CLASS(SoVRMLTimeSensor);
}

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor.
*/
SoVRMLTimeSensor::SoVRMLTimeSensor(void)
{
  PRIVATE(this) = new SoVRMLTimeSensorP;

  SO_NODEENGINE_INTERNAL_CONSTRUCTOR(SoVRMLTimeSensor);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(cycleInterval, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(enabled, (TRUE));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(loop, (FALSE));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(startTime, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(stopTime, (0.0f));
  SO_VRMLNODE_ADD_EVENT_IN(timeIn); // private

  SO_NODEENGINE_ADD_OUTPUT(cycleTime, SoSFTime);
  SO_NODEENGINE_ADD_OUTPUT(fraction_changed, SoSFFloat);
  SO_NODEENGINE_ADD_OUTPUT(isActive, SoSFBool);
  SO_NODEENGINE_ADD_OUTPUT(time, SoSFTime);

  this->isActive.enable(FALSE);
  this->cycleTime.enable(FALSE);

  PRIVATE(this)->fraction = 0.0;
  PRIVATE(this)->cyclestart = 0.0;
  PRIVATE(this)->cycletime = 1.0;
  PRIVATE(this)->running = FALSE;
  PRIVATE(this)->loop = FALSE;
  PRIVATE(this)->starttime = 0.0;
  PRIVATE(this)->stoptime = 0.0;

  this->timeIn.enableNotify(FALSE);
  SoField * realtime = SoDB::getGlobalField("realTime");
  this->timeIn.connectFrom(realtime);

  // we always connect and just disable notification when timer
  // is not active, since it is currently not possible to disconnect
  // from a field in the inputChanged() method. inputChanged() is
  // triggered by notify(), and if a field is disconnected while the
  // master field is notifying, bad things will happen in
  // SoAuditorList.

  // FIXME: Maybe we should consider making a version of SoAuditorList
  // that handles disconnects in the notification loop? I think
  // it might be difficult though.   pederb, 2001-11-06
}

/*!
  Destructor.
*/
SoVRMLTimeSensor::~SoVRMLTimeSensor()
{
    delete PRIVATE(this);
    PRIVATE(this) = 0;
}

// Doc in parent
void
SoVRMLTimeSensor::notify(SoNotList * list)
{
  inherited::notify(list);
}

// Documented in superclass. Overridden to not write connection to
// realTime global field.
void
SoVRMLTimeSensor::write(SoWriteAction * action)
{
  // Note: the code in this method matches that of SoElapsedTime and
  // SoOneShot and SoTimeSensor, so if any bugs are found and
  // corrected, remember to pass on the updates.

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

  inherited::write(action);

  // Re-connect to realTime field.
  if (connectfromrealTime) {
    // Don't send notification when reconnecting to preserve the state
    // of the scene graph between write passes.
    this->timeIn.connectFrom(connectfield, TRUE);
    this->timeIn.setDefault(defaultflag);
  }
}


// Doc in parent
void
SoVRMLTimeSensor::evaluate(void)
{
  SO_ENGINE_OUTPUT(time, SoSFTime, setValue(PRIVATE(this)->currtime));
  SO_ENGINE_OUTPUT(isActive, SoSFBool, setValue(PRIVATE(this)->running));
  SO_ENGINE_OUTPUT(cycleTime, SoSFTime, setValue(PRIVATE(this)->cyclestart));
  SO_ENGINE_OUTPUT(fraction_changed, SoSFFloat, setValue(PRIVATE(this)->fraction));
}

// Doc in parent
void
SoVRMLTimeSensor::inputChanged(SoField * which)
{
  // Default is to not do any notification when we return from this
  // function to SoEngine::notify(). This is an optimization for this
  // engine to avoid transmission of notification to all slave fields
  // each time the timeIn field is updated.
  this->fraction_changed.enable(FALSE);
  this->isActive.enable(FALSE);
  this->cycleTime.enable(FALSE);

  if (which == &this->enabled) {
    SbBool on = this->enabled.getValue();

    if (!on) this->timeIn.enableNotify(FALSE);

    if (PRIVATE(this)->running && !on) {
      PRIVATE(this)->running = FALSE;
      this->fraction_changed.enable(TRUE);
      this->isActive.enable(TRUE);
    }
    else if (!PRIVATE(this)->running && on) {
      which = &this->startTime; // warning, hack
    }
  }

  if (which == &this->loop) {
    PRIVATE(this)->loop = this->loop.getValue();
    if (PRIVATE(this)->loop == TRUE && !this->timeIn.isNotifyEnabled())
      which = &this->startTime; // warning hack
  }

  if (which == &this->startTime) {
    double currtime = this->timeIn.getValue().getValue();
    PRIVATE(this)->starttime = currtime;
    if (!PRIVATE(this)->running) {
      PRIVATE(this)->starttime = this->startTime.getValue().getValue();
      if (currtime >= PRIVATE(this)->starttime) {
        SbBool old = this->timeIn.enableNotify(TRUE);
        assert(old == FALSE);
        which = &this->timeIn; // warning, hack
      } else {
        // enable to wait for timeIn to be >= starttime
        this->timeIn.enableNotify(TRUE);
      }
    }
  }

  if (which == &this->timeIn) {
    double currtime = this->timeIn.getValue().getValue();
    if (!PRIVATE(this)->running) {
      if (currtime >= PRIVATE(this)->starttime) {
        this->isActive.enable(TRUE);
        this->cycleTime.enable(TRUE);
        PRIVATE(this)->cyclestart = PRIVATE(this)->starttime;
        PRIVATE(this)->running = TRUE;
      }
      else return; // wait for startTime
    }
    PRIVATE(this)->currtime = currtime;
    this->time.enable(TRUE);
    this->fraction_changed.enable(TRUE);

    SbBool stopit = FALSE;
    if (currtime >= PRIVATE(this)->stoptime && PRIVATE(this)->stoptime > PRIVATE(this)->starttime) stopit = TRUE;

    double difftime = currtime - PRIVATE(this)->cyclestart;

    if (difftime > PRIVATE(this)->cycletime) {
      this->cycleTime.enable(TRUE);
      double num = difftime / PRIVATE(this)->cycletime;
      PRIVATE(this)->cyclestart += PRIVATE(this)->cycletime * floor(num);
      difftime = currtime - PRIVATE(this)->cyclestart;

      if (PRIVATE(this)->loop == FALSE) stopit = TRUE;
    }
    PRIVATE(this)->fraction = (float) (difftime / PRIVATE(this)->cycletime);

    if (stopit) {
      PRIVATE(this)->running = FALSE;
      this->isActive.enable(TRUE);
      this->fraction_changed.enable(FALSE);
      this->timeIn.enableNotify(FALSE);
    }
  }
  else if (which == &this->stopTime) {
    PRIVATE(this)->stoptime = this->stopTime.getValue().getValue();
  }
  else if (which == &this->cycleInterval) {
    PRIVATE(this)->cycletime = this->cycleInterval.getValue().getValue();
  }
}

// Doc in parent
void
SoVRMLTimeSensor::handleEvent(SoHandleEventAction * action)
{
  inherited::handleEvent(action);
}

#undef PRIVATE

#endif // HAVE_VRML97
