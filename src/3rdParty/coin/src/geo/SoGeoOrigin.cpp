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
  \class SoGeoOrigin SoGeoOrigin.h Inventor/nodes/SoGeoOrigin.h
  \brief The SoGeoOrigin class is used to specify an absolute geographic location against which geometry is referenced.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    GeoOrigin {
      geoSystem ["GD", "WE"]
      geoCoords 0 0 0
    }
  \endcode

  A common problem when dealing with geographic data is the reduced
  floating point precision you often get. UTM coordinates are often in
  the 10^5 a 10^6 magnitude, and this leaves very little precision for
  details at that position.

  The SoGeo nodes are therefore useful when you want to keep your data
  in its original system, but still get good floating point precision
  when rendering.

  Coin needs a local Cartesian coordinate system when rendering. When
  a SoGeoOrigin node is used, Coin will create a coordinate system at
  the SoGeoOrigin position, and all geometry (and the camera) in the
  scene graph will be projected into that coordinate system.

  The coordinate system will always have the Z-axis point up from the
  ground. The Y-axis will point towards the north pole, and the X-axis
  is found using the right hand rule. 

  A scene graph should only contain one GeoOrigin node, and all
  geometry in the scene graph will, as stated earlier, be rendered
  relative to this position. This means that the precision will be
  best if the GeoOrigin position is as close to actual camera position
  as possible. If you move around on a large area, it might therefore
  be a good idea too actually move the GeoOrigin position instead of
  the camera.

  To place geometry in the scene graph, you can either use an
  SoGeoSeparator node or an SoGeoCoordinate node. When using a
  GeoSeparator node, all geometry inside that separator will be
  rendered relative to its geo-system position and orientation, and
  you then use regular shapes and regular SoCoordinate3 nodes to
  specify data (the points in an SoCoordinate3 must be adjusted to be
  relative to the GeoSeparator position).

  The SoGeoCoordinate node on the other hand can contain double
  precision geo-coordinates, and that node will internally recalculate
  the double precision array to a single precision array which is
  relative to the SoGeoOrgin node.

  One note regarding UTM projections: Since it is quite common to assume
  a flat earth when working with UTM data, it is possible to supply a 
  "FLAT" keyword for UTM coordinate systems:

  \code

  GeoOrigin {
    geoSystem [ "UTM", "Z17", "FLAT" ]
    geoCoords  846889 4313850 0
  }

  \endcode

  Example scene graph:
  
  \code
  
  GeoOrigin { geoSystem "GD" geoCoords 40.77 -73.97 0 }

  GeoSeparator {
    # New York, NY
    geoSystem  "GD"
    geoCoords 40.67 -73.94 0

    BaseColor { rgb 0 1 0 }
    Cube { width 25000 height 25000 depth 25000 }
    Translation { translation 0 0 30000 }
    Text2 { string "New York" }
  }

  GeoSeparator {
    # Los Angeles, CA
    geoSystem "GD"
    geoCoords 34.11 -118.4 0

    BaseColor { rgb 1 0 0 }
    Cube { width 25000 height 25000 depth 25000 }
    Translation { translation 0 0 30000 }
    Text2 { string "Los Angeles" }
  }

  GeoSeparator {
    # Washington, DC
    geoSystem [ "UTM", "Z17" ]
    geoCoords  846889 4313850 0

    BaseColor { rgb 0 1 1 }
    Cube { width 25000 height 25000 depth 25000 }

    Translation { translation 0 0 30000 }
    Text2 { string "Washington" }    
  }

  # add a small geogrid
  GeoCoordinate {
    geoSystem "GD"
    point [
    32 -120 0,
    32 -110 0,
    32 -100 0,
    32 -90 0,
    32 -80 0,
    32 -70 0,

    34 -120 0,
    34 -110 0,
    34 -100 0,
    34 -90 0,
    34 -80 0,
    34 -70 0,

    36 -120 0,
    36 -110 0,
    36 -100 0,
    36 -90 0,
    36 -80 0,
    36 -70 0,

    38 -120 0,
    38 -110 0,
    38 -100 0,
    38 -90 0,
    38 -80 0,
    38 -70 0,

    40 -120 0,
    40 -110 0,
    40 -100 0,
    40 -90 0,
    40 -80 0,
    40 -70 0

    42 -120 0,
    42 -110 0,
    42 -100 0,
    42 -90 0,
    42 -80 0,
    42 -70 0
    ]
  }
  
  DrawStyle { style LINES }
  BaseColor {}
  ShapeHints { vertexOrdering COUNTERCLOCKWISE }
  QuadMesh { verticesPerRow 6 verticesPerColumn 6 }

  \endcode

  \since Coin 2.5  
*/

// *************************************************************************

#include <Inventor/nodes/SoGeoOrigin.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGeoElement.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3d SoGeoOrigin::geoCoords

  Used for specifying the geographic coordinates. For the GD system this should
  be &lt;latitude&gt; &lt;longitude&gt; &lt;elevation&gt;. For UTM it is &lt;easting&gt; &lt;northing&gt; &lt;elevation&gt;,
  and for GC it is simply &lt;x&gt; &lt;y&gt; &lt;z&gt;.

*/

/*!
  \var SoMFString SoGeoOrigin::geoSystem

  Used to specify a spatial reference frame. Coin currently supports three different
  systems. Support for more systems might be added in the future.

  \li "GD" - The Geodetic system (latitude/longitude).  

  \li "UTM" - Universal Transverse Mercator coordinate system. The
  second string should be the zone, encoded as "Z<n>".
  
  \li "GC" - Earth-fixed Geocentric with respect to the WGS84 ellipsoid.

  The "GD" and "UTM" systems can, for future support, have an ellipsoid
  specification. The default is "WE" which is the WGS84 ellipsoid, the only
  ellipsoid currently supported in Coin.

*/


// *************************************************************************

SO_NODE_SOURCE(SoGeoOrigin);

/*!
  Constructor.
*/
SoGeoOrigin::SoGeoOrigin(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGeoOrigin);

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
SoGeoOrigin::~SoGeoOrigin()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoGeoOrigin::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGeoOrigin, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGetBoundingBoxAction, SoGeoElement);
  SO_ENABLE(SoGLRenderAction, SoGeoElement);
  SO_ENABLE(SoGetMatrixAction, SoGeoElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoGeoElement);
  SO_ENABLE(SoPickAction, SoGeoElement);
  SO_ENABLE(SoCallbackAction, SoGeoElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoGeoElement);
  SO_ENABLE(SoAudioRenderAction, SoGeoElement);
}

// Doc from superclass.
void
SoGeoOrigin::doAction(SoAction * action)
{
  SoGeoElement::set(action->getState(), this);
}

// Doc from superclass.
void
SoGeoOrigin::GLRender(SoGLRenderAction * action)
{
  SoGeoOrigin::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoOrigin::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoGeoOrigin::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoOrigin::getMatrix(SoGetMatrixAction * action)
{
  SoGeoOrigin::doAction((SoAction*) action);
}

// Doc from superclass.
void
SoGeoOrigin::callback(SoCallbackAction * action)
{
  SoGeoOrigin::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoOrigin::pick(SoPickAction * action)
{
  SoGeoOrigin::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoOrigin::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoGeoOrigin::doAction((SoAction *)action);
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoGeoOrigin * node = new SoGeoOrigin;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE
