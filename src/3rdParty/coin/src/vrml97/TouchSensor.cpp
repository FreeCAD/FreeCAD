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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLTouchSensor SoVRMLTouchSensor.h Inventor/VRMLnodes/SoVRMLTouchSensor.h
  \brief The SoVRMLTouchSensor class tracks to pointer position and sends events based on user interaction.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  TouchSensor {
    exposedField SFBool  enabled TRUE
    eventOut     SFVec3f hitNormal_changed
    eventOut     SFVec3f hitPoint_changed
    eventOut     SFVec2f hitTexCoord_changed
    eventOut     SFBool  isActive
    eventOut     SFBool  isOver
    eventOut     SFTime  touchTime
  }
  \endverbatim
  
  A TouchSensor node tracks the location and state of the pointing
  device and detects when the user points at geometry contained by the
  TouchSensor node's parent group. A TouchSensor node can be enabled
  or disabled by sending it an enabled event with a value of TRUE or
  FALSE. If the TouchSensor node is disabled, it does not track user
  input or send events.

  The TouchSensor generates events when the pointing device points toward
  any geometry nodes that are descendants of the TouchSensor's parent group.
  See 4.6.7.5, Activating and manipulating sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.5>),
  for more details on using the pointing device to activate the TouchSensor.

  The \e isOver eventOut reflects the state of the pointing device
  with regard to whether it is pointing towards the TouchSensor node's
  geometry or not. When the pointing device changes state from a
  position such that its bearing does not intersect any of the
  TouchSensor node's geometry to one in which it does intersect
  geometry, an isOver TRUE event is generated. When the pointing
  device moves from a position such that its bearing intersects
  geometry to one in which it no longer intersects the geometry, or
  some other geometry is obstructing the TouchSensor node's geometry,
  an isOver FALSE event is generated. These events are generated only
  when the pointing device has moved and changed 'over' state. Events
  are not generated if the geometry itself is animating and moving
  underneath the pointing device.

  As the user moves the bearing over the TouchSensor node's geometry,
  the point of intersection (if any) between the bearing and the
  geometry is determined.  Each movement of the pointing device, while
  isOver is TRUE, generates hitPoint_changed, hitNormal_changed and
  hitTexCoord_changed events. hitPoint_changed events contain the 3D
  point on the surface of the underlying geometry, given in the
  TouchSensor node's coordinate system.  hitNormal_changed events
  contain the surface normal vector at the
  hitPoint. hitTexCoord_changed events contain the texture coordinates
  of that surface at the hitPoint. The values of hitTexCoord_changed
  and hitNormal_changed events are computed as appropriate for the
  associated shape.

  If isOver is TRUE, the user may activate the pointing device to
  cause the TouchSensor node to generate isActive events (e.g., by
  pressing the primary mouse button). When the TouchSensor node
  generates an isActive TRUE event, it grabs all further motion events
  from the pointing device until it is released and generates an
  isActive FALSE event (other pointing-device sensors will not
  generate events during this time). Motion of the pointing device
  while isActive is TRUE is termed a "drag." If a 2D pointing device
  is in use, isActive events reflect the state of the primary button
  associated with the device (i.e., isActive is TRUE when the primary
  button is pressed and FALSE when it is released).  If a 3D pointing
  device is in use, isActive events will typically reflect whether the
  pointing device is within (or in contact with) the TouchSensor
  node's geometry.

  The eventOut field touchTime is generated when all three of the
  following conditions are true:

  - The pointing device was pointing towards the geometry when it
    was initially activated (isActive is TRUE).
 
  - The pointing device is currently pointing towards the geometry (isOver is TRUE).
 
  - The pointing device is deactivated (isActive FALSE event
    is also generated).

  More information about this behaviour is described in 4.6.7.3,
  Pointing-device sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.3>),
  4.6.7.4, Drag sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.4>),
  and 4.6.7.5, Activating and manipulating sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.5>).

  \ENDWEB3D

  Here is a small example which demonstrates use of the
  SoVRMLTouchSensor. Click on the SoVRMLSphere to turn the
  SoVRMLPointLight on:

  \code
  #VRML V2.0 utf8
  
  Group {
     children [
        Transform {
           children [
              DEF light PointLight {
                 intensity 1
                 on FALSE
              }
  
              Transform {
                 translation -2 0 -2
                 children [
                    Shape {
                       appearance Appearance {
                          material Material {
                             diffuseColor 1 0 1
                             specularColor 1 1 1
                             shininess 0.9
                          }
                       }
                       geometry Sphere { }
                    }
                    DEF touchsensor TouchSensor { }
                 ]
              }
           ]
        }
     ]
     ROUTE touchsensor.isActive TO light.set_on
  }
  \endcode
*/

/*!
  \var SoSFBool SoVRMLTouchSensor::enabled
  TRUE is enabled. Default value is TRUE.
*/

/*!
  \var SoSFVec3f SoVRMLTouchSensor::hitNormal_changed

  An eventOut that is sent when the pointer is over some child
  geometry. Contains the object space normal.

*/

/*!
  \var SoSFVec3f SoVRMLTouchSensor::hitPoint_changed

  An eventOut that is sent when the pointer is over some child
  geometry. Contains the object space point.

*/

/*!
  \var SoSFVec2f SoVRMLTouchSensor::hitTexCoord_changed

  An eventOut that is sent when the pointer is over some child
  geometry. Contains the object space texture coordinates.

*/

/*!
  \var SoSFBool SoVRMLTouchSensor::isActive

  An event out that is sent when the active state changes. The sensor
  goes active when the user clicks on some child, and is inactive
  again when the mouse button is release.

*/

/*!
  \var SoSFBool SoVRMLTouchSensor::isOver

  An event out that is sent when the pointer is moved onto or away from
  a child object.

*/

/*!
  \var SoSFTime SoVRMLTouchSensor::touchTime

  An event out that is generated when the sensor is active and is
  currently pointing on some child geometry.
*/



#include <Inventor/VRMLnodes/SoVRMLTouchSensor.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoDB.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/SbMatrix.h>

#include "nodes/SoSubNodeP.h"

//
// returns the current time. First tries the realTime field, then
// SbTime::getTimeOfDay() if field is not found.
//
static SbTime
touchsensor_get_current_time(void)
{
  SoField * realtime = SoDB::getGlobalField("realTime");
  if (realtime && realtime->isOfType(SoSFTime::getClassTypeId())) {
    return ((SoSFTime*)realtime)->getValue();
  }
  return SbTime::getTimeOfDay();
}

SO_NODE_SOURCE(SoVRMLTouchSensor);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLTouchSensor::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLTouchSensor, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLTouchSensor::SoVRMLTouchSensor(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLTouchSensor);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(enabled, (TRUE));

  SO_VRMLNODE_ADD_EVENT_OUT(hitNormal_changed);
  SO_VRMLNODE_ADD_EVENT_OUT(hitPoint_changed);
  SO_VRMLNODE_ADD_EVENT_OUT(hitTexCoord_changed);
  SO_VRMLNODE_ADD_EVENT_OUT(isActive);
  SO_VRMLNODE_ADD_EVENT_OUT(isOver);
  SO_VRMLNODE_ADD_EVENT_OUT(touchTime);

  this->isactive = FALSE;
}

/*!
  Destructor.
*/
SoVRMLTouchSensor::~SoVRMLTouchSensor()
{
}

// Doc in parent
SbBool
SoVRMLTouchSensor::affectsState(void) const // virtual
{
  return TRUE;
}

// Doc in parent
void
SoVRMLTouchSensor::handleEvent(SoHandleEventAction * action)
{
  const SoEvent * event = action->getEvent();

  SbBool buttondown = SO_MOUSE_PRESS_EVENT(event, BUTTON1);
  SbBool buttonup = SO_MOUSE_RELEASE_EVENT(event, BUTTON1);
  SbBool mousemove = event->isOfType(SoLocation2Event::getClassTypeId());

  SbBool wasover = this->isOver.getValue();

  if (mousemove || buttondown || buttonup) {
    SbBool isover = FALSE;
    const SoPickedPoint * pp = action->getPickedPoint();
    SoNode * parentnode = NULL;
    if (pp) {
      const SoFullPath * currpath = (const SoFullPath*) action->getCurPath();
      SoFullPath * parentpath = (SoFullPath*) currpath->copy(0, currpath->getLength()-1);
      parentnode = parentpath->getTail();
      parentpath->ref();
      isover = pp->getPath()->containsPath(parentpath);
      parentpath->unref();
    }
    SbBool active = this->isactive;
    if (active && buttonup) {
      this->isActive.setValue(FALSE);
      this->isactive = FALSE;
    }
    if (!active && buttondown && isover) {
      this->isActive.setValue(TRUE);
      this->isactive = TRUE;
      active = TRUE;
    }
    if (wasover != isover) {
      this->isOver.setValue(isover);
    }
    if (isover) {
      if (active) {
          this->touchTime.setValue(touchsensor_get_current_time());
      }
      SbMatrix transform = pp->getWorldToObject(parentnode);
      SbVec3f normal = pp->getNormal();
      transform.multDirMatrix(normal, normal);
      this->hitNormal_changed = normal;
      SbVec3f pt = pp->getPoint();
      transform.multVecMatrix(pt, pt);
      this->hitPoint_changed = pt;

      transform = pp->getImageToObject(parentnode);
      SbVec4f tc = pp->getTextureCoords();
      transform.multVecMatrix(tc, tc);
      float div = tc[3] ? 1.0f / tc[3] : 1.0f;
      SbVec2f tc2(tc[0] * div, tc[1] * div);
      this->hitTexCoord_changed = tc2;
    }
  }
  inherited::handleEvent(action);
}

// Doc in parent
void
SoVRMLTouchSensor::notify(SoNotList * list)
{
  inherited::notify(list);
}

#endif // HAVE_VRML97
