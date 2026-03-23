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
  \class SoGeoSeparator SoGeoSeparator.h Inventor/nodes/SoGeoSeparator.h
  \brief The SoGeoSeparator class is used to georeference a scene graph.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    GeoSeparator {
      geoSystem ["GD", "WE"]
      geoCoords 0 0 0
    }
  \endcode

  This node specifies an absolute geographic coordinate system for the
  children. When rendering (or applying other actions), Coin will add
  a transformation which transforms the geometry into the SoGeoOrigin
  coordinate system. All objects will be rotated to make the local
  Z-axis point up from the ground (at the specified geo-location), the
  Y-axis will point towards the north pole, and the X-axis is found
  using the right hand rule.

  \sa SoGeoOrigin
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/nodes/SoGeoSeparator.h>

#include <Inventor/nodes/SoGeoOrigin.h>
#include <Inventor/elements/SoGeoElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGeo.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3d SoGeoSeparator::geoCoords

  Used for specifying the geographic coordinates.

  \sa SoGeoOrigin::geoCoords
*/

/*!
  \var SoMFString SoGeoSeparator::geoSystem

  Used to specify a spatial reference frame.

  \sa SoGeoOrigin::geoSystem
*/


// *************************************************************************

class SoGeoSeparatorP {
private:
  SoGeoSeparatorP(void) {
    assert(FALSE);
  }
};

SO_NODE_SOURCE(SoGeoSeparator);

/*!
  Constructor.
*/
SoGeoSeparator::SoGeoSeparator(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGeoSeparator);

  SO_NODE_ADD_FIELD(geoCoords, (0.0, 0.0, 0.0));
  SO_NODE_ADD_FIELD(geoSystem, (""));

  this->geoSystem.setNum(2);
  this->geoSystem.set1Value(0, "GD");
  this->geoSystem.set1Value(1, "WE");
  this->geoSystem.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoGeoSeparator::~SoGeoSeparator(void)
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoGeoSeparator::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGeoSeparator, SO_FROM_INVENTOR_1|SoNode::VRML1);
  SoRayPickAction::addMethod(SoGeoSeparator::getClassTypeId(), SoNode::rayPickS);
}

// Doc from superclass.
void
SoGeoSeparator::applyTransformation(SoAction * action)
{
  SoState * state = action->getState();
  SbMatrix m = this->getTransform(state);

  SoModelMatrixElement::set(state, this, m);
}

// Doc from superclass.
void
SoGeoSeparator::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyTransformation((SoAction*) action);
  SoSeparator::GLRenderBelowPath(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::GLRenderInPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyTransformation((SoAction*) action);
  SoSeparator::GLRenderInPath(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();
  state->push();
  SbMatrix m = this->getTransform(state);

  SoModelMatrixElement::mult(state,
                             this,
                             SoModelMatrixElement::get(state).inverse());
  SoModelMatrixElement::mult(state,
                             this,
                             m);

  SoSeparator::getBoundingBox(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix m = this->getTransform(action->getState());
  action->getMatrix() = m;
  action->getInverse() = m.inverse();
}

// Doc from superclass.
void
SoGeoSeparator::callback(SoCallbackAction * action)
{
  SoState * state = action->getState();
  state->push();

  this->applyTransformation((SoAction *)action);
  SoSeparator::callback(action);

  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::rayPick(SoRayPickAction * action)
{
  SoState * state = action->getState();
  state->push();
  
  this->applyTransformation((SoAction *)action);
  SoSeparator::rayPick(action);

  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoState * state = action->getState();
  state->push();

  this->applyTransformation((SoAction *)action);
  SoSeparator::getPrimitiveCount(action);

  state->pop();
}

// *************************************************************************

SbMatrix
SoGeoSeparator::getTransform(SoState * state) const
{
  SoGeoOrigin * origin = SoGeoElement::get(state);

  if (origin) {
    return SoGeo::calculateTransform(origin->geoSystem.getValues(0),
                                     origin->geoSystem.getNum(),
                                     origin->geoCoords.getValue(),

                                     this->geoSystem.getValues(0),
                                     this->geoSystem.getNum(),
                                     this->geoCoords.getValue());
  }
  
  SoDebugError::post("SoGeoSeparator::getTransform",
                     "No SoGeoOrigin node found on stack.");
  return SbMatrix::identity();
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoGeoSeparator::getClassTypeId() != SoType::badType(),
                      "SoGeoSeparator class not initialized");
  SoRefPtr<SoGeoSeparator> node(new SoGeoSeparator);
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "SoGeoSeparator object wrongly initialized");
}

#endif // COIN_TEST_SUITE
