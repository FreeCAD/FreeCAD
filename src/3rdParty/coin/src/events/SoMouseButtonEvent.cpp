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
  \class SoMouseButtonEvent SoMouseButtonEvent.h Inventor/events/SoMouseButtonEvent.h
  \brief The SoMouseButtonEvent class contains information about
  mouse button interaction.

  \ingroup coin_events

  When the user presses any buttons on the mouse, these will be
  translated from a system specific event into a Coin event and sent
  to the scene graph by using instances of this class.

  \sa SoEvent, SoButtonEvent, SoSpaceballButtonEvent, SoKeyboardEvent
  \sa SoEventCallback, SoHandleEventAction */

#include <Inventor/events/SoMouseButtonEvent.h>

#include "SbBasicP.h"

#include <Inventor/SbName.h>
#include <cassert>

/*!
  \enum SoMouseButtonEvent::Button
  This enum contains all mouse buttons detected by Coin.
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::ANY
  Unknown button.
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::BUTTON1
  First mouse button (usually the leftmost button).
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::BUTTON2
  Second mouse button (usually the middle button).
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::BUTTON3
  Third mouse button (usually the rightmost button).
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::BUTTON4
  Fourth mouse button (typically from a wheel mouse). This is
  \e not part of the original Open Inventor API.
*/
/*!
  \var SoMouseButtonEvent::Button SoMouseButtonEvent::BUTTON5
  Fifth mouse button (typically from a wheel mouse). This is
  \e not part of the original Open Inventor API.
*/

/*!
  \def SO_MOUSE_PRESS_EVENT(EVENT, BUTTON)

  This macro evaluates to \c TRUE iff the \a EVENT represents a press
  on the given \a BUTTON.
*/
/*!
  \def SO_MOUSE_RELEASE_EVENT(EVENT, BUTTON)

  This macro evaluates to \c TRUE iff the \a EVENT represents a
  release of the given \a BUTTON.
*/



SO_EVENT_SOURCE(SoMouseButtonEvent);

/*!
  Initialize the type information data.
 */
void
SoMouseButtonEvent::initClass(void)
{
  SO_EVENT_INIT_CLASS(SoMouseButtonEvent, SoButtonEvent);
}

/*!
  Constructor.
*/
SoMouseButtonEvent::SoMouseButtonEvent(void)
{
  this->button = SoMouseButtonEvent::ANY;
}

/*!
  Destructor.
*/
SoMouseButtonEvent::~SoMouseButtonEvent()
{
}

/*!
  Set the value of the button which the user interacted with.

  This method is used from the window specific device classes when
  translating events to the generic Coin library.

  \sa getButton()
 */
void
SoMouseButtonEvent::setButton(SoMouseButtonEvent::Button buttonarg)
{
  this->button = buttonarg;
}

/*!
  Returns the value of the button which was pressed or released.

  \sa getState()
  \sa wasShiftDown(), wasCtrlDown(), wasAltDown(), getPosition(), getTime()
 */
SoMouseButtonEvent::Button
SoMouseButtonEvent::getButton(void) const
{
  return this->button;
}

/*!
  Convenience method for quickly checking if the given event is a
  press on the given button, \a whichButton.

  \sa isButtonReleaseEvent(), isOfType(), getButton(), getState()
 */
SbBool
SoMouseButtonEvent::isButtonPressEvent(const SoEvent * e,
                                       SoMouseButtonEvent::Button whichButton)
{
  if (e->isOfType(SoMouseButtonEvent::getClassTypeId())) {
    const SoMouseButtonEvent * me = coin_assert_cast<const SoMouseButtonEvent *>(e);
    if ((me->getState() == SoButtonEvent::DOWN) &&
        ((whichButton == SoMouseButtonEvent::ANY) ||
         whichButton == me->getButton())) return TRUE;
  }
  return FALSE;
}

/*!
  Convenience method for quickly checking if the given event is a
  release of the given button, \a whichButton.

  \sa isButtonPressEvent(), isOfType(), getButton(), getState()
 */
SbBool
SoMouseButtonEvent::isButtonReleaseEvent(const SoEvent * e,
                                         SoMouseButtonEvent::Button
                                         whichButton)
{
  if (e->isOfType(SoMouseButtonEvent::getClassTypeId())) {
    const SoMouseButtonEvent * me = coin_assert_cast<const SoMouseButtonEvent *>(e);
    if ((me->getState() == SoButtonEvent::UP) &&
        ((whichButton == SoMouseButtonEvent::ANY) ||
         whichButton == me->getButton())) return TRUE;
  }
  return FALSE;
}

/*!
  Converts from an enum value of type SoMouseButtonEvent::Button to a
  string containing the enum symbol.

  \COIN_FUNCTION_EXTENSION
  \since Coin 3.0
*/
// Should we add stringToEnum as well perhaps?
SbBool
SoMouseButtonEvent::enumToString(Button enumval, SbString & stringrep)
{
  switch (enumval) {
  case SoMouseButtonEvent::ANY:
    stringrep = "ANY";
    break;
  case SoMouseButtonEvent::BUTTON1:
    stringrep = "BUTTON1";
    break;
  case SoMouseButtonEvent::BUTTON2:
    stringrep = "BUTTON2";
    break;
  case SoMouseButtonEvent::BUTTON3:
    stringrep = "BUTTON3";
    break;
  case SoMouseButtonEvent::BUTTON4:
    stringrep = "BUTTON4";
    break;
  case SoMouseButtonEvent::BUTTON5:
    stringrep = "BUTTON5";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}
