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

#include <Inventor/errors/SoDebugError.h>

#include "KeyboardP.h"
#include "devices/Keyboard.h"


using namespace SIM::Coin3D::Quarter;

#define PUBLIC(obj) obj->publ

KeyboardP::KeyboardP(Keyboard * publ)
{
  PUBLIC(this) = publ;
  this->keyboard = new SoKeyboardEvent;

  if (!keyboardmap) {
    keyboardmap = new KeyMap;
    keypadmap = new KeyMap;
    this->initKeyMap();
  }
}

KeyboardP::~KeyboardP()
{
  delete this->keyboard;
}

bool
KeyboardP::debugKeyEvents()
{
  const char * env = coin_getenv("QUARTER_DEBUG_KEYEVENTS");
  return env && (atoi(env) > 0);
}

const SoEvent *
KeyboardP::keyEvent(QKeyEvent * qevent)
{
  const Qt::KeyboardModifiers modifiers = qevent->modifiers();

  this->keyboard->setPosition(PUBLIC(this)->mousepos);
  PUBLIC(this)->setModifiers(this->keyboard, qevent);

  (qevent->type() == QEvent::KeyPress) ?
    this->keyboard->setState(SoButtonEvent::DOWN):
    this->keyboard->setState(SoButtonEvent::UP);

  Qt::Key qkey = (Qt::Key) qevent->key();

  SoKeyboardEvent::Key sokey = (modifiers & Qt::KeypadModifier) ?
    keypadmap->value(qkey, SoKeyboardEvent::ANY) :
    keyboardmap->value(qkey, SoKeyboardEvent::ANY);

  //Need to use a temporary to avoid reference becoming deleted before
  //we get a hold of it.
  QByteArray tmp = qevent->text().toLatin1();
  const char * printable = tmp.constData();
  this->keyboard->setPrintableCharacter(*printable);
  this->keyboard->setKey(sokey);

#if QUARTER_DEBUG
  if (KeyboardP::debugKeyEvents()) {
    SbString s;
    this->keyboard->enumToString(this->keyboard->getKey(), s);
    SoDebugError::postInfo("KeyboardP::keyEvent",
                           "enum: '%s', pos: <%i %i>, printable: '%s'",
                           s.getString(),
                           PUBLIC(this)->mousepos[0],
                           PUBLIC(this)->mousepos[1],
                           printable);
  }
#endif
  return this->keyboard;
}

KeyboardP::KeyMap * KeyboardP::keyboardmap = nullptr;
KeyboardP::KeyMap * KeyboardP::keypadmap = nullptr;

void
KeyboardP::initKeyMap()
{
  // keyboard
  keyboardmap->insert(Qt::Key_Shift,   SoKeyboardEvent::LEFT_SHIFT);
  keyboardmap->insert(Qt::Key_Alt,     SoKeyboardEvent::LEFT_ALT);
  keyboardmap->insert(Qt::Key_Control, SoKeyboardEvent::LEFT_CONTROL);
  keyboardmap->insert(Qt::Key_0, SoKeyboardEvent::NUMBER_0);
  keyboardmap->insert(Qt::Key_1, SoKeyboardEvent::NUMBER_1);
  keyboardmap->insert(Qt::Key_2, SoKeyboardEvent::NUMBER_2);
  keyboardmap->insert(Qt::Key_3, SoKeyboardEvent::NUMBER_3);
  keyboardmap->insert(Qt::Key_4, SoKeyboardEvent::NUMBER_4);
  keyboardmap->insert(Qt::Key_5, SoKeyboardEvent::NUMBER_5);
  keyboardmap->insert(Qt::Key_6, SoKeyboardEvent::NUMBER_6);
  keyboardmap->insert(Qt::Key_7, SoKeyboardEvent::NUMBER_7);
  keyboardmap->insert(Qt::Key_8, SoKeyboardEvent::NUMBER_8);
  keyboardmap->insert(Qt::Key_9, SoKeyboardEvent::NUMBER_9);

  keyboardmap->insert(Qt::Key_A, SoKeyboardEvent::A);
  keyboardmap->insert(Qt::Key_B, SoKeyboardEvent::B);
  keyboardmap->insert(Qt::Key_C, SoKeyboardEvent::C);
  keyboardmap->insert(Qt::Key_D, SoKeyboardEvent::D);
  keyboardmap->insert(Qt::Key_E, SoKeyboardEvent::E);
  keyboardmap->insert(Qt::Key_F, SoKeyboardEvent::F);
  keyboardmap->insert(Qt::Key_G, SoKeyboardEvent::G);
  keyboardmap->insert(Qt::Key_H, SoKeyboardEvent::H);
  keyboardmap->insert(Qt::Key_I, SoKeyboardEvent::I);
  keyboardmap->insert(Qt::Key_J, SoKeyboardEvent::J);
  keyboardmap->insert(Qt::Key_K, SoKeyboardEvent::K);
  keyboardmap->insert(Qt::Key_L, SoKeyboardEvent::L);
  keyboardmap->insert(Qt::Key_M, SoKeyboardEvent::M);
  keyboardmap->insert(Qt::Key_N, SoKeyboardEvent::N);
  keyboardmap->insert(Qt::Key_O, SoKeyboardEvent::O);
  keyboardmap->insert(Qt::Key_P, SoKeyboardEvent::P);
  keyboardmap->insert(Qt::Key_Q, SoKeyboardEvent::Q);
  keyboardmap->insert(Qt::Key_R, SoKeyboardEvent::R);
  keyboardmap->insert(Qt::Key_S, SoKeyboardEvent::S);
  keyboardmap->insert(Qt::Key_T, SoKeyboardEvent::T);
  keyboardmap->insert(Qt::Key_U, SoKeyboardEvent::U);
  keyboardmap->insert(Qt::Key_V, SoKeyboardEvent::V);
  keyboardmap->insert(Qt::Key_W, SoKeyboardEvent::W);
  keyboardmap->insert(Qt::Key_X, SoKeyboardEvent::X);
  keyboardmap->insert(Qt::Key_Y, SoKeyboardEvent::Y);
  keyboardmap->insert(Qt::Key_Z, SoKeyboardEvent::Z);

  keyboardmap->insert(Qt::Key_Home,     SoKeyboardEvent::HOME);
  keyboardmap->insert(Qt::Key_Left,     SoKeyboardEvent::LEFT_ARROW);
  keyboardmap->insert(Qt::Key_Up,       SoKeyboardEvent::UP_ARROW);
  keyboardmap->insert(Qt::Key_Right,    SoKeyboardEvent::RIGHT_ARROW);
  keyboardmap->insert(Qt::Key_Down,     SoKeyboardEvent::DOWN_ARROW);
  keyboardmap->insert(Qt::Key_PageUp,   SoKeyboardEvent::PAGE_UP);
  keyboardmap->insert(Qt::Key_PageDown, SoKeyboardEvent::PAGE_DOWN);
  keyboardmap->insert(Qt::Key_End,      SoKeyboardEvent::END);

  keyboardmap->insert(Qt::Key_F1,  SoKeyboardEvent::F1);
  keyboardmap->insert(Qt::Key_F2,  SoKeyboardEvent::F2);
  keyboardmap->insert(Qt::Key_F3,  SoKeyboardEvent::F3);
  keyboardmap->insert(Qt::Key_F4,  SoKeyboardEvent::F4);
  keyboardmap->insert(Qt::Key_F5,  SoKeyboardEvent::F5);
  keyboardmap->insert(Qt::Key_F6,  SoKeyboardEvent::F6);
  keyboardmap->insert(Qt::Key_F7,  SoKeyboardEvent::F7);
  keyboardmap->insert(Qt::Key_F8,  SoKeyboardEvent::F8);
  keyboardmap->insert(Qt::Key_F9,  SoKeyboardEvent::F9);
  keyboardmap->insert(Qt::Key_F10, SoKeyboardEvent::F10);
  keyboardmap->insert(Qt::Key_F11, SoKeyboardEvent::F11);
  keyboardmap->insert(Qt::Key_F12, SoKeyboardEvent::F12);

  keyboardmap->insert(Qt::Key_Backspace,  SoKeyboardEvent::BACKSPACE);
  keyboardmap->insert(Qt::Key_Tab,        SoKeyboardEvent::TAB);
  keyboardmap->insert(Qt::Key_Return,     SoKeyboardEvent::RETURN);
  keyboardmap->insert(Qt::Key_Enter,      SoKeyboardEvent::ENTER);
  keyboardmap->insert(Qt::Key_Pause,      SoKeyboardEvent::PAUSE);
  keyboardmap->insert(Qt::Key_ScrollLock, SoKeyboardEvent::SCROLL_LOCK);
  keyboardmap->insert(Qt::Key_Escape,     SoKeyboardEvent::ESCAPE);
  keyboardmap->insert(Qt::Key_Delete,     SoKeyboardEvent::DELETE);
  keyboardmap->insert(Qt::Key_Print,      SoKeyboardEvent::PRINT);
  keyboardmap->insert(Qt::Key_Insert,     SoKeyboardEvent::INSERT);
  keyboardmap->insert(Qt::Key_NumLock,    SoKeyboardEvent::NUM_LOCK);
  keyboardmap->insert(Qt::Key_CapsLock,   SoKeyboardEvent::CAPS_LOCK);

  keyboardmap->insert(Qt::Key_Space,        SoKeyboardEvent::SPACE);
  keyboardmap->insert(Qt::Key_Apostrophe,   SoKeyboardEvent::APOSTROPHE);
  keyboardmap->insert(Qt::Key_Comma,        SoKeyboardEvent::COMMA);
  keyboardmap->insert(Qt::Key_Minus,        SoKeyboardEvent::MINUS);
  keyboardmap->insert(Qt::Key_Period,       SoKeyboardEvent::PERIOD);
  keyboardmap->insert(Qt::Key_Slash,        SoKeyboardEvent::SLASH);
  keyboardmap->insert(Qt::Key_Semicolon,    SoKeyboardEvent::SEMICOLON);
  keyboardmap->insert(Qt::Key_Equal,        SoKeyboardEvent::EQUAL);
  keyboardmap->insert(Qt::Key_BracketLeft,  SoKeyboardEvent::BRACKETLEFT);
  keyboardmap->insert(Qt::Key_BracketRight, SoKeyboardEvent::BRACKETRIGHT);
  keyboardmap->insert(Qt::Key_Backslash,    SoKeyboardEvent::BACKSLASH);
  keyboardmap->insert(Qt::Key_Agrave,       SoKeyboardEvent::GRAVE);

  // keypad

  // on Mac OS X, the keypad modifier will also be set when an arrow
  // key is pressed as the arrow keys are considered part of the
  // keypad
  keypadmap->insert(Qt::Key_Left,     SoKeyboardEvent::LEFT_ARROW);
  keypadmap->insert(Qt::Key_Up,       SoKeyboardEvent::UP_ARROW);
  keypadmap->insert(Qt::Key_Right,    SoKeyboardEvent::RIGHT_ARROW);
  keypadmap->insert(Qt::Key_Down,     SoKeyboardEvent::DOWN_ARROW);

  keypadmap->insert(Qt::Key_Enter,    SoKeyboardEvent::PAD_ENTER);
  keypadmap->insert(Qt::Key_F1,       SoKeyboardEvent::PAD_F1);
  keypadmap->insert(Qt::Key_F2,       SoKeyboardEvent::PAD_F2);
  keypadmap->insert(Qt::Key_F3,       SoKeyboardEvent::PAD_F3);
  keypadmap->insert(Qt::Key_F4,       SoKeyboardEvent::PAD_F4);
  keypadmap->insert(Qt::Key_0,        SoKeyboardEvent::PAD_0);
  keypadmap->insert(Qt::Key_1,        SoKeyboardEvent::PAD_1);
  keypadmap->insert(Qt::Key_2,        SoKeyboardEvent::PAD_2);
  keypadmap->insert(Qt::Key_3,        SoKeyboardEvent::PAD_3);
  keypadmap->insert(Qt::Key_4,        SoKeyboardEvent::PAD_4);
  keypadmap->insert(Qt::Key_5,        SoKeyboardEvent::PAD_5);
  keypadmap->insert(Qt::Key_6,        SoKeyboardEvent::PAD_6);
  keypadmap->insert(Qt::Key_7,        SoKeyboardEvent::PAD_7);
  keypadmap->insert(Qt::Key_8,        SoKeyboardEvent::PAD_8);
  keypadmap->insert(Qt::Key_9,        SoKeyboardEvent::PAD_9);
  keypadmap->insert(Qt::Key_Plus,     SoKeyboardEvent::PAD_ADD);
  keypadmap->insert(Qt::Key_Minus,    SoKeyboardEvent::PAD_SUBTRACT);
  keypadmap->insert(Qt::Key_multiply, SoKeyboardEvent::PAD_MULTIPLY);
  keypadmap->insert(Qt::Key_division, SoKeyboardEvent::PAD_DIVIDE);
  keypadmap->insert(Qt::Key_Tab,      SoKeyboardEvent::PAD_TAB);
  keypadmap->insert(Qt::Key_Space,    SoKeyboardEvent::PAD_SPACE);
  keypadmap->insert(Qt::Key_Insert,   SoKeyboardEvent::PAD_INSERT);
  keypadmap->insert(Qt::Key_Delete,   SoKeyboardEvent::PAD_DELETE);
  keypadmap->insert(Qt::Key_Period,   SoKeyboardEvent::PAD_PERIOD);

}

#undef PUBLIC
