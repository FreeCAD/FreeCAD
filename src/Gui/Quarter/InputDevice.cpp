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
#pragma warning(disable : 4267)
#endif

#include <QInputEvent>
#include <Inventor/events/SoEvents.h>

#include "devices/InputDevice.h"


using namespace SIM::Coin3D::Quarter;

/*!
  \class SIM::Coin3D::Quarter::InputDevice InputDevice.h Quarter/devices/InputDevice.h

  \brief The InputDevice class is the base class for devices such as
  the Keyboard and Mouse. It can be subclassed to support other
  devices.
*/

InputDevice::InputDevice() : quarter(nullptr)
{
  this->mousepos = SbVec2s(0, 0);
}

InputDevice::InputDevice(QuarterWidget *quarter) : quarter(quarter)
{
    this->mousepos = SbVec2s(0, 0);
}

/*!
  Sets the mouseposition

  \param[in] pos position of mouse in pixelcoordinates
*/

void
InputDevice::setMousePosition(const SbVec2s & pos)
{
  this->mousepos = pos;
}

/*!
  Sets the window size of the owning window

  \param[in] size in pixels
*/
void
InputDevice::setWindowSize(const SbVec2s & size)
{
  this->windowsize = size;
}

/*!
  Transforms a qevent into an soevent

  \param[in,out] soevent the transformed event
  \param[in] qevent incoming qevent
*/
void
InputDevice::setModifiers(SoEvent * soevent, const QInputEvent * qevent)
{
  // FIXME: How do we get the time from the qevent? (20070306 frodo)
  soevent->setTime(SbTime::getTimeOfDay());

  // Note: On Mac OS X, the ControlModifier value corresponds to the
  // Command keys on the Macintosh keyboard, and the MetaModifier
  // value corresponds to the Control keys.
  soevent->setShiftDown(qevent->modifiers() & Qt::ShiftModifier);
  soevent->setAltDown(qevent->modifiers() & Qt::AltModifier);
  soevent->setCtrlDown(qevent->modifiers() & Qt::ControlModifier);
}

/*!
  \var InputDevice::mousepos

  Holds the last known position of the mouse. This should be set even
  for a keyboard event.
*/

/*!
  \var InputDevice::windowsize

  Holds the size of the owning window
*/

#undef PRIVATE
