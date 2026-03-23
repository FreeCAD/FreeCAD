#ifndef SMALLCHANGE_SWITCHBOARDOPERATOR_H
#define SMALLCHANGE_SWITCHBOARDOPERATOR_H

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

#ifdef _MSC_VER
#ifdef SWITCHBOARDOPERATOR_INTERNAL
#define SWITCHBOARDOPERATOR_API __declspec(dllexport)
#else
#define SWITCHBOARDOPERATOR_API __declspec(dllimport)
#endif
#else
#define SWITCHBOARDOPERATOR_API
#endif

#include "Switchboard.h"
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/events/SoKeyboardEvent.h>

class SWITCHBOARDOPERATOR_API SwitchboardOperator : public Switchboard {
  typedef Switchboard inherited;
  SO_NODE_HEADER(SwitchboardOperator);

public:
  static void initClass(void);
  SwitchboardOperator(void);
  SwitchboardOperator(int numchildren);

  enum Behavior {
    NONE, TOGGLE, HOLD, INVERSE_HOLD, TIME_HOLD
  };

  enum Key {
    ANY = SoKeyboardEvent::ANY,
    UNDEFINED = SoKeyboardEvent::UNDEFINED,
    LEFT_SHIFT = SoKeyboardEvent::LEFT_SHIFT,
    RIGHT_SHIFT = SoKeyboardEvent::RIGHT_SHIFT,
    LEFT_CONTROL = SoKeyboardEvent::LEFT_CONTROL,
    RIGHT_CONTROL = SoKeyboardEvent::RIGHT_CONTROL,
    LEFT_ALT = SoKeyboardEvent::LEFT_ALT,
    RIGHT_ALT = SoKeyboardEvent::RIGHT_ALT,
    CAPS_LOCK = SoKeyboardEvent::CAPS_LOCK,
    A = SoKeyboardEvent::A,
    B = SoKeyboardEvent::B,
    C = SoKeyboardEvent::C,
    D = SoKeyboardEvent::D,
    E = SoKeyboardEvent::E,
    F = SoKeyboardEvent::F,
    G = SoKeyboardEvent::G,
    H = SoKeyboardEvent::H,
    I = SoKeyboardEvent::I,
    J = SoKeyboardEvent::J,
    K = SoKeyboardEvent::K,
    L = SoKeyboardEvent::L,
    M = SoKeyboardEvent::M,
    N = SoKeyboardEvent::N,
    O = SoKeyboardEvent::O,
    P = SoKeyboardEvent::P,
    Q = SoKeyboardEvent::Q,
    R = SoKeyboardEvent::R,
    S = SoKeyboardEvent::S,
    T = SoKeyboardEvent::T,
    U = SoKeyboardEvent::U,
    V = SoKeyboardEvent::V,
    W = SoKeyboardEvent::W,
    X = SoKeyboardEvent::X,
    Y = SoKeyboardEvent::Y,
    Z = SoKeyboardEvent::Z,
    NUMBER_0 = SoKeyboardEvent::NUMBER_0,
    NUMBER_1 = SoKeyboardEvent::NUMBER_1,
    NUMBER_2 = SoKeyboardEvent::NUMBER_2,
    NUMBER_3 = SoKeyboardEvent::NUMBER_3,
    NUMBER_4 = SoKeyboardEvent::NUMBER_4,
    NUMBER_5 = SoKeyboardEvent::NUMBER_5,
    NUMBER_6 = SoKeyboardEvent::NUMBER_6,
    NUMBER_7 = SoKeyboardEvent::NUMBER_7,
    NUMBER_8 = SoKeyboardEvent::NUMBER_8,
    NUMBER_9 = SoKeyboardEvent::NUMBER_9,
    MINUS = SoKeyboardEvent::MINUS,
    EQUAL = SoKeyboardEvent::EQUAL,
    SPACE = SoKeyboardEvent::SPACE,
    BACKSPACE = SoKeyboardEvent::BACKSPACE,
    TAB = SoKeyboardEvent::TAB,
    RETURN = SoKeyboardEvent::RETURN,
    BRACKETLEFT = SoKeyboardEvent::BRACKETLEFT,
    BRACKETRIGHT = SoKeyboardEvent::BRACKETRIGHT,
    SEMICOLON = SoKeyboardEvent::SEMICOLON,
    APOSTROPHE = SoKeyboardEvent::APOSTROPHE,
    COMMA = SoKeyboardEvent::COMMA,
    PERIOD = SoKeyboardEvent::PERIOD,
    SLASH = SoKeyboardEvent::SLASH,
    BACKSLASH = SoKeyboardEvent::BACKSLASH,
    GRAVE = SoKeyboardEvent::GRAVE
  };

  SoMFEnum key;
  SoMFEnum behavior;
  SoMFInt32 msecs;

  virtual void handleEvent(SoHandleEventAction * action);

protected:
  virtual ~SwitchboardOperator(void);

private:
  void constructor(void);

};

#endif // !SMALLCHANGE_SWITCHBOARDOPERATOR_H
