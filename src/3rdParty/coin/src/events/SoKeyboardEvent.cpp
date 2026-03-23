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
  \class SoKeyboardEvent SoKeyboardEvent.h Inventor/events/SoKeyboardEvent.h
  \brief The SoKeyboardEvent class contains information about
  keyboard interaction.

  \ingroup coin_events

  When the user presses any keys on the keyboard, these will be translated
  from a system specific event into a Coin event and sent to the
  scene graph by using instances of this class.

  \sa SoEvent, SoButtonEvent, SoMouseButtonEvent, SoSpaceballButtonEvent
  \sa SoEventCallback, SoHandleEventAction
*/

// *************************************************************************

#include <Inventor/events/SoKeyboardEvent.h>

#include "SbBasicP.h"
#include "tidbitsp.h"
#include "misc/SbHash.h"

#include <cassert>

#include <Inventor/SbName.h>

// *************************************************************************

// Avoid problem with Microsoft Win32 API headers (yes, they actually
// #define DELETE somewhere in their header files).
#undef DELETE


/*
  FIXME: The keycode handling in SoKeyboardEvent is really terrible --
  could we improve it while still keeping Open Inventor API
  compatibility?  -mortene.
*/



/*!
  \enum SoKeyboardEvent::Key
  This enum contains all keys detected by Coin.
 */

/*!
  \def SO_KEY_PRESS_EVENT(EVENT, KEY)
  This macro evaluates to \c TRUE iff the \c EVENT represents a press on the
  given \c KEY.
*/
/*!
  \def SO_KEY_RELEASE_EVENT(EVENT, KEY)
  This macro evaluates to \c TRUE iff the \c EVENT represents a release of the
  given \c KEY.
*/

static SbHash<int, char> * converttoprintable = NULL;
static SbHash<int, char> * converttoprintable_shift = NULL;

extern "C" {

static void
sokeyboardevent_cleanup(void)
{
  delete converttoprintable;
  converttoprintable = NULL;
  delete converttoprintable_shift;
  converttoprintable_shift = NULL;
}

} // extern "C"

static void
build_convert_dicts(void)
{
  int i;
  converttoprintable = new SbHash<int, char>();
  converttoprintable_shift = new SbHash<int, char>();
  coin_atexit(sokeyboardevent_cleanup, CC_ATEXIT_NORMAL);

#define ADD_KEY(x,y) d->put(SoKeyboardEvent::x, y)

  // shift not down
  SbHash<int, char> * d = converttoprintable;
  ADD_KEY(NUMBER_0, '0');
  ADD_KEY(NUMBER_1, '1');
  ADD_KEY(NUMBER_2, '2');
  ADD_KEY(NUMBER_3, '3');
  ADD_KEY(NUMBER_4, '4');
  ADD_KEY(NUMBER_5, '5');
  ADD_KEY(NUMBER_6, '6');
  ADD_KEY(NUMBER_7, '7');
  ADD_KEY(NUMBER_8, '8');
  ADD_KEY(NUMBER_9, '9');

  ADD_KEY(PAD_0, '0');
  ADD_KEY(PAD_1, '1');
  ADD_KEY(PAD_2, '2');
  ADD_KEY(PAD_3, '3');
  ADD_KEY(PAD_4, '4');
  ADD_KEY(PAD_5, '5');
  ADD_KEY(PAD_6, '6');
  ADD_KEY(PAD_7, '7');
  ADD_KEY(PAD_8, '8');
  ADD_KEY(PAD_9, '9');

  ADD_KEY(PAD_ADD, '+');
  ADD_KEY(PAD_SUBTRACT, '-');
  ADD_KEY(PAD_MULTIPLY, '*');
  ADD_KEY(PAD_DIVIDE, '/');
  ADD_KEY(PAD_SPACE, ' ');

  ADD_KEY(SPACE, ' ');
  ADD_KEY(APOSTROPHE, '\'');
  ADD_KEY(COMMA, ',');
  ADD_KEY(MINUS, '-');
  ADD_KEY(PERIOD, '.');
  ADD_KEY(SLASH, '/');
  ADD_KEY(SEMICOLON, ';');
  ADD_KEY(EQUAL, '=');
  ADD_KEY(BRACKETLEFT, '[');
  ADD_KEY(BACKSLASH, '\\');
  ADD_KEY(BRACKETRIGHT,']');
  ADD_KEY(GRAVE,'`');

  for (
       i = static_cast<int>(SoKeyboardEvent::A);
       i <= static_cast<int>(SoKeyboardEvent::Z);
       i++
       )
    {
      d->put(i, ('a' + i - static_cast<int>(SoKeyboardEvent::A)));
    }

  // shift down
  d = converttoprintable_shift;
  ADD_KEY(NUMBER_0, ')');
  ADD_KEY(NUMBER_1, '!');
  ADD_KEY(NUMBER_2, '@');
  ADD_KEY(NUMBER_3, '#');
  ADD_KEY(NUMBER_4, '$');
  ADD_KEY(NUMBER_5, '%');
  ADD_KEY(NUMBER_6, '^');
  ADD_KEY(NUMBER_7, '&');
  ADD_KEY(NUMBER_8, '*');
  ADD_KEY(NUMBER_9, '(');

  ADD_KEY(PAD_0, '0');
  ADD_KEY(PAD_1, '1');
  ADD_KEY(PAD_2, '2');
  ADD_KEY(PAD_3, '3');
  ADD_KEY(PAD_4, '4');
  ADD_KEY(PAD_5, '5');
  ADD_KEY(PAD_6, '6');
  ADD_KEY(PAD_7, '7');
  ADD_KEY(PAD_8, '8');
  ADD_KEY(PAD_9, '9');

  ADD_KEY(PAD_ADD, '+');
  ADD_KEY(PAD_SUBTRACT, '-');
  ADD_KEY(PAD_MULTIPLY, '*');
  ADD_KEY(PAD_DIVIDE, '/');
  ADD_KEY(PAD_SPACE, ' ');

  ADD_KEY(SPACE, ' ');
  ADD_KEY(APOSTROPHE, '\"');
  ADD_KEY(COMMA, '<');
  ADD_KEY(MINUS, '_');
  ADD_KEY(PERIOD, '>');
  ADD_KEY(SLASH, '?');
  ADD_KEY(SEMICOLON, ':');
  ADD_KEY(EQUAL, '+');
  ADD_KEY(BRACKETLEFT, '{');
  ADD_KEY(BACKSLASH, '|');
  ADD_KEY(BRACKETRIGHT,'}');
  ADD_KEY(GRAVE,'~');

  for (
       i = static_cast<int>(SoKeyboardEvent::A);
       i <= static_cast<int>(SoKeyboardEvent::Z);
       i++
       )
    {
    d->put(i, ('A' + i - static_cast<int>(SoKeyboardEvent::A)));
  }
#undef ADD_KEY
}

// *************************************************************************

SO_EVENT_SOURCE(SoKeyboardEvent);

// *************************************************************************

/*!
  Initialize the type information data.
 */
void
SoKeyboardEvent::initClass(void)
{
  SO_EVENT_INIT_CLASS(SoKeyboardEvent, SoButtonEvent);
}

/*!
  Constructor.
 */
SoKeyboardEvent::SoKeyboardEvent(void)
{
  this->key = SoKeyboardEvent::ANY;
  this->isprintableset = 0;
}

/*!
  Destructor.
*/
SoKeyboardEvent::~SoKeyboardEvent()
{
}

/*!
  Set the value of the key which the user interacted with.

  This method is used from the window specific device classes when
  translating events to the generic Coin library.

  \sa getKey()
 */
void
SoKeyboardEvent::setKey(SoKeyboardEvent::Key keyarg)
{
  this->key = keyarg;
  this->isprintableset = 0;
}

/*!
  Returns the value of the key which was pressed or released.

  Coin adds a new key value called UNDEFINED. This is needed to
  support GUI toolkits where it is not possible to find exactly which
  key is pressed, and/or to support non-US keyboards. The Open
  Inventor design for this class is flawed, since it assumes everybody
  uses a US keyboard. We recommend using getPrintableCharacter() to
  find which key is pressed/released, at least for printable
  non-alphanumerical characters.

  \sa getPrintableCharacter(), getState()
  \sa wasShiftDown(), wasCtrlDown(), wasAltDown(), getPosition(), getTime()
*/
SoKeyboardEvent::Key
SoKeyboardEvent::getKey(void) const
{
  return this->key;
}

/*!
  Convenience method for quickly checking if the given event is a
  key press on the given key, \c whichKey.

  \sa isKeyReleaseEvent(), isOfType(), getKey(), getState()
 */
SbBool
SoKeyboardEvent::isKeyPressEvent(const SoEvent * e,
                                 SoKeyboardEvent::Key whichKey)
{
  return (e->isOfType(SoKeyboardEvent::getClassTypeId()) &&
          (whichKey == SoKeyboardEvent::ANY ||
           coin_assert_cast<const SoKeyboardEvent *>(e)->getKey() == whichKey) &&
          coin_assert_cast<const SoButtonEvent *>(e)->getState() == SoButtonEvent::DOWN);
}

/*!
  Convenience method for quickly checking if the given event is a
  key release of the given key, \c whichKey.

  \sa isKeyPressEvent(), isOfType(), getKey(), getState()
 */
SbBool
SoKeyboardEvent::isKeyReleaseEvent(const SoEvent * e,
                                   SoKeyboardEvent::Key whichKey)
{
  return (e->isOfType(SoKeyboardEvent::getClassTypeId()) &&
          (whichKey == SoKeyboardEvent::ANY ||
           coin_assert_cast<const SoKeyboardEvent *>(e)->getKey() == whichKey) &&
          coin_assert_cast<const SoButtonEvent *>(e)->getState() == SoButtonEvent::UP);
}

/*!
  Sets the printable character for this keyboard event. If this method
  is not called when creating an event, getPrintableCharacter() will
  convert the SoKeyboardEvent::Key value into a printable character.
  This conversion does not work on non-US keyboards, so we recommend
  that you set the printable character using this method instead.

  This printable character is cleared each time setKey() is called.

  This method is an extension versus the Open Inventor API.

  \sa getPrintableCharacter()
*/
void
SoKeyboardEvent::setPrintableCharacter(const char c)
{
  this->printable = c;
  this->isprintableset = 1;
}

/*!
  Return ASCII value which would be generated by the key and
  modifier combination.

  NB! If setPrintableCharacter() hasn't been called, this function
  does not always work as expected, particularly not on non-US
  keyboards. The Coin GUI toolkits (SoGtk/SoQt/SoWin/SoXt/Sc21) will
  set the printable character correctly.

  \sa getKey(), wasShiftDown(), wasCtrlDown(), wasAltDown(), setPrintableCharacter()
*/
char
SoKeyboardEvent::getPrintableCharacter(void) const
{
  if (this->isprintableset) return this->printable;

  if (converttoprintable == NULL) {
    build_convert_dicts();
  }

  SbHash<int, char> * dict =
    this->wasShiftDown() ? converttoprintable_shift : converttoprintable;
  char value;
  if (dict->get(this->getKey(), value)) { return value; }
  return '.';
}

/*!
  Converts from an enum value of type SoKeyboardEvent::State to a
  string containing the enum symbol.

  \COIN_FUNCTION_EXTENSION
  \since Coin 3.0
*/
// Should we add stringToEnum as well perhaps?
SbBool
SoKeyboardEvent::enumToString(Key enumval, SbString & stringrep)
{
  if (enumval >= SoKeyboardEvent::A && enumval <= SoKeyboardEvent::Z) {
    stringrep.sprintf("%c", 'A' + (enumval - SoKeyboardEvent::A));
    return TRUE;
  }
  if (enumval >= SoKeyboardEvent::F1 && enumval <= SoKeyboardEvent::F12) {
    stringrep.sprintf("F%d", 1 + (enumval - SoKeyboardEvent::F1));
    return TRUE;
  }

  switch (enumval) {
  case SoKeyboardEvent::ANY:
    stringrep = "ANY";
    break;
  case SoKeyboardEvent::UNDEFINED:
    stringrep = "UNDEFINED";
    break;
  case SoKeyboardEvent::LEFT_SHIFT:
    stringrep = "LEFT_SHIFT";
    break;
  case SoKeyboardEvent::RIGHT_SHIFT:
    stringrep = "RIGHT_SHIFT";
    break;
  case SoKeyboardEvent::LEFT_CONTROL:
    stringrep = "LEFT_CONTROL";
    break;
  case SoKeyboardEvent::RIGHT_CONTROL:
    stringrep = "RIGHT_CONTROL";
    break;
  case SoKeyboardEvent::LEFT_ALT:
    stringrep = "LEFT_ALT";
    break;
  case SoKeyboardEvent::NUMBER_0:
    stringrep = "NUMBER_0";
    break;
  case SoKeyboardEvent::NUMBER_1:
    stringrep = "NUMBER_1";
    break;
  case SoKeyboardEvent::NUMBER_2:
    stringrep = "NUMBER_2";
    break;
  case SoKeyboardEvent::NUMBER_3:
    stringrep = "NUMBER_3";
    break;
  case SoKeyboardEvent::NUMBER_4:
    stringrep = "NUMBER_4";
    break;
  case SoKeyboardEvent::NUMBER_5:
    stringrep = "NUMBER_5";
    break;
  case SoKeyboardEvent::NUMBER_6:
    stringrep = "NUMBER_6";
    break;
  case SoKeyboardEvent::NUMBER_7:
    stringrep = "NUMBER_7";
    break;
  case SoKeyboardEvent::NUMBER_8:
    stringrep = "NUMBER_8";
    break;
  case SoKeyboardEvent::NUMBER_9:
    stringrep = "NUMBER_9";
    break;
  case SoKeyboardEvent::HOME:
    stringrep = "HOME";
    break;
  case SoKeyboardEvent::LEFT_ARROW:
    stringrep = "LEFT_ARROW";
    break;
  case SoKeyboardEvent::UP_ARROW:
    stringrep = "UP_ARROW";
    break;
  case SoKeyboardEvent::RIGHT_ARROW:
    stringrep = "RIGHT_ARROW";
    break;
  case SoKeyboardEvent::DOWN_ARROW:
    stringrep = "DOWN_ARROW";
    break;
  case SoKeyboardEvent::PAGE_UP: // aka PRIOR
    stringrep = "PAGE_UP";
    break;
  case SoKeyboardEvent::PAGE_DOWN: // aka NEXT
    stringrep = "PAGE_DOWN";
    break;
  case SoKeyboardEvent::END:
    stringrep = "END";
    break;
  case SoKeyboardEvent::PAD_ENTER: // aka PAD_SPACE
    stringrep = "PAD_ENTER";
    break;
  case SoKeyboardEvent::PAD_F1:
    stringrep = "PAD_F1";
    break;
  case SoKeyboardEvent::PAD_F2:
    stringrep = "PAD_F2";
    break;
  case SoKeyboardEvent::PAD_F3:
    stringrep = "PAD_F3";
    break;
  case SoKeyboardEvent::PAD_F4:
    stringrep = "PAD_F4";
    break;
  case SoKeyboardEvent::PAD_0: // aka PAD_INSERT
    stringrep = "PAD_0";
    break;
  case SoKeyboardEvent::PAD_1:
    stringrep = "PAD_1";
    break;
  case SoKeyboardEvent::PAD_2:
    stringrep = "PAD_2";
    break;
  case SoKeyboardEvent::PAD_3:
    stringrep = "PAD_3";
    break;
  case SoKeyboardEvent::PAD_4:
    stringrep = "PAD_4";
    break;
  case SoKeyboardEvent::PAD_5:
    stringrep = "PAD_5";
    break;
  case SoKeyboardEvent::PAD_6:
    stringrep = "PAD_6";
    break;
  case SoKeyboardEvent::PAD_7:
    stringrep = "PAD_7";
    break;
  case SoKeyboardEvent::PAD_8:
    stringrep = "PAD_8";
    break;
  case SoKeyboardEvent::PAD_9:
    stringrep = "PAD_9";
    break;
  case SoKeyboardEvent::PAD_ADD:
    stringrep = "PAD_ADD";
    break;
  case SoKeyboardEvent::PAD_SUBTRACT:
    stringrep = "PAD_SUBTRACT";
    break;
  case SoKeyboardEvent::PAD_MULTIPLY:
    stringrep = "PAD_MULTIPLY";
    break;
  case SoKeyboardEvent::PAD_DIVIDE:
    stringrep = "PAD_DIVIDE";
    break;
  case SoKeyboardEvent::PAD_TAB:
    stringrep = "PAD_TAB";
    break;
  case SoKeyboardEvent::PAD_DELETE: // aka PAD_PERIOD
    stringrep = "PAD_DELETE";
    break;
  case SoKeyboardEvent::BACKSPACE:
    stringrep = "BACKSPACE";
    break;
  case SoKeyboardEvent::TAB:
    stringrep = "TAB";
    break;
  case SoKeyboardEvent::RETURN: // aka ENTER
    stringrep = "RETURN";
    break;
  case SoKeyboardEvent::PAUSE:
    stringrep = "PAUSE";
    break;
  case SoKeyboardEvent::SCROLL_LOCK:
    stringrep = "SCROLL_LOCK";
    break;
  case SoKeyboardEvent::ESCAPE:
    stringrep = "ESCAPE";
    break;
  case SoKeyboardEvent::KEY_DELETE:
    stringrep = "DELETE";
    break;
  case SoKeyboardEvent::PRINT:
    stringrep = "PRINT";
    break;
  case SoKeyboardEvent::INSERT:
    stringrep = "INSERT";
    break;
  case SoKeyboardEvent::NUM_LOCK:
    stringrep = "NUM_LOCK";
    break;
  case SoKeyboardEvent::CAPS_LOCK:
    stringrep = "CAPS_LOCK";
    break;
  case SoKeyboardEvent::SHIFT_LOCK:
    stringrep = "SHIFT_LOCK";
    break;
  case SoKeyboardEvent::SPACE:
    stringrep = "SPACE";
    break;
  case SoKeyboardEvent::APOSTROPHE:
    stringrep = "APOSTROPHE";
    break;
  case SoKeyboardEvent::COMMA:
    stringrep = "COMMA";
    break;
  case SoKeyboardEvent::MINUS:
    stringrep = "MINUS";
    break;
  case SoKeyboardEvent::PERIOD:
    stringrep = "PERIOD";
    break;
  case SoKeyboardEvent::SLASH:
    stringrep = "SLASH";
    break;
  case SoKeyboardEvent::SEMICOLON:
    stringrep = "SEMICOLON";
    break;
  case SoKeyboardEvent::EQUAL:
    stringrep = "EQUAL";
    break;
  case SoKeyboardEvent::BRACKETLEFT:
    stringrep = "BRACKETLEFT";
    break;
  case SoKeyboardEvent::BACKSLASH:
    stringrep = "BACKSLASH";
    break;
  case SoKeyboardEvent::BRACKETRIGHT:
    stringrep = "BRACKETRIGHT";
    break;
  case SoKeyboardEvent::GRAVE:
    stringrep = "GRAVE";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}
