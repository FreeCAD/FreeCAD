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
  \class SoEvent SoEvent.h Inventor/events/SoEvent.h
  \brief The SoEvent class is the base class for all Coin events.

  \ingroup coin_events

  Coin contains its own set of event classes, independent of the underlying
  window system.

  Upon system specific events, a translation is done by the window
  specific device classes into one of the Coin event object classes
  listed below. The event is then typically sent by the render area
  to an SoSceneManager which will apply it to the scene graph through
  an SoHandleEventAction.

  Events may be caught by the user by attaching an SoEventCallback
  node to the scene graph, or they can automatically be handled by a
  dragger or manipulator in the graph.

  \sa SoButtonEvent, SoKeyboardEvent, SoLocation2Event, SoMotion3Event
  \sa SoMouseButtonEvent, SoSpaceballButtonEvent
  \sa SoEventCallback, SoHandleEventAction
*/

#include <Inventor/events/SoEvent.h>

#include <cassert>

#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoSpaceballButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMotion3Event.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbName.h>

#include "tidbitsp.h"

SO_EVENT_ABSTRACT_SOURCE(SoEvent);

/*!
  \fn SoType SoEvent::getClassTypeId(void)

  This static method returns the SoType object associated with objects
  of this class.
 */


// FIXME: grab better version of getTypeId() doc from SoBase, SoAction
// and / or SoDetail. 20010913 mortene.
/*!
  \fn SoType SoEvent::getTypeId(void) const

  Returns the actual type id of an instantiated object.
*/


/*!
  Initialize SoEvent and all its known subclasses (i.e. all subclasses
  which are part of the standard classes in the Coin library).

  This method is called from SoDB::init(), so it is very unlikely that
  you will have to call it explicitly.
 */
void
SoEvent::initClass(void)
{
  // Make sure we only initialize once.
  assert(SoEvent::classTypeId == SoType::badType());

  SoEvent::classTypeId = SoType::createType(SoType::badType(), "SoEvent");
  coin_atexit(reinterpret_cast<coin_atexit_f *>(cleanupClass), CC_ATEXIT_NORMAL);

  SoEvent::initEvents();
}

/*!
  \COININTERNAL

  Initialize all known subclasses.
 */
void
SoEvent::initEvents(void)
{
  SoButtonEvent::initClass();
    SoMouseButtonEvent::initClass();
    SoKeyboardEvent::initClass();
    SoSpaceballButtonEvent::initClass();
  SoLocation2Event::initClass();
  SoMotion3Event::initClass();
}

/*!
  Constructor, will set all modifiers to "off" state.
*/
SoEvent::SoEvent(void)
{
  this->modifiers.shiftdown = 0;
  this->modifiers.ctrldown = 0;
  this->modifiers.altdown = 0;
}

/*!
  Destructor.
 */
SoEvent::~SoEvent()
{
}

/*!
  Returns TRUE if this object either has the same type as the given
  \c type parameter, or if \c type belongs to a superclass of ourselves.
*/
SbBool
SoEvent::isOfType(SoType type) const
{
  const SoType myType = this->getTypeId();
  if (myType == type) return TRUE;
  if (myType.isDerivedFrom(type)) return TRUE;
  return FALSE;
}

/*!
  From a system specific device object, set the time the event occurred.

  \sa getTime()
 */
void
SoEvent::setTime(const SbTime t)
{
  this->timeofevent = t;
}

/*!
  Returns the time the event occurred.

  \sa getPosition(), wasShiftDown(), wasCtrlDown(), wasAltDown()
 */
SbTime
SoEvent::getTime(void) const
{
  return this->timeofevent;
}

/*!
  From a system specific device object, set the mouse pointer position
  when the event occurred.

  \sa getPosition(), getNormalizedPosition()
 */
void
SoEvent::setPosition(const SbVec2s & p)
{
  this->positionofevent = p;
}

// FIXME: "window" below is ambiguous, replace with something less
// generic, like e.g. "the rendering canvas" or some such. Should also
// scan API docs for other references to "window" (and "widget"?) and
// do likewise. 20040728 mortene.
/*!
  Returns the mouse pointer position when the event occurred. The
  coordinates are given relative to the window coordinates.

  \sa getNormalizedPosition(), getTime(), wasShiftDown(), wasCtrlDown(),
  \sa wasAltDown()
 */
const SbVec2s &
SoEvent::getPosition(void) const
{
  return this->positionofevent;
}

/*!
  Returns the mouse pointer position when the event occurred. The
  coordinates are given relative to the viewport coordinates.

  \sa getNormalizedPosition(), getTime(), wasShiftDown(), wasCtrlDown(),
  \sa wasAltDown()
 */
const SbVec2s &
SoEvent::getPosition(const SbViewportRegion & vpRgn) const
{
  positionVP = SbVec2s(this->positionofevent - vpRgn.getViewportOriginPixels());
  
  return positionVP;
}

/*!
  Returns the mouse pointer position when the event occurred. The
  coordinates are given relative to the viewport coordinates,
  normalized according to the size of the viewport.

  \sa getPosition(), getTime(), wasShiftDown(), wasCtrlDown(), wasAltDown()
 */
const SbVec2f &
SoEvent::getNormalizedPosition(const SbViewportRegion & vpRgn) const
{
  SbVec2s p = this->positionofevent - vpRgn.getViewportOriginPixels();
  SbVec2s s = vpRgn.getViewportSizePixels();

  positionVPNorm = SbVec2f(
                           static_cast<float>(p[0])/static_cast<float>(s[0]),
                           static_cast<float>(p[1])/static_cast<float>(s[1])
                           );

  return positionVPNorm;
}

/*!
  From a system specific device object, set the state of the Shift key(s)
  when the event occurred.

  \sa wasShiftDown(), setCtrlDown(), setAltDown()
 */
void
SoEvent::setShiftDown(SbBool isDown)
{
  this->modifiers.shiftdown = isDown ? TRUE : FALSE;
}

/*!
  Returns state of Shift key(s) when the event occurred.

  \sa wasCtrlDown(), wasAltDown(), getPosition(), getTime()
 */
SbBool
SoEvent::wasShiftDown(void) const
{
  return this->modifiers.shiftdown;
}

/*!
  From a system specific device object, set the state of the Ctrl key(s)
  when the event occurred.

  \sa wasCtrlDown(), setShiftDown(), setAltDown()
 */
void
SoEvent::setCtrlDown(SbBool isDown)
{
  this->modifiers.ctrldown = isDown ? TRUE : FALSE;
}

/*!
  Returns state of Ctrl key(s) when the event occurred.

  \sa wasShiftDown(), wasAltDown(), getPosition(), getTime()
 */
SbBool
SoEvent::wasCtrlDown(void) const
{
  return this->modifiers.ctrldown;
}

/*!
  From a system specific device object, set the state of the Alt key(s)
  when the event occurred.

  \sa wasAltDown(), setCtrlDown(), setShiftDown()
 */
void
SoEvent::setAltDown(SbBool isDown)
{
  this->modifiers.altdown = isDown ? TRUE : FALSE;
}

/*!
  Returns state of Alt key(s) when the event occurred.

  \sa wasShiftDown(), wasCtrlDown(), getPosition(), getTime()
 */
SbBool
SoEvent::wasAltDown(void) const
{
  return this->modifiers.altdown;
}
