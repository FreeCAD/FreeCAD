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
  \class SoVRMLCylinderSensor SoVRMLCylinderSensor.h Inventor/VRMLnodes/SoVRMLCylinderSensor
  \brief The SoVRMLCylinderSensor class maps pointer motion into rotations around the Y-axis.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  CylinderSensor {
    exposedField SFBool     autoOffset TRUE
    exposedField SFFloat    diskAngle  0.262       # (0,/2)
    exposedField SFBool     enabled    TRUE
    exposedField SFFloat    maxAngle   -1          # [-2,2]
    exposedField SFFloat    minAngle   0           # [-2,2]
    exposedField SFFloat    offset     0           # (-inf, inf)
    eventOut     SFBool     isActive
    eventOut     SFRotation rotation_changed
    eventOut     SFVec3f    trackPoint_changed
  }
  \endverbatim

  The CylinderSensor node maps pointer motion (e.g., a mouse or wand)
  into a rotation on an invisible cylinder that is aligned with the
  Y-axis of the local coordinate system. The CylinderSensor uses the
  descendent geometry of its parent node to determine whether it is
  liable to generate events.

  The enabled exposed field enables and disables the CylinderSensor
  node. If TRUE, the sensor reacts appropriately to user events. If
  FALSE, the sensor does not track user input or send events. If
  enabled receives a FALSE event and isActive is TRUE, the sensor
  becomes disabled and deactivated, and outputs an isActive FALSE
  event. If enabled receives a TRUE event the sensor is enabled and
  ready for user activation.

  A CylinderSensor node generates events when the pointing device is
  activated while the pointer is indicating any descendent geometry
  nodes of the sensor's parent group. See 4.6.7.5, Activating and
  manipulating sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.5>),
  for more details on using the pointing device to activate the
  CylinderSensor.

  Upon activation of the pointing device while indicating the sensor's
  geometry, an isActive TRUE event is sent. The initial acute angle
  between the bearing vector and the local Y-axis of the
  CylinderSensor node determines whether the sides of the invisible
  cylinder or the caps (disks) are used for manipulation. If the
  initial angle is less than the diskAngle, the geometry is treated as
  an infinitely large disk lying in the local Y=0 plane and coincident
  with the initial intersection point. Dragging motion is mapped into
  a rotation around the local +Y-axis vector of the sensor's
  coordinate system. The perpendicular vector from the initial
  intersection point to the Y-axis defines zero rotation about the
  Y-axis. For each subsequent position of the bearing, a
  rotation_changed event is sent that equals the sum of the rotation
  about the +Y-axis vector (from the initial intersection to the new
  intersection) plus the offset value. trackPoint_changed events
  reflect the unclamped drag position on the surface of this disk.
  When the pointing device is deactivated and autoOffset is TRUE,
  offset is set to the last value of rotation_changed and an
  offset_changed event is generated. See 4.6.7.4, Drag sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.4>),
  for a more general description of autoOffset and offset fields.
 
  If the initial acute angle between the bearing vector and the local
  Y-axis of the CylinderSensor node is greater than or equal to
  diskAngle, then the sensor behaves like a cylinder. The shortest
  distance between the point of intersection (between the bearing and
  the sensor's geometry) and the Y-axis of the parent group's local
  coordinate system determines the radius of an invisible cylinder
  used to map pointing device motion and marks the zero rotation
  value. For each subsequent position of the bearing, a
  rotation_changed event is sent that equals the sum of the
  right-handed rotation from the original intersection about the
  +Y-axis vector plus the offset value. trackPoint_changed events
  reflect the unclamped drag position on the surface of the invisible
  cylinder. When the pointing device is deactivated and autoOffset is
  TRUE, offset is set to the last rotation angle and an offset_changed
  event is generated.  More details are available in 4.6.7.4, Drag
  sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.4>).

  When the sensor generates an isActive TRUE event, it grabs all
  further motion events from the pointing device until it is released
  and generates an isActive FALSE event (other pointing-device sensors
  shall not generate events during this time). Motion of the pointing
  device while isActive is TRUE is referred to as a "drag." If a 2D
  pointing device is in use, isActive events will typically reflect
  the state of the primary button associated with the device (i.e.,
  isActive is TRUE when the primary button is pressed and FALSE when
  it is released).  If a 3D pointing device (e.g., a wand) is in use,
  isActive events will typically reflect whether the pointer is within
  or in contact with the sensor's geometry. 
  
  While the pointing device is activated, trackPoint_changed and
  rotation_changed events are output and are interpreted from pointing
  device motion based on the sensor's local coordinate system at the
  time of activation. trackPoint_changed events represent the
  unclamped intersection points on the surface of the invisible
  cylinder or disk. If the initial angle results in cylinder rotation
  (as opposed to disk behaviour) and if the pointing device is dragged
  off the cylinder while activated, browsers may interpret this in a
  variety of ways (e.g., clamp all values to the cylinder and
  continuing to rotate as the point is dragged away from the
  cylinder). Each movement of the pointing device while isActive is
  TRUE generates trackPoint_changed and rotation_changed events.  The
  minAngle and maxAngle fields clamp rotation_changed events to a
  range of values. If minAngle is greater than maxAngle,
  rotation_changed events are not clamped. 

  The minAngle and maxAngle fields are restricted to the range [-2,
  2].  More information about this behaviour is described in 4.6.7.3,
  Pointing-device sensors, 4.6.7.4, Drag sensors
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.4>),
  and 4.6.7.5
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.7.5>),
  Activating and manipulating sensors.

*/

/*!
  \var SoSFFloat SoVRMLCylinderSensor::diskAngle
  Angle where sensor starts to behave like a disk.
*/

/*!
  \var SoSFFloat SoVRMLCylinderSensor::maxAngle
  Maximum angle around Y-axis.
*/

/*!
  \var SoSFFloat SoVRMLCylinderSensor::minAngle
  Minimum angle around Y-axis.
*/

/*!
  \var SoSFFloat SoVRMLCylinderSensor::offset
  Current rotation value. Initial value is 0.0.
*/

/*!
  \var SoSFRotation SoVRMLCylinderSensor::rotation_changed
  This eventOut is signaled during sensor interaction.
*/

#include <Inventor/VRMLnodes/SoVRMLCylinderSensor.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/projectors/SbCylinderPlaneProjector.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbCylinder.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLCylinderSensor);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLCylinderSensor::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLCylinderSensor, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLCylinderSensor::SoVRMLCylinderSensor(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLCylinderSensor);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(diskAngle, (0.262f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(maxAngle, (-1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(minAngle, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(offset, (0.0f));
  SO_VRMLNODE_ADD_EVENT_OUT(rotation_changed);

  this->cylinderproj = new SbCylinderPlaneProjector();
}

/*!
  Destructor.
*/
SoVRMLCylinderSensor::~SoVRMLCylinderSensor()
{
  delete this->cylinderproj;
}

// Doc in parent
SbBool
SoVRMLCylinderSensor::dragStart(void)
{
  // FIXME: heed the minAngle/maxAngle parameters.
  SbVec3f thehitpt = this->getLocalStartingPoint();
  SbLine line(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f));
  SbVec3f ptonline = line.getClosestPoint(thehitpt);
  if (ptonline != thehitpt) {
    this->cylinderproj->setCylinder(SbCylinder(line, (ptonline-thehitpt).length()));
    this->rotation_changed = SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), this->offset.getValue());
    return TRUE;
  }
  return FALSE;
}

// Doc in parent
void
SoVRMLCylinderSensor::drag(void)
{
  this->cylinderproj->setViewVolume(this->getViewVolume());
  this->cylinderproj->setWorkingSpace(this->getLocalToWorldMatrix());

  SbVec3f projpt = this->cylinderproj->project(this->getNormalizedLocaterPosition());
  SbVec3f startpt = this->getLocalStartingPoint();

  SbRotation rot = this->cylinderproj->getRotation(startpt, projpt);
  this->rotation_changed = rot * SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), this->offset.getValue());
}

// Doc in parent
void
SoVRMLCylinderSensor::dragFinish(void)
{
  if (this->autoOffset.getValue())
    this->offset = findAngle(this->rotation_changed.getValue());
}

//
// Private method that finds the angle around the Y-axis.
//
float
SoVRMLCylinderSensor::findAngle(const SbRotation & rot)
{
  SbVec3f axis;
  float angle;
  rot.getValue(axis, angle);
  if (axis[1] >= 0.0f) return angle;
  return -angle;
}

#endif // HAVE_VRML97
