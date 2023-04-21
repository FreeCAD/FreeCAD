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
  \class SIM::Coin3D::Quarter::Keyboard Keyboard.h Quarter/devices/Keyboard.h

  \brief The Keyboard class provides translation of keyboard events on
  the QuarterWidget.
*/

#ifdef _MSC_VER
#pragma warning(disable : 4267)
#endif

#include <QEvent>
#include <QKeyEvent>
#include <Inventor/events/SoEvent.h>

#include "KeyboardP.h"
#include "devices/Keyboard.h"


using namespace SIM::Coin3D::Quarter;

#define PRIVATE(obj) obj->pimpl

Keyboard::Keyboard()
{
  PRIVATE(this) = new KeyboardP(this);
}

Keyboard::Keyboard(QuarterWidget* quarter) :
    InputDevice(quarter)
{
    PRIVATE(this) = new KeyboardP(this);
}

Keyboard::~Keyboard()
{
  delete PRIVATE(this);
}

/*! Translates from QKeyEvents to SoKeyboardEvents
 */
const SoEvent *
Keyboard::translateEvent(QEvent * event)
{
  switch (event->type()) {
  case QEvent::KeyPress:
  case QEvent::KeyRelease:
    return PRIVATE(this)->keyEvent((QKeyEvent *) event);
  default:
    return nullptr;
  }
}

#undef PRIVATE
