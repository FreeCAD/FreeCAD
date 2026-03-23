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
  \class SoGeoCoordinate SoGeoCoordinate.h Inventor/nodes/SoGeoCoordinate.h
  \brief The SoGeoCoordinate class is used to specify a list of geographical coordinates.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    GeoCoordinate {
      geoSystem ["GD", "WE"]
      point ""
    }
  \endcode

  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/nodes/SoGeoCoordinate.h>

#include <Inventor/nodes/SoGeoOrigin.h>
#include <Inventor/misc/SoGeo.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGeoElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFString SoGeoCoordinate::point

  \sa SoGeoOrigin::geoSystem
*/

/*!
  \var SoMFString SoGeoCoordinate::geoSystem

  \sa SoGeoOrigin::geoSystem
*/


// *************************************************************************

class SoGeoCoordinateP {
public:
  SbUniqueId originid;
  SbUniqueId thisid;
  SbList <SbVec3f> coords;
};

#define PRIVATE(obj) obj->pimpl

// *************************************************************************

SO_NODE_SOURCE(SoGeoCoordinate);

/*!
  Constructor.
*/
SoGeoCoordinate::SoGeoCoordinate(void)
{
  PRIVATE(this)->originid = 0;
  PRIVATE(this)->thisid = 0;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoGeoCoordinate);

  SO_NODE_ADD_FIELD(point, (0.0, 0.0, 0.0));
  SO_NODE_ADD_FIELD(geoSystem, (""));

  this->geoSystem.setNum(2);
  this->geoSystem.set1Value(0, "GD");
  this->geoSystem.set1Value(1, "WE");
  this->geoSystem.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoGeoCoordinate::~SoGeoCoordinate(void)
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoGeoCoordinate::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGeoCoordinate, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc from superclass.
void
SoGeoCoordinate::doAction(SoAction * action)
{
  SoState * state = action->getState();
  SoGeoOrigin * origin = SoGeoElement::get(state);

  if (!origin) {
    SoDebugError::post("SoGeoCoordinate::doAction",
                       "No SoGeoOrigin node found on stack.");
    return;
  }

  if (origin->getNodeId() != PRIVATE(this)->originid ||
      this->getNodeId() != PRIVATE(this)->thisid) {

    if (PRIVATE(this)->originid != origin->getNodeId()) {
      this->touch(); // to invalidate caches that depends on this coordinate node
    }
    PRIVATE(this)->originid = origin->getNodeId();
    PRIVATE(this)->thisid = this->getNodeId();

    PRIVATE(this)->coords.truncate(0);
    const int n = this->point.getNum();

    for (int i = 0; i < n; i++) {
      SbMatrix m = this->getTransform(origin, i);
      PRIVATE(this)->coords.append(SbVec3f(m[3]));
    }
  }

  SoCoordinateElement::set3(state, this, PRIVATE(this)->coords.getLength(),
                            PRIVATE(this)->coords.getArrayPtr());
}

// Doc from superclass.
void
SoGeoCoordinate::GLRender(SoGLRenderAction * action)
{
  SoGeoCoordinate::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoCoordinate::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoGeoCoordinate::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoCoordinate::callback(SoCallbackAction * action)
{
  SoGeoCoordinate::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoCoordinate::pick(SoPickAction * action)
{
  SoGeoCoordinate::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoCoordinate::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoGeoCoordinate::doAction((SoAction *)action);
}

SbMatrix
SoGeoCoordinate::getTransform(SoGeoOrigin * origin, const int idx) const
{
  return SoGeo::calculateTransform(origin->geoSystem.getValues(0),
                                   origin->geoSystem.getNum(),
                                   origin->geoCoords.getValue(),

                                   this->geoSystem.getValues(0),
                                   this->geoSystem.getNum(),
                                   this->point[idx]);
}

#undef PRIVATE

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoGeoCoordinate::getClassTypeId() != SoType::badType(),
                      "SoGeoCoordinate class not initialized");
  SoRefPtr<SoGeoCoordinate> node(new SoGeoCoordinate);
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(node->point.getNum(), 1);
}

#endif // COIN_TEST_SUITE
