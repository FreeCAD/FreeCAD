#ifndef COIN_SOKEYBOARDEVENT_H
#define COIN_SOKEYBOARDEVENT_H

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

#include <Inventor/events/SoButtonEvent.h>

// Avoid problem with Microsoft Visual C++ Win32 API headers (yes,
// they actually #define DELETE in their WINNT.H header file).
#ifdef DELETE
#define SOKEYBOARDEVENT_UNDEF_DELETE
#undef DELETE
#endif // DELETE


#define SO_KEY_PRESS_EVENT(EVENT, KEY) \
 (SoKeyboardEvent::isKeyPressEvent(EVENT, SoKeyboardEvent::KEY))

#define SO_KEY_RELEASE_EVENT(EVENT, KEY) \
 (SoKeyboardEvent::isKeyReleaseEvent(EVENT, SoKeyboardEvent::KEY))


class COIN_DLL_API SoKeyboardEvent : public SoButtonEvent {
  typedef SoButtonEvent inherited;
  SO_EVENT_HEADER();

public:
  static void initClass(void);

  SoKeyboardEvent(void);
  virtual ~SoKeyboardEvent();

  enum Key {
    ANY = 0,
    UNDEFINED = 1,

    LEFT_SHIFT = 0xffe1, RIGHT_SHIFT, LEFT_CONTROL, RIGHT_CONTROL,
    LEFT_ALT = 0xffe9, RIGHT_ALT,

    NUMBER_0 = 0x0030, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4, NUMBER_5,
    NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9,

    A = 0x0061, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T,
    U, V, W, X, Y, Z,

    HOME = 0xff50, LEFT_ARROW, UP_ARROW, RIGHT_ARROW, DOWN_ARROW,
    PAGE_UP, PAGE_DOWN, END,
    PRIOR = 0xff55, NEXT,

    PAD_ENTER = 0xff8d,
    PAD_F1 = 0xff91, PAD_F2, PAD_F3, PAD_F4,
    PAD_0 = 0xff9e, PAD_1 = 0xff9c, PAD_2 = 0xff99, PAD_3 = 0xff9b,
    PAD_4 = 0xff96, PAD_5 = 0xff9d, PAD_6 = 0xff98, PAD_7 = 0xff95,
    PAD_8 = 0xff97, PAD_9 = 0xff9a,
    PAD_ADD = 0xffab, PAD_SUBTRACT = 0xffad,
    PAD_MULTIPLY = 0xffaa, PAD_DIVIDE = 0xffaf,
    PAD_SPACE = 0xff8d, PAD_TAB = 0xff89,
    PAD_INSERT = 0xff9e, PAD_DELETE = 0xff9f, PAD_PERIOD = 0xff9f,

    F1 = 0xffbe, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    BACKSPACE = 0xff08, TAB = 0xff09,
    RETURN = 0xff0d, ENTER = 0xff0d,
    PAUSE = 0xff13, SCROLL_LOCK = 0xff14,
    ESCAPE = 0xff1b, DELETE = 0xffff, KEY_DELETE = DELETE,
    PRINT = 0xff61, INSERT = 0xff63,
    NUM_LOCK = 0xff7f, CAPS_LOCK = 0xffe5, SHIFT_LOCK = 0xffe6,

    SPACE = 0x0020, APOSTROPHE = 0x0027,
    COMMA = 0x002c, MINUS = 0x002d, PERIOD = 0x002e, SLASH = 0x002f,
    SEMICOLON = 0x003b, EQUAL = 0x003d,
    BRACKETLEFT = 0x005b, BACKSLASH = 0x005c,
    BRACKETRIGHT = 0x005d, GRAVE = 0x0060
  };

  void setKey(Key key);
  Key getKey(void) const;

  void setPrintableCharacter(const char c);
  char getPrintableCharacter(void) const;

  static SbBool isKeyPressEvent(const SoEvent * e, Key whichKey);
  static SbBool isKeyReleaseEvent(const SoEvent * e, Key whichKey);

  static SbBool enumToString(Key enumval, SbString & stringrep);

private:
  Key key;
  char printable;
  char isprintableset;

}; // SoKeyboardEvent

// Avoid problem with Microsoft Win32 API headers (see above). Define
// DELETE back to its value in the MSVC header file.
//
// FIXME: we shouldn't uncritically trust this value to come from the
// MSVC headers, but rather check in the block at the top to see that
// it matches the value we believe it does. Alternatively, we could
// just don't bother to set it back -- it seems quite unlikely that
// this would break any client code, but if so, it would be a simple
// fix on the client side to get around it -- just rearrange
// headers. 20040629 mortene.
#ifdef SOKEYBOARDEVENT_UNDEF_DELETE
#define DELETE (0x00010000L)
#undef SOKEYBOARDEVENT_UNDEF_DELETE
#endif // SOKEYBOARDEVENT_UNDEF_DELETE

#endif // !COIN_SOKEYBOARDEVENT_H
