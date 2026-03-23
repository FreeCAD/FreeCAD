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
  \class Switchboard Switchboard.h SmallChange/nodes/Switchboard.h
  \brief The Switchboard class is a group node that can toggle children
  on and off arbitrarily based on keyboard events.
*/

// FIXME: implement proper searching / SearchAction handling  2002-02-07 larsa
//   search should probably just traverse each child once in ChildList order
// FIXME: implement proper writing / WriteAction handling  2002-02-07 larsa
//   write should just traverse each child once in ChildList order and
//   write fields until there's only defaults left in the arrays

#include "SwitchboardOperator.h"
#include <Inventor/nodes/SoSubNode.h>

#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoKeyboardEvent.h>

#include <Inventor/errors/SoDebugError.h>

SO_NODE_SOURCE(SwitchboardOperator);

void
SwitchboardOperator::initClass(void)
{
  SO_NODE_INIT_CLASS(SwitchboardOperator, Switchboard, Switchboard);
}

SwitchboardOperator::SwitchboardOperator(void)
{
  this->constructor();
}

SwitchboardOperator::SwitchboardOperator(int numchildren)
: inherited(numchildren)
{
  this->constructor();
}

void
SwitchboardOperator::constructor(void) // private
{
  SO_NODE_CONSTRUCTOR(SwitchboardOperator);

  SO_NODE_ADD_FIELD(key, (UNDEFINED));
  SO_NODE_ADD_FIELD(behavior, (TOGGLE));
  SO_NODE_ADD_FIELD(msecs, (0));

  // FIXME: complete this list
  SO_NODE_DEFINE_ENUM_VALUE(Key, LEFT_SHIFT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, RIGHT_SHIFT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, LEFT_CONTROL);
  SO_NODE_DEFINE_ENUM_VALUE(Key, RIGHT_CONTROL);
  SO_NODE_DEFINE_ENUM_VALUE(Key, LEFT_ALT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, RIGHT_ALT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, CAPS_LOCK);

  SO_NODE_DEFINE_ENUM_VALUE(Key, ANY);
  SO_NODE_DEFINE_ENUM_VALUE(Key, UNDEFINED);
  SO_NODE_DEFINE_ENUM_VALUE(Key, A);
  SO_NODE_DEFINE_ENUM_VALUE(Key, B);
  SO_NODE_DEFINE_ENUM_VALUE(Key, C);
  SO_NODE_DEFINE_ENUM_VALUE(Key, D);
  SO_NODE_DEFINE_ENUM_VALUE(Key, E);
  SO_NODE_DEFINE_ENUM_VALUE(Key, F);
  SO_NODE_DEFINE_ENUM_VALUE(Key, G);
  SO_NODE_DEFINE_ENUM_VALUE(Key, H);
  SO_NODE_DEFINE_ENUM_VALUE(Key, I);
  SO_NODE_DEFINE_ENUM_VALUE(Key, J);
  SO_NODE_DEFINE_ENUM_VALUE(Key, K);
  SO_NODE_DEFINE_ENUM_VALUE(Key, L);
  SO_NODE_DEFINE_ENUM_VALUE(Key, M);
  SO_NODE_DEFINE_ENUM_VALUE(Key, N);
  SO_NODE_DEFINE_ENUM_VALUE(Key, O);
  SO_NODE_DEFINE_ENUM_VALUE(Key, P);
  SO_NODE_DEFINE_ENUM_VALUE(Key, Q);
  SO_NODE_DEFINE_ENUM_VALUE(Key, R);
  SO_NODE_DEFINE_ENUM_VALUE(Key, S);
  SO_NODE_DEFINE_ENUM_VALUE(Key, T);
  SO_NODE_DEFINE_ENUM_VALUE(Key, U);
  SO_NODE_DEFINE_ENUM_VALUE(Key, V);
  SO_NODE_DEFINE_ENUM_VALUE(Key, W);
  SO_NODE_DEFINE_ENUM_VALUE(Key, X);
  SO_NODE_DEFINE_ENUM_VALUE(Key, Y);
  SO_NODE_DEFINE_ENUM_VALUE(Key, Z);

  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_0);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_1);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_2);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_3);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_4);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_5);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_6);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_7);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_8);
  SO_NODE_DEFINE_ENUM_VALUE(Key, NUMBER_9);
  SO_NODE_DEFINE_ENUM_VALUE(Key, MINUS);
  SO_NODE_DEFINE_ENUM_VALUE(Key, EQUAL);

  SO_NODE_DEFINE_ENUM_VALUE(Key, SPACE);
  SO_NODE_DEFINE_ENUM_VALUE(Key, BACKSPACE);
  SO_NODE_DEFINE_ENUM_VALUE(Key, TAB);
  SO_NODE_DEFINE_ENUM_VALUE(Key, RETURN);
  SO_NODE_DEFINE_ENUM_VALUE(Key, BRACKETLEFT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, BRACKETRIGHT);
  SO_NODE_DEFINE_ENUM_VALUE(Key, SEMICOLON);
  SO_NODE_DEFINE_ENUM_VALUE(Key, APOSTROPHE);
  SO_NODE_DEFINE_ENUM_VALUE(Key, COMMA);
  SO_NODE_DEFINE_ENUM_VALUE(Key, PERIOD);
  SO_NODE_DEFINE_ENUM_VALUE(Key, SLASH);
  SO_NODE_DEFINE_ENUM_VALUE(Key, BACKSLASH);
  SO_NODE_DEFINE_ENUM_VALUE(Key, GRAVE);

  SO_NODE_DEFINE_ENUM_VALUE(Behavior, NONE);
  SO_NODE_DEFINE_ENUM_VALUE(Behavior, TOGGLE);
  SO_NODE_DEFINE_ENUM_VALUE(Behavior, HOLD);
  SO_NODE_DEFINE_ENUM_VALUE(Behavior, INVERSE_HOLD);
  SO_NODE_DEFINE_ENUM_VALUE(Behavior, TIME_HOLD);

  SO_NODE_SET_SF_ENUM_TYPE(key, Key);
  SO_NODE_SET_SF_ENUM_TYPE(behavior, Behavior);
}

SwitchboardOperator::~SwitchboardOperator(void) // virtual, protected
{
}

void
SwitchboardOperator::handleEvent(SoHandleEventAction * action)
{
  const SoEvent * ev = action->getEvent();
  if ( ev->isOfType(SoKeyboardEvent::getClassTypeId()) ) {
    const SoKeyboardEvent * event = (const SoKeyboardEvent *) ev;
    SoKeyboardEvent::Key key = event->getKey();
    for ( int idx = 0; idx < this->key.getNum(); idx++ ) {
      if ( this->key[idx] == key ) {
        switch ( idx < this->behavior.getNum() ? this->behavior[idx] : TOGGLE ) {
        case TOGGLE:
          if ( event->getState() == SoKeyboardEvent::DOWN ) {
            if ( idx >= this->enable.getNum() ) this->enable.setNum(idx+1);
            this->enable.set1Value(idx, this->enable[idx] ? FALSE : TRUE);
          }
          break;
        case HOLD:
          if ( idx >= this->enable.getNum() ) this->enable.setNum(idx+1);
          this->enable.set1Value(idx, event->getState() == SoKeyboardEvent::DOWN ? TRUE : FALSE);
          break;
        case INVERSE_HOLD:
          if ( idx >= this->enable.getNum() ) this->enable.setNum(idx+1);
          this->enable.set1Value(idx, event->getState() == SoKeyboardEvent::DOWN ? FALSE : TRUE);
          break;
        case TIME_HOLD:
          SoDebugError::postInfo("SwitchboardOperator::handleEvent", "not implemented yet");
          break;
        default:
          break;
        }
      }
    }
  }
}
