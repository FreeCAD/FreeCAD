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
  \class SoSpaceballButtonEvent SoSpaceballButtonEvent.h Inventor/events/SoSpaceballButtonEvent.h
  \brief The SoSpaceballButtonEvent class contains information about
  spaceball button interaction.

  \ingroup coin_events

  When the user presses any buttons on a spaceball device, these will
  be translated from a system specific event into a Coin event and
  sent to the scene graph by using instances of this class.

  \sa SoEvent, SoButtonEvent, SoMouseButtonEvent, SoKeyboardEvent
  \sa SoEventCallback, SoHandleEventAction */

#include <Inventor/events/SoSpaceballButtonEvent.h>

#include "SbBasicP.h"

#include <Inventor/SbName.h>
#include <cassert>

/*!
  \enum SoSpaceballButtonEvent::Button
  This enum contains all spaceball buttons detected by Coin.
 */
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::ANY
  Any of the buttons.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON1
  Spaceball button 1.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON2
  Spaceball button 2.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON3
  Spaceball button 3.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON4
  Spaceball button 4.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON5
  Spaceball button 5.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON6
  Spaceball button 6.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON7
  Spaceball button 7.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::BUTTON8
  Spaceball button 8.
*/
/*!
  \var SoSpaceballButtonEvent::Button SoSpaceballButtonEvent::PICK
  Spaceball pick button.
*/

/*!
  \def SO_SPACEBALL_PRESS_EVENT(EVENT, BUTTON)
  This macro evaluates to \c TRUE iff the \c EVENT represents a press on the
  given \c BUTTON.
*/
/*!
  \def SO_SPACEBALL_RELEASE_EVENT(EVENT, BUTTON)
  This macro evaluates to \c TRUE iff the \c EVENT represents a release of the
  given \c BUTTON.
*/

SO_EVENT_SOURCE(SoSpaceballButtonEvent);

/*!
  Initialize the type information data.
 */
void
SoSpaceballButtonEvent::initClass(void)
{
  SO_EVENT_INIT_CLASS(SoSpaceballButtonEvent, SoButtonEvent);
}

/*!
  Constructor.
 */
SoSpaceballButtonEvent::SoSpaceballButtonEvent(void)
{
  this->button = SoSpaceballButtonEvent::ANY;
}

/*!
  Destructor.
 */
SoSpaceballButtonEvent::~SoSpaceballButtonEvent()
{
}

/*!
  Set the value of the button which the user interacted with.

  This method is used from the window specific device classes when
  translating events to the generic Coin library.

  \sa getButton()
 */
void
SoSpaceballButtonEvent::setButton(SoSpaceballButtonEvent::Button buttonarg)
{
  this->button = buttonarg;
}

/*!
  Returns the value of the button which was pressed or released.

  \sa getState()
  \sa wasShiftDown(), wasCtrlDown(), wasAltDown(), getPosition(), getTime()
 */
SoSpaceballButtonEvent::Button
SoSpaceballButtonEvent::getButton(void) const
{
  return this->button;
}

/*!
  Convenience method for quickly checking if the given event is a
  press on the given button, \c whichButton.

  \sa isButtonReleaseEvent(), isOfType(), getButton(), getState()
 */
SbBool
SoSpaceballButtonEvent::isButtonPressEvent(const SoEvent * e,
                                           SoSpaceballButtonEvent::Button
                                           whichButton)
{
  return (e->isOfType(SoSpaceballButtonEvent::getClassTypeId()) &&
          (
          whichButton == SoSpaceballButtonEvent::ANY ||
           coin_assert_cast<const SoSpaceballButtonEvent *>(e)->getButton()
          == whichButton
          )
         &&
          coin_assert_cast<const SoButtonEvent *>(e)->getState() == SoButtonEvent::DOWN
         );
}

/*!
  Convenience method for quickly checking if the given event is a
  release of the given button, \c whichButton.

  \sa isButtonPressEvent(), isOfType(), getButton(), getState()
 */
SbBool
SoSpaceballButtonEvent::isButtonReleaseEvent(const SoEvent * e,
                                             SoSpaceballButtonEvent::Button
                                             whichButton)
{
  return (e->isOfType(SoSpaceballButtonEvent::getClassTypeId()) &&
          (
          whichButton == SoSpaceballButtonEvent::ANY ||
           coin_assert_cast<const SoSpaceballButtonEvent *>(e)->getButton()
          == whichButton
          ) &&
          coin_assert_cast<const SoButtonEvent *>(e)->getState() == SoButtonEvent::UP
         );
}


/*!
  Converts from an enum value of type SoMouseButtonEvent::Button to a
  string containing the enum symbol.

  \COIN_FUNCTION_EXTENSION
  \since Coin 3.0
*/
// Should we add stringToEnum as well perhaps?
SbBool
SoSpaceballButtonEvent::enumToString(Button enumval, SbString & stringrep)
{
  switch (enumval) {
  case SoSpaceballButtonEvent::ANY:
    stringrep = "ANY";
    break;
  case SoSpaceballButtonEvent::BUTTON1:
    stringrep = "BUTTON1";
    break;
  case SoSpaceballButtonEvent::BUTTON2:
    stringrep = "BUTTON2";
    break;
  case SoSpaceballButtonEvent::BUTTON3:
    stringrep = "BUTTON3";
    break;
  case SoSpaceballButtonEvent::BUTTON4:
    stringrep = "BUTTON4";
    break;
  case SoSpaceballButtonEvent::BUTTON5:
    stringrep = "BUTTON5";
    break;
  case SoSpaceballButtonEvent::BUTTON6:
    stringrep = "BUTTON6";
    break;
  case SoSpaceballButtonEvent::BUTTON7:
    stringrep = "BUTTON7";
    break;
  case SoSpaceballButtonEvent::BUTTON8:
    stringrep = "BUTTON8";
    break;
  case SoSpaceballButtonEvent::PICK:
    stringrep = "PICK";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}
