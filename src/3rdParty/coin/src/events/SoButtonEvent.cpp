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
  \class SoButtonEvent SoButtonEvent.h Inventor/events/SoButtonEvent.h
  \brief The SoButtonEvent class is the base class for all button events.

  \ingroup coin_events

  The event classes which results from the user pushing buttons on
  some device (keyboard, mouse or spaceball) all inherit this
  class. The SoButtonEvent class contains methods for setting and
  getting the state of the button(s).

  \sa SoEvent, SoKeyboardEvent, SoMouseButtonEvent, SoSpaceballButtonEvent
  \sa SoEventCallback, SoHandleEventAction */

#include <Inventor/events/SoButtonEvent.h>
#include <Inventor/SbName.h>
#include <cassert>

/*!
  \enum SoButtonEvent::State
  This gives the actual state of the button.
 */
/*!
  \var SoButtonEvent::State SoButtonEvent::UP
  Button has been released.
*/
/*!
  \var SoButtonEvent::State SoButtonEvent::DOWN
  Button has been pressed down.
*/
/*!
  \var SoButtonEvent::State SoButtonEvent::UNKNOWN
  The state of the button is unknown.
*/


SO_EVENT_SOURCE(SoButtonEvent);

/*!
  Initialize the type information data.
 */
void
SoButtonEvent::initClass(void)
{
  SO_EVENT_INIT_CLASS(SoButtonEvent, SoEvent);
}

/*!
  Constructor.
*/
SoButtonEvent::SoButtonEvent(void)
{
  this->buttonstate = SoButtonEvent::UNKNOWN;
}

/*!
  Destructor.
*/
SoButtonEvent::~SoButtonEvent()
{
}

/*!
  Set the state of the button which the user interacted with.

  This method is used from the window specific device classes when
  translating events to the generic Coin library.

  \sa getState()
 */
void
SoButtonEvent::setState(SoButtonEvent::State state)
{
  this->buttonstate = state;
}

/*!
  Returns the state of the button which is the cause of the event, i.e.
  whether it was pushed down or released.

  \sa wasShiftDown(), wasCtrlDown(), wasAltDown(), getPosition(), getTime()
 */
SoButtonEvent::State
SoButtonEvent::getState(void) const
{
  return this->buttonstate;
}

/*!
  Converts from an enum value of type SoButtonEvent::State to a
  string containing the enum symbol.

  \COIN_FUNCTION_EXTENSION
  \since Coin 3.0
*/
// Should we add stringToEnum as well perhaps?
SbBool
SoButtonEvent::enumToString(State enumval, SbString & stringrep)
{
  switch (enumval) {
  case SoButtonEvent::UP:
    stringrep = "UP";
    break;
  case SoButtonEvent::DOWN:
    stringrep = "DOWN";
    break;
  case SoButtonEvent::UNKNOWN:
    stringrep = "UNKNOWN";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}
