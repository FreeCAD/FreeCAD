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
  \class SoRotationXYZ SoRotationXYZ.h Inventor/nodes/SoRotationXYZ.h
  \brief The SoRotationXYZ class is a node type for specifying rotation around a particular axis.

  \ingroup coin_nodes

  Application programmers can use nodes of this type instead of
  SoRotation nodes for simplicity and clarity if the rotation will
  only happen around one particular axis.

  Using SoRotationXYZ nodes are also simpler and more efficient than
  using SoRotation nodes if you are connecting engines to rotation
  angles for animation purposes.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    RotationXYZ {
        angle 0
        axis X
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoRotationXYZ.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoRotationXYZ::Axis
  Enumerated values for the 3 axes.
*/
/*!
  \var SoRotationXYZ::Axis SoRotationXYZ::X
  Rotation around X-axis.
*/
/*!
  \var SoRotationXYZ::Axis SoRotationXYZ::Y
  Rotation around Y-axis.
*/
/*!
  \var SoRotationXYZ::Axis SoRotationXYZ::Z
  Rotation around Z-axis.
*/

/*!
  \var SoSFEnum SoRotationXYZ::axis
  Which axis to rotate around. Defaults to SoRotationXYZ::X.
*/
/*!
  \var SoSFFloat SoRotationXYZ::angle
  The angle to rotate, specified in radians.
*/


// *************************************************************************

SO_NODE_SOURCE(SoRotationXYZ);

/*!
  Constructor.
*/
SoRotationXYZ::SoRotationXYZ(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoRotationXYZ);

  SO_NODE_ADD_FIELD(angle, (0.0f));
  SO_NODE_ADD_FIELD(axis, (SoRotationXYZ::X));

  SO_NODE_DEFINE_ENUM_VALUE(Axis, X);
  SO_NODE_DEFINE_ENUM_VALUE(Axis, Y);
  SO_NODE_DEFINE_ENUM_VALUE(Axis, Z);
  SO_NODE_SET_SF_ENUM_TYPE(axis, Axis);
}

/*!
  Destructor.
*/
SoRotationXYZ::~SoRotationXYZ()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoRotationXYZ::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoRotationXYZ, SO_FROM_INVENTOR_1);
}

// Doc from superclass.
void
SoRotationXYZ::doAction(SoAction * action)
{
  SbVec3f rotvec;
  if (this->getVector(rotvec)) {
    SoModelMatrixElement::rotateBy(action->getState(), this,
                                   SbRotation(rotvec, angle.getValue()));
  }
}

// Doc from superclass.
void
SoRotationXYZ::callback(SoCallbackAction * action)
{
  SoRotationXYZ::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoRotationXYZ::GLRender(SoGLRenderAction * action)
{
  SoRotationXYZ::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoRotationXYZ::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoRotationXYZ::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoRotationXYZ::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix m;

  SbRotation r = this->getRotation();
  r.getValue(m);
  action->getMatrix().multLeft(m);

  SbRotation ri = r.inverse();
  ri.getValue(m);
  action->getInverse().multRight(m);
}

// Doc from superclass.
void
SoRotationXYZ::pick(SoPickAction * action)
{
  SoRotationXYZ::doAction((SoAction *)action);
}

// Doc from superclass.
SbBool
SoRotationXYZ::getVector(SbVec3f & rotvec) const
{
  assert((int)axis.getValue() >= 0 && (int)axis.getValue() <= 2);

  rotvec.setValue(0.0f, 0.0f, 0.0f);
  rotvec[(int)axis.getValue()] = 1.0f;
  return TRUE;
}

/*!
  Returns an SbRotation object with values set up to correspond with
  the specified rotation of the node.
 */
SbRotation
SoRotationXYZ::getRotation(void) const
{
  SbVec3f theaxis;
  this->getVector(theaxis);
  return SbRotation(theaxis, this->angle.getValue());
}

// Doc from superclass. Overrides the traversal method in this class for
// the SoGetPrimitiveCountAction because the number of primitives can
// be different depending on scene location (and thereby distance to
// camera) if there are e.g. SoLOD nodes in the scene.
void
SoRotationXYZ::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoRotationXYZ::doAction((SoAction *)action);
}
