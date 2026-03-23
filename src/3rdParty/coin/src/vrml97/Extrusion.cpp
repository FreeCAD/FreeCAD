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
  \class SoVRMLExtrusion SoVRMLExtrusion.h Inventor/VRMLnodes/SoVRMLExtrusion.h
  \brief The SoVRMLExtrusion class is a a geometry node for extruding a cross section along a spine.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Extrusion {
    eventIn MFVec2f    set_crossSection
    eventIn MFRotation set_orientation
    eventIn MFVec2f    set_scale
    eventIn MFVec3f    set_spine
    field   SFBool     beginCap         TRUE
    field   SFBool     ccw              TRUE
    field   SFBool     convex           TRUE
    field   SFFloat    creaseAngle      0                # [0,inf)
    field   MFVec2f    crossSection     [ 1 1, 1 -1, -1 -1, -1 1, 1  1 ]    # (-inf,inf)
    field   SFBool     endCap           TRUE
    field   MFRotation orientation      0 0 1 0          # [-1,1],(-inf,inf)
    field   MFVec2f    scale            1 1              # (0,inf)
    field   SFBool     solid            TRUE
    field   MFVec3f    spine            [ 0 0 0, 0 1 0 ] # (-inf,inf)
  }
  \endverbatim

  \e Introduction

  The Extrusion node specifies geometric shapes based on a two
  dimensional cross-section extruded along a three dimensional spine
  in the local coordinate system. The cross-section can be scaled and
  rotated at each spine point to produce a wide variety of shapes.  An
  Extrusion node is defined by:

  \li a 2D crossSection piecewise linear curve (described as a series
  of connected vertices);

  \li a 3D spine piecewise linear curve (also described as a series
  of connected vertices);

  \li a list of 2D scale parameters;

  \li a list of 3D orientation parameters.

  \e Algorithmic \e description

  Shapes are constructed as follows. The cross-section curve, which
  starts as a curve in the Y=0 plane, is first scaled about the origin
  by the first scale parameter (first value scales in X, second value
  scales in Z). It is then translated by the first spine point and
  oriented using the first orientation parameter (as explained
  later). The same procedure is followed to place a cross- section at
  the second spine point, using the second scale and orientation
  values. Corresponding vertices of the first and second
  cross-sections are then connected, forming a quadrilateral polygon
  between each pair of vertices. This same procedure is then repeated
  for the rest of the spine points, resulting in a surface extrusion
  along the spine.

  The final orientation of each cross-section is computed by first
  orienting it relative to the spine segments on either side of point
  at which the cross-section is placed. This is known as the
  spine-aligned cross-section plane (SCP), and is designed to provide
  a smooth transition from one spine segment to the next (see Figure
  6.6). The SCP is then rotated by the corresponding orientation
  value. This rotation is performed relative to the SCP. For example,
  to impart twist in the cross- section, a rotation about the Y-axis
  (0 1 0) would be used. Other orientations are valid and rotate the
  cross-section out of the SCP.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/Extrusion.gif">
  Figure 6.6
  </center>

  The SCP is computed by first computing its Y-axis and Z-axis, then
  taking the cross product of these to determine the X-axis. These
  three axes are then used to determine the rotation value needed to
  rotate the Y=0 plane to the SCP. This results in a plane that is the
  approximate tangent of the spine at each point, as shown in Figure
  6.6. First the Y-axis is determined, as follows:

  Let n be the number of spines and let i be the index variable
  satisfying 0 <= i < n:

  \li For all points other than the first or last: The Y-axis for
  spine[i] is found by normalizing the vector defined by (spine[i+1]
  - spine[i-1]).

  \li If the spine curve is closed: The SCP for the first and last
  points is the same and is found using (spine[1] - spine[n-2])
  to compute the Y-axis.

  \li If the spine curve is not closed: The Y-axis used for the
  first point is the vector from spine[0] to spine[1], and for the
  last it is the vector from spine[n-2] to spine[n-1].

  The Z-axis is determined as follows:

  \li For all points other than the first or last: Take the following
  cross-product:

  \verbatim
  Z = (spine[i+1] - spine[i]) × (spine[i-1] - spine[i])
  \endverbatim

  \li If the spine curve is closed: The SCP for the first and last
  points is the same and is found by taking the following cross- product:

  \verbatim
  Z = (spine[1] - spine[0]) × (spine[n-2] - spine[0])
  \endverbatim

  \li If the spine curve is not closed: The Z-axis used for the first
  spine point is the same as the Z-axis for spine[1]. The Z- axis used for
  the last spine point is the same as the Z-axis for spine[n-2].

  \li After determining the Z-axis, its dot product with the Z-axis of the
  previous spine point is computed. If this value is negative, the
  Z-axis is flipped (multiplied by -1). In most cases, this prevents
  small changes in the spine segment angles from flipping the
  cross-section 180 degrees.

  Once the Y- and Z-axes have been computed, the X-axis can be
  calculated as their cross-product.

  \e Special \e Cases

  If the number of scale or orientation values is greater than the
  number of spine points, the excess values are ignored. If they
  contain one value, it is applied at all spine points. The results
  are undefined if the number of scale or orientation values is
  greater than one but less than the number of spine points. The scale
  values shall be positive.

  If the three points used in computing the Z-axis are collinear, the
  cross-product is zero so the value from the previous point is used
  instead.  If the Z-axis of the first point is undefined (because the
  spine is not closed and the first two spine segments are collinear)
  then the Z-axis for the first spine point with a defined Z-axis is
  used.

  If the entire spine is collinear, the SCP is computed by finding the
  rotation of a vector along the positive Y-axis (v1) to the vector
  formed by the spine points (v2). The Y=0 plane is then rotated by
  this value.  If two points are coincident, they both have the same
  SCP. If each point has a different orientation value, then the
  surface is constructed by connecting edges of the cross-sections as
  normal. This is useful in creating revolved surfaces.

  Note: combining coincident and non-coincident spine segments, as
  well as other combinations, can lead to interpenetrating surfaces
  which the extrusion algorithm makes no attempt to avoid.

  \e Common \e Cases

  The following common cases are among the effects which are supported
  by the Extrusion node:

  \li Surfaces of revolution: If the cross-section is an approximation
  of a circle and the spine is straight, the Extrusion is equivalent
  to a surface of revolution, where the scale parameters define the
  size of the cross-section along the spine.

  \li Uniform extrusions: If the scale is (1, 1) and the spine is
  straight, the cross-section is extruded uniformly without twisting
  or scaling along the spine. The result is a cylindrical shape with a
  uniform cross section.

  \li Bend/twist/taper objects: These shapes are the result of using
  all fields. The spine curve bends the extruded shape defined by the
  cross-section, the orientation parameters (given as rotations about
  the Y-axis) twist it around the spine, and the scale parameters
  taper it (by scaling about the spine).

  \e Other \e Fields

  Extrusion has three parts: the sides, the beginCap (the
  surface at the initial end of the spine) and the endCap (the surface
  at the final end of the spine). The caps have an associated SFBool field
  that indicates whether each exists (TRUE) or doesn't exist (FALSE).

  When the beginCap or endCap fields are specified as TRUE, planar cap
  surfaces will be generated regardless of whether the crossSection is
  a closed curve. If crossSection is not a closed curve, the caps are
  generated by adding a final point to crossSection that is equal to
  the initial point. An open surface can still have a cap, resulting
  (for a simple case) in a shape analogous to a soda can sliced in
  half vertically.  These surfaces are generated even if spine is also
  a closed curve.  If a field value is FALSE, the corresponding cap is
  not generated.

  Texture coordinates are automatically generated by Extrusion
  nodes. Textures are mapped so that the coordinates range in the U
  direction from 0 to 1 along the crossSection curve (with 0
  corresponding to the first point in crossSection and 1 to the last)
  and in the V direction from 0 to 1 along the spine curve (with 0
  corresponding to the first listed spine point and 1 to the last). If
  either the endCap or beginCap exists, the crossSection curve is
  uniformly scaled and translated so that the larger dimension of the
  cross-section (X or Z) produces texture coordinates that range from
  0.0 to 1.0. The beginCap and endCap textures' S and T directions
  correspond to the X and Z directions in which the crossSection
  coordinates are defined.

  The browser shall automatically generate normals for the Extrusion
  node,using the creaseAngle field to determine if and how normals are
  smoothed across the surface. Normals for the caps are generated
  along the Y-axis of the SCP, with the ordering determined by viewing
  the cross-section from above (looking along the negative Y-axis of
  the SCP). By default, a beginCap with a counterclockwise ordering
  shall have a normal along the negative Y-axis. An endCap with a
  counterclockwise ordering shall have a normal along the positive
  Y-axis.

  Each quadrilateral making up the sides of the extrusion are ordered
  from the bottom cross-section (the one at the earlier spine point)
  to the top.  So, one quadrilateral has the points:

  \verbatim
  spine[0](crossSection[0], crossSection[1])
  spine[1](crossSection[1], crossSection[0])
  \endverbatim

  in that order. By default, normals for the sides are generated as
  described in 4.6.3, Shapes and geometry
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.3>).

  For instance, a circular crossSection with counterclockwise
  ordering and the default spine form a cylinder. With solid TRUE and
  ccw TRUE, the cylinder is visible from the outside. Changing ccw to
  FALSE makes it visible from the inside.  The ccw, solid, convex, and
  creaseAngle fields are described in 4.6.3, Shapes and geometry
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.3>).

*/

/*!
  SoSFBool SoVRMLExtrusion::beginCap
  Used to enable/disable begin cap. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLExtrusion::ccw
  Specifies counterclockwise vertex ordering. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLExtrusion::convex
  Specifies if cross sections is convex. Default value is TRUE.
*/

/*!
  SoSFFloat SoVRMLExtrusion::creaseAngle
  Specifies the crease angle for the generated normals. Default value is 0.0.
*/

/*!
  SoMFVec2f SoVRMLExtrusion::crossSection
  The cross section.
*/

/*!
  SoSFBool SoVRMLExtrusion::endCap
  Used to enable/disable end cap. Default value is TRUE.

*/

/*!
  SoMFRotation SoVRMLExtrusion::orientation
  Orientation for the cross section at each spine point.
*/

/*!
  SoMFVec2f SoVRMLExtrusion::scale
  Scaling for the cross section at each spine point.
*/

/*!
  SoSFBool SoVRMLExtrusion::solid
  When TRUE, backface culling will be enabled. Default value is TRUE.
*/

/*!
  SoMFVec3f SoVRMLExtrusion::spine
  The spine points.
*/

#include <Inventor/VRMLnodes/SoVRMLExtrusion.h>

#include <cfloat>
#include <cmath>
#include <cstring>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoVertexAttributeBundle.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/SbTesselator.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/SbBox2f.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbRWMutex.h>
#endif // HAVE_THREADS

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "rendering/SoGL.h"
#include "misc/SbHash.h"
#include "caches/SoVBOCache.h"

// *************************************************************************

//
// needed to avoid warnings generated by SbVec3f::normalize
//
static float
my_normalize(SbVec3f & vec)
{
  float len = vec.length();
  if (len > FLT_EPSILON) {
    vec /= len;
  }
  return len;
}

// set this to TRUE to create triangles, even if convex == TRUE just
// testing this feature. Will consider an environment variable or
// something later. pederb, 2005-01-25
static const SbBool ALWAYS_CREATE_TRIANGLES = FALSE;

class SoVRMLExtrusionVertex {
public:
  SbVec3f coord;
  SbVec3f normal;
  SbVec2f texcoord;

  // needed for SbHash
  operator unsigned long(void) const {
    unsigned long key = 0;
    // create an xor key based on coordinates, normal and texcoords
    const unsigned char * ptr = (const unsigned char *) this;
    const ptrdiff_t size = sizeof(SoVRMLExtrusionVertex);

    for (int i = 0; i < size; i++) {
      int shift = (i%4) * 8;
      key ^= (ptr[i]<<shift);
    }
    return key;
  }
  // needed, since if we don't add this the unsigned long operator
  // will be used when comparing two vertices.
  int operator==(const SoVRMLExtrusionVertex & v) {
    return
      (this->coord == v.coord) &&
      (this->normal == v.normal) &&
      (this->texcoord == v.texcoord);
  }
};

class SoVRMLExtrusionP {
public:

  SoVRMLExtrusionP(SoVRMLExtrusion * master)
    :master(master),
     coord(32),
     tcoord(32),
     idx(32),
     gen(TRUE),
     dirty(TRUE),
     vbocache(NULL)
#ifdef COIN_THREADSAFE
     , rwmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  {
    this->tess.setCallback(tess_callback, this);
  }
  ~SoVRMLExtrusionP() {
    if (this->vbocache) this->vbocache->unref();
  }

  SoVRMLExtrusion * master;
  SbList <SbVec3f> coord;
  SbList <SbVec2f> tcoord;
  SbList <int32_t> idx;
  SoNormalGenerator gen;
  SbTesselator tess;
  static void tess_callback(void *, void *, void *, void *);
  void generateCoords(void);
  void generateNormals(void);
  SbBool dirty;
  SoVBOCache * vbocache;

  SbHash<SoVRMLExtrusionVertex, int32_t> vbohash;

  SbList <SbVec3f> vbocoord;
  SbList <SbVec3f> vbonormal;
  SbList <SbVec2f> vbotexcoord;

  void updateVBO(SoAction * action);
  void generateVBO(SoAction * action, SoTextureCoordinateBundle & tb);

#ifdef COIN_THREADSAFE
  SbRWMutex rwmutex;
  void readLock(void) { this->rwmutex.readLock(); }
  void readUnlock(void) { this->rwmutex.readUnlock(); }
  void writeLock(void) { this->rwmutex.writeLock(); }
  void writeUnlock(void) { this->rwmutex.writeUnlock(); }
#else // !COIN_THREADSAFE
  void readLock(void) { }
  void readUnlock(void) { }
  void writeLock(void) { }
  void writeUnlock(void) { }
#endif // !COIN_THREADSAFE
};

#define PRIVATE(obj) (obj)->pimpl
#define PUBLIC(obj) obj->master

// *************************************************************************

SO_NODE_SOURCE(SoVRMLExtrusion);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLExtrusion::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLExtrusion, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLExtrusion::SoVRMLExtrusion(void)
{
  PRIVATE(this) = new SoVRMLExtrusionP(this);

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLExtrusion);

  SO_VRMLNODE_ADD_FIELD(beginCap, (TRUE));
  SO_VRMLNODE_ADD_FIELD(endCap, (TRUE));
  SO_VRMLNODE_ADD_FIELD(solid, (TRUE));
  SO_VRMLNODE_ADD_FIELD(ccw, (TRUE));
  SO_VRMLNODE_ADD_FIELD(convex, (TRUE));
  SO_VRMLNODE_ADD_FIELD(creaseAngle, (0.0f));

  SO_NODE_ADD_FIELD(crossSection, (0.0f, 0.0f));
  this->crossSection.setNum(5);
  SbVec2f * cs = this->crossSection.startEditing();
  cs[0] = SbVec2f(1.0f, 1.0f);
  cs[1] = SbVec2f(1.0f, -1.0f);
  cs[2] = SbVec2f(-1.0f, -1.0f);
  cs[3] = SbVec2f(-1.0f, 1.0f);
  cs[4] = SbVec2f(1.0f, 1.0f);
  this->crossSection.finishEditing();
  this->crossSection.setDefault(TRUE);

  SO_NODE_ADD_FIELD(orientation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(scale, (1.0f, 1.0f));

  SO_NODE_ADD_FIELD(spine, (0.0f, 0.0f, 0.0f));
  this->spine.setNum(2);
  this->spine.set1Value(1, 0.0f, 1.0f, 0.0f);
  this->spine.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoVRMLExtrusion::~SoVRMLExtrusion()
{
  delete PRIVATE(this);
}


// Doc in parent
void
SoVRMLExtrusion::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();
  state->push();

  this->setupShapeHints(state, this->ccw.getValue(), this->solid.getValue());

  PRIVATE(this)->readLock();

  this->updateCache();

  if ((SoMultiTextureCoordinateElement::getType(state) !=
       SoMultiTextureCoordinateElement::FUNCTION) &&
      (SoMultiTextureCoordinateElement::getType(state) !=
       SoMultiTextureCoordinateElement::TEXGEN)) {
    SoGLMultiTextureCoordinateElement::setTexGen(state, this, NULL);
    SoMultiTextureCoordinateElement::set2(state, this, PRIVATE(this)->tcoord.getLength(),
                                          PRIVATE(this)->tcoord.getArrayPtr());
  }
  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(contextid);
  SbBool vbo = SoVBO::shouldCreateVBO(state, contextid, PRIVATE(this)->coord.getLength());

  if (vbo) PRIVATE(this)->updateVBO(action);

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool doTextures = SoMultiTextureEnabledElement::get(state);

  if (vbo) {
    if (!SoGLDriverDatabase::isSupported(glue, SO_GL_VBO_IN_DISPLAYLIST)) {
      SoCacheElement::invalidate(state);
      SoGLCacheContextElement::shouldAutoCache(state,
                                               SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
    int i;
    int lastenabled = -1;
    const SbBool * enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);

    if (doTextures) {
      PRIVATE(this)->vbocache->getTexCoordVBO(0)->bindBuffer(contextid);
      cc_glglue_glTexCoordPointer(glue, 2, GL_FLOAT, 0, NULL);
      cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

      for (i = 1; i <= lastenabled; i++) {
        if (enabled[i]) {
          cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
          cc_glglue_glTexCoordPointer(glue, 2, GL_FLOAT, 0, NULL);
          cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
        }
      }
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
    }

    PRIVATE(this)->vbocache->getNormalVBO()->bindBuffer(contextid);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);

    PRIVATE(this)->vbocache->getCoordVBO()->bindBuffer(contextid);
    cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);

    SoGLVertexAttributeElement::getInstance(state)->enableVBO(action);

    PRIVATE(this)->vbocache->getVertexArrayIndexer()->render(state, TRUE, contextid);

    cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0); // Reset VBO binding
    cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);
    cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);

    SoGLVertexAttributeElement::getInstance(state)->disableVBO(action);

    if (doTextures) {
      for (i = 1; i <= lastenabled; i++) {
        if (enabled[i]) {
          cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
          cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
        }
      }
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
      cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
    }
  }
  else {
    const SbVec3f * normals = PRIVATE(this)->gen.getNormals();

    SoCoordinateElement::set3(state, this, PRIVATE(this)->coord.getLength(), PRIVATE(this)->coord.getArrayPtr());
    const SoCoordinateElement * coords = SoCoordinateElement::getInstance(state);

    if (doTextures) {
      int lastenabled = -1;
      const SbBool * enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
      for (int i = 1; i <= lastenabled; i++) {
        if (enabled[i] && (SoMultiTextureCoordinateElement::getType(state, i) !=
                           SoMultiTextureCoordinateElement::FUNCTION)) {
          SoMultiTextureCoordinateElement::set2(state, this, i,
                                                PRIVATE(this)->tcoord.getLength(),
                                                PRIVATE(this)->tcoord.getArrayPtr());
        }
      }
    }

    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    doTextures = tb.needCoordinates();

    SoVertexAttributeBundle vab(action, TRUE);
    SbBool doattribs = vab.doAttributes();

    SoVertexAttributeBindingElement::Binding attribbind =
      SoVertexAttributeBindingElement::get(state);

    if (!doattribs) {
      // for overall attribute binding we check for doattribs before
      // sending anything in SoGL::FaceSet::GLRender
      attribbind = SoVertexAttributeBindingElement::OVERALL;
    }

    sogl_render_faceset((SoGLCoordinateElement *) coords,
                        PRIVATE(this)->idx.getArrayPtr(),
                        PRIVATE(this)->idx.getLength(),
                        normals,
                        NULL,
                        &mb,
                        NULL,
                        &tb,
                        PRIVATE(this)->idx.getArrayPtr(),
                        &vab,
                        3, /* SoIndexedFaceSet::PER_VERTEX */
                        0,
                        (int) attribbind,
                        doTextures ? 1 : 0,
                        doattribs ? 1 : 0);

  }
  PRIVATE(this)->readUnlock();

  state->pop();

  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, PRIVATE(this)->idx.getLength() / 4,
                        vbo);
}

// Doc in parent
void
SoVRMLExtrusion::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  PRIVATE(this)->readLock();
  this->updateCache();
  action->addNumTriangles(PRIVATE(this)->idx.getLength() / 4);
  PRIVATE(this)->readUnlock();
}

// Doc in parent
void
SoVRMLExtrusion::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                             SbBox3f & box,
                             SbVec3f & center)
{
  PRIVATE(this)->readLock();

  this->updateCache();

  int num = PRIVATE(this)->coord.getLength();
  const SbVec3f * coords = PRIVATE(this)->coord.getArrayPtr();

  box.makeEmpty();
  while (num--) {
    box.extendBy(*coords++);
  }
  if (!box.isEmpty()) center = box.getCenter();
  PRIVATE(this)->readUnlock();
}

// Doc in parent
void
SoVRMLExtrusion::generatePrimitives(SoAction * action)
{
  PRIVATE(this)->readLock();
  this->updateCache();

  const SbVec3f * normals = PRIVATE(this)->gen.getNormals();
  const SbVec2f * tcoords = PRIVATE(this)->tcoord.getArrayPtr();
  const SbVec3f * coords = PRIVATE(this)->coord.getArrayPtr();
  const int32_t * iptr = PRIVATE(this)->idx.getArrayPtr();
  const int32_t * endptr = iptr + PRIVATE(this)->idx.getLength();

  SoState * state = action->getState();
  state->push();

  int lastenabled = -1;
  const SbBool * enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
  for (int i = 1; i <= lastenabled; i++) {
    if (enabled[i] && (SoMultiTextureCoordinateElement::getType(state, i) !=
                       SoMultiTextureCoordinateElement::FUNCTION)) {
      SoMultiTextureCoordinateElement::set2(state, this, i,
                                            PRIVATE(this)->tcoord.getLength(),
                                            PRIVATE(this)->tcoord.getArrayPtr());
    }
  }
  SoShapeHintsElement::set(state, this,
                           this->ccw.getValue() ?
                           SoShapeHintsElement::COUNTERCLOCKWISE :
                           SoShapeHintsElement::CLOCKWISE,
                           this->solid.getValue() ?
                           SoShapeHintsElement::SOLID :
                           SoShapeHintsElement::UNKNOWN_SHAPE_TYPE,
                           this->convex.getValue() ?
                           SoShapeHintsElement::CONVEX :
                           SoShapeHintsElement::UNKNOWN_FACE_TYPE);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  SbBool istexfunc = tb.isFunction();
  SoPrimitiveVertex vertex;

  this->beginShape(action, TRIANGLES);
  TriangleShape shapetype = LINES; // set it to some impossible value

  int idx;
  while (iptr < endptr) {

    // we generate either triangles or quads, so this test is safe
    SbBool isquad = iptr[3] >= 0;
    if (isquad && (shapetype != QUADS)) {
      if (shapetype == TRIANGLES) this->endShape();
      this->beginShape(action, QUADS);
      shapetype = QUADS;
    }
    if (!isquad && (shapetype != TRIANGLES)) {
      if (shapetype == QUADS) this->endShape();
      this->beginShape(action, TRIANGLES);
      shapetype = TRIANGLES;
    }
    idx = *iptr++;
    while (idx >= 0) {
      vertex.setNormal(*normals);
      vertex.setPoint(coords[idx]);
      if (istexfunc) {
        vertex.setTextureCoords(tb.get(coords[idx], *normals));
      }
      else {
        vertex.setTextureCoords(tcoords[idx]);
      }
      this->shapeVertex(&vertex);
      idx = *iptr++;
      normals++;
    }
  }
  if ((shapetype == TRIANGLES) || (shapetype == QUADS)) this->endShape();

  state->pop();
  PRIVATE(this)->readUnlock();
}

// private method that updates the coordinate and normal cache.
// cache must be read-locked when entering here!
void
SoVRMLExtrusion::updateCache(void)
{
  if (PRIVATE(this)->dirty) {
    PRIVATE(this)->readUnlock();
    PRIVATE(this)->writeLock();
    PRIVATE(this)->generateCoords();
    PRIVATE(this)->generateNormals();
    PRIVATE(this)->dirty = FALSE;
    PRIVATE(this)->writeUnlock();
    PRIVATE(this)->readLock();
  }
}

void
SoVRMLExtrusionP::updateVBO(SoAction * action)
{
  if (this->vbocache == NULL || !this->vbocache->isValid(action->getState())) {
    this->readUnlock();
    SoTextureCoordinateBundle tb(action, FALSE, FALSE);
    SbBool istexfunc = tb.isFunction();
    if (istexfunc) {
      // trigger a texture coordinate function callback to update (for
      // instance) bounding box caches in texture function nodes. It's
      // important that this is done before we writeLock() the node.
      (void) tb.get(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 0.0f, 1.0f));
    }
    this->writeLock();
    this->generateVBO(action, tb);
    this->writeUnlock();
    this->readLock();
  }
}

void
SoVRMLExtrusionP::generateVBO(SoAction * action, SoTextureCoordinateBundle & tb)
{
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);

  SoState * state = action->getState();

  state->push();

  if (this->vbocache) {
    this->vbocache->unref();
  }
  this->vbocache = new SoVBOCache(state);
  this->vbocache->ref();

  // set active cache to record cache dependencies
  SoCacheElement::set(state, this->vbocache);

  // create a dependency on the texture coordinate element
  (void) SoMultiTextureCoordinateElement::getType(state);

  SbBool istexfunc = tb.isFunction();

  const SbVec3f * normals = this->gen.getNormals();
  const SbVec2f * tcoords = this->tcoord.getArrayPtr();
  const SbVec3f * coords = this->coord.getArrayPtr();
  const int32_t * iptr = this->idx.getArrayPtr();
  const int32_t * endptr = iptr + this->idx.getLength();

  this->vbohash.clear();
  this->vbocoord.truncate(0);
  this->vbonormal.truncate(0);
  this->vbotexcoord.truncate(0);

  SoVRMLExtrusionVertex v;
  int32_t vidx[4];
  int curridx = 0;

  SoVertexArrayIndexer * vboindexer = this->vbocache->getVertexArrayIndexer(TRUE);

  while (iptr < endptr) {
    // we generate either triangles or quads, so this test is safe
    SbBool isquad = iptr[3] >= 0;

    for (int i = 0; i < (isquad ? 4 : 3); i++) {
      int idx = *iptr++;
      v.normal = *normals;
      if (istexfunc) {
        SbVec4f tmp = tb.get(coords[idx], *normals);
        v.texcoord = SbVec2f(tmp[0]/tmp[3], tmp[1]/tmp[3]);
      }
      else {
        v.texcoord = tcoords[idx];
      }
      v.coord = coords[idx];
      normals++;

      if (!this->vbohash.get(v, vidx[i])) {
        vidx[i] = curridx++;
        this->vbohash.put(v, vidx[i]);
        this->vbocoord.append(v.coord);
        this->vbonormal.append(v.normal);
        this->vbotexcoord.append(v.texcoord);
      }
    }
    iptr++;
    if (isquad) {
      vboindexer->addQuad(vidx[0], vidx[1], vidx[2], vidx[3]);
    }
    else {
      vboindexer->addTriangle(vidx[0], vidx[1], vidx[2]);
    }
  }
  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  this->vbohash.clear();
  vboindexer->close();

  this->vbocache->getCoordVBO()->setBufferData(this->vbocoord.getArrayPtr(),
                                               this->vbocoord.getLength()*sizeof(SbVec3f), 1);

  this->vbocache->getNormalVBO()->setBufferData(this->vbonormal.getArrayPtr(),
                                                this->vbonormal.getLength()*sizeof(SbVec3f), 1);
  this->vbocache->getTexCoordVBO(0)->setBufferData(this->vbotexcoord.getArrayPtr(),
                                                   this->vbotexcoord.getLength()*sizeof(SbVec2f), 1);
}


// Doc in parent
void
SoVRMLExtrusion::notify(SoNotList * list)
{
  if (PRIVATE(this)->vbocache) PRIVATE(this)->vbocache->invalidate();
  PRIVATE(this)->dirty = TRUE;
  inherited::notify(list);
}


// Doc in parent
SoDetail *
SoVRMLExtrusion::createTriangleDetail(SoRayPickAction * COIN_UNUSED_ARG(action),
                                      const SoPrimitiveVertex * COIN_UNUSED_ARG(v1),
                                      const SoPrimitiveVertex * COIN_UNUSED_ARG(v2),
                                      const SoPrimitiveVertex * COIN_UNUSED_ARG(v3),
                                      SoPickedPoint * COIN_UNUSED_ARG(pp))
{
  // no triangle detail for Extrusion
  return NULL;
}

static SbVec3f
calculate_y_axis(const SbVec3f * spine, const int i,
                 const int numspine, const SbBool closed)
{
  SbVec3f Y;
  if (closed) {
    if (i > 0) {
      if (i == numspine-1) {
        Y = spine[1] - spine[i-1];
      }
      else {
        Y = spine[i+1] - spine[i-1];
      }
    }
    else {
      // use numspine-2, since for closed spines, the last spine point == the first point
      Y = spine[1] - spine[numspine >= 2 ? numspine-2 : numspine-1];
    }
  }
  else {
    if (i == 0) Y = spine[1] - spine[0];
    else if (i == numspine-1) Y = spine[numspine-1] - spine[numspine-2];
    else Y = spine[i+1] - spine[i-1];
  }
  my_normalize(Y);
  return Y;
}

static SbVec3f
calculate_z_axis(const SbVec3f * spine, const int i,
                 const int numspine, const SbBool closed)
{
  SbVec3f z0, z1;

  if (closed) {
    if (i > 0) {
      if (i == numspine-1) {
        z0 = spine[1] - spine[i];
        z1 = spine[i-1] - spine[i];
      }
      else {
        z0 = spine[i+1] - spine[i];
        z1 = spine[i-1] - spine[i];
      }
    }
    else {
      z0 = spine[1] - spine[0];
      z1 = spine[numspine >= 2 ? numspine-2 : numspine-1] - spine[0];
    }
  }
  else {
    if (numspine == 2) return SbVec3f(0.0f, 0.0f, 0.0f);
    else if (i == 0) {
      z0 = spine[2] - spine[1];
      z1 = spine[0] - spine[1];
    }
    else if (i == numspine-1) {
      z0 = spine[numspine-1] - spine[numspine-2];
      z1 = spine[numspine-3] - spine[numspine-2];
    }
    else {
      z0 = spine[i+1] - spine[i];
      z1 = spine[i-1] - spine[i];
    }
  }

  my_normalize(z0);
  my_normalize(z1);

  // test if spine segments are collinear. If they are, the cross
  // product will not be reliable, and we should just use the previous
  // Z-axis instead.
  if (SbAbs(z0.dot(z1)) > 0.999f) {
    return SbVec3f(0.0f, 0.0f, 0.0f);
  }
  SbVec3f tmp = z0.cross(z1);
  if (my_normalize(tmp) == 0.0f) {
    return SbVec3f(0.0f, 0.0f, 0.0f);
  }
  return tmp;
}

//
// generates extruded coordinates
//
void
SoVRMLExtrusionP::generateCoords(void)
{
  this->coord.truncate(0);
  this->tcoord.truncate(0);
  this->idx.truncate(0);

  if (PUBLIC(this)->crossSection.getNum() == 0 ||
      PUBLIC(this)->spine.getNum() == 0) return;

  SbMatrix matrix = SbMatrix::identity();

  SbBox2f crossbox;
  crossbox.makeEmpty();

  int i, j, numcross;
  SbBool connected = FALSE;   // is cross section closed
  SbBool closed = FALSE;      // is spine closed
  numcross = PUBLIC(this)->crossSection.getNum();
  const SbVec2f * cross = PUBLIC(this)->crossSection.getValues(0);
  if (cross[0] == cross[numcross-1]) {
    connected = TRUE;
  }

  int numspine = PUBLIC(this)->spine.getNum();
  const SbVec3f * spine = PUBLIC(this)->spine.getValues(0);
  if (spine[0] == spine[numspine-1]) {
    closed = TRUE;
  }

  // calculate the length of the spine and cross section. Needed for
  // texture coordinates.
  float spinelen = 0.0f;
  float crosslen = 0.0f;

  for (i = 0; i < numspine-1; i++) {
    spinelen += (spine[i+1]-spine[i]).length();
  }
  if (spinelen == 0.0f) spinelen = 1.0f;

  for (i = 0; i < numcross-1; i++) {
    crosslen += (cross[i+1]-cross[i]).length();
  }
  if (crosslen == 0.0f) crosslen = 1.0f;

  SbVec3f prevY(0.0f, 0.0f, 0.0f);
  SbVec3f prevZ(0.0f, 0.0f, 0.0f);
  const SbVec3f empty(0.0f, 0.0f, 0.0f);

  SbBool colinear = FALSE;
  SbVec3f X, Y, Z;

  // find first non-collinear spine segments and calculate the first
  // valid Y- and Z-axis
  for (i = 0; i < numspine && (prevY == empty || prevZ == empty); i++) {
    if (prevY == empty) {
      Y = calculate_y_axis(spine, i, numspine, closed);
      if (Y != empty) prevY = Y;
    }
    if (prevZ == empty) {
      Z = calculate_z_axis(spine, i, numspine, closed);
      if (Z != empty) prevZ = Z;
    }
  }

  if (prevY == empty) prevY = SbVec3f(0.0f, 1.0f, 0.0f);
  if (prevZ == empty) { // all spine segments are colinear, calculate constant Z-axis
    prevZ = SbVec3f(0.0f, 0.0f, 1.0f);
    if (prevY != SbVec3f(0.0f, 1.0f, 0.0f)) {
      SbRotation rot(SbVec3f(0.0f, 1.0f, 0.0f), prevY);
      rot.multVec(prevZ, prevZ);
    }
    colinear = TRUE;
  }

  int numorient = PUBLIC(this)->orientation.getNum();
  const SbRotation * orient = PUBLIC(this)->orientation.getValues(0);

  int numscale = PUBLIC(this)->scale.getNum();
  const SbVec2f * scale = PUBLIC(this)->scale.getValues(0);

  // calculate cross section bbox
  for (j = 0; j < numcross; j++) {
    crossbox.extendBy(cross[j]);
  }

  float currentspinelen = 0.0f; // for texcoords

  // loop through all spines
  for (i = 0; i < numspine; i++) {
    if (colinear) {
      Y = prevY;
      Z = prevZ;
    }
    else {
      Y = calculate_y_axis(spine, i, numspine, closed);
      Z = calculate_z_axis(spine, i, numspine, closed);
      if (Y == empty) Y = prevY;
      if (Z == empty) Z = prevZ;
      if (Z.dot(prevZ) < 0) Z = -Z;
    }

    X = Y.cross(Z);
    my_normalize(X);

    prevY = Y;
    prevZ = Z;

    matrix[0][0] = X[0];
    matrix[0][1] = X[1];
    matrix[0][2] = X[2];
    matrix[0][3] = 0.0f;

    matrix[1][0] = Y[0];
    matrix[1][1] = Y[1];
    matrix[1][2] = Y[2];
    matrix[1][3] = 0.0f;

    matrix[2][0] = Z[0];
    matrix[2][1] = Z[1];
    matrix[2][2] = Z[2];
    matrix[2][3] = 0.0f;

    matrix[3][0] = spine[i][0];
    matrix[3][1] = spine[i][1];
    matrix[3][2] = spine[i][2];
    matrix[3][3] = 1.0f;

    if (numorient) {
      SbMatrix rmat;
      orient[SbMin(i, numorient-1)].getValue(rmat);
      matrix.multLeft(rmat);
    }

    if (numscale) {
      SbMatrix smat = SbMatrix::identity();
      SbVec2f s = scale[SbMin(i, numscale-1)];
      smat[0][0] = s[0];
      smat[2][2] = s[1];
      matrix.multLeft(smat);
    }

    float currentcrosslen = 0.0f; // for texcoords
    for (j = 0; j < numcross; j++) {
      SbVec3f c;
      SbVec2f tc;
      c[0] = cross[j][0];
      c[1] = 0.0f;
      c[2] = cross[j][1];

      matrix.multVecMatrix(c, c);
      this->coord.append(c);
      tc[0] = currentcrosslen / crosslen;
      tc[1] = currentspinelen / spinelen;
      this->tcoord.append(tc);

      if (j < numcross-1) {
        currentcrosslen += (cross[j+1]-cross[j]).length();
      }
    }
    if (i < numspine-1) {
      currentspinelen += (spine[i+1]-spine[i]).length();
    }
  }

#define ADD_POINT(i0, j0) \
  do { \
    this->idx.append((i0)*numcross+(j0)); \
  } while (0)

  // this macro makes the code below more readable
#define ADD_TRIANGLE(i0, j0, i1, j1, i2, j2) \
  do { \
    this->idx.append((i0)*numcross+(j0)); \
    this->idx.append((i2)*numcross+(j2)); \
    this->idx.append((i1)*numcross+(j1)); \
    this->idx.append(-1); \
  } while (0)

#define ADD_QUAD(i0, j0, i1, j1, i2, j2, i3, j3)   \
  do { \
    this->idx.append((i0)*numcross+(j0)); \
    this->idx.append((i3)*numcross+(j3)); \
    this->idx.append((i2)*numcross+(j2)); \
    this->idx.append((i1)*numcross+(j1)); \
    this->idx.append(-1); \
  } while (0)

  // create walls
  for (i = 0; i < numspine-1; i++) {
    for (j = 0; j < numcross-1; j++) {
      if (PUBLIC(this)->convex.getValue() && !ALWAYS_CREATE_TRIANGLES) {
        ADD_QUAD(i, j, i+1, j, i+1, j+1, i, j+1);
      }
      else {
        ADD_TRIANGLE(i, j, i+1, j, i+1, j+1);
        ADD_TRIANGLE(i, j, i+1, j+1, i, j+1);
      }
    }
  }

  SbVec2f crossboxsize = crossbox.getMax() - crossbox.getMin();

  // create beginCap polygon
  if (PUBLIC(this)->beginCap.getValue() && !closed) {
    // create texcoords
    for (i = 0; i < numcross; i++) {
      SbVec2f c = cross[i];
      c -= crossbox.getMin();
      c[0] /= crossboxsize[0];
      c[1] /= crossboxsize[1];
      this->tcoord.append(c);
    }
    // just duplicated begincap coords to simplify texture coordinate handling
    for (i = 0; i < numcross; i++) {
      this->coord.append(coord[i]);
    }

    if (PUBLIC(this)->convex.getValue()) {
      for (i = 1; i < (connected ? numcross-2 : numcross-1); i++) {
        ADD_TRIANGLE(numspine, 0, numspine, i, numspine, i+1);
      }
    }
    else {
      // let the tessellator create triangles
      this->tess.beginPolygon(FALSE);
      for (i = (connected ? numcross-2 : numcross-1); i >= 0; i--) {
        int theidx = numcross*numspine + i;
        SbVec3f tc;
        tc.setValue(cross[i][0],
                    cross[i][1],
                    0.0f);
        this->tess.addVertex(tc, (void*) ((uintptr_t) theidx));
      }
      this->tess.endPolygon();
    }
  }

  // create endCap polygon
  if (PUBLIC(this)->endCap.getValue() && !closed) {
    // just duplicate endcap coords to simplify texture coordinate handling
    for (i = 0; i < numcross; i++) {
      this->coord.append(coord[(numspine-1)*numcross+i]);
    }
    // create texcoords
    for (i = 0; i < numcross; i++) {
      SbVec2f c = cross[i];
      c -= crossbox.getMin();
      c[0] /= crossboxsize[0];
      c[1] /= crossboxsize[1];
      // the endCap texcoords should be flipped in the T dimension
      c[1] = 1.0f - c[1];
      this->tcoord.append(c);
    }

	int offset = PUBLIC(this)->beginCap.getValue() ? 1 : 0;

	if (PUBLIC(this)->convex.getValue()) {
      for (i = 1; i < (connected ? numcross-2 : numcross-1); i++) {
        ADD_TRIANGLE(numspine+offset, numcross-1,
                     numspine+offset, numcross-1-i,
                     numspine+offset, numcross-2-i);
      }
    }
    else {
      // let the tessellator create triangles
      this->tess.beginPolygon(FALSE);
      for (i = (connected ? numcross-2 : numcross-1); i >= 0; i--) {
        int theidx = (numspine+offset)*numcross + numcross - 1 - i;
        SbVec3f tc;
        tc.setValue(cross[(numcross-1)-i][0],
                    cross[(numcross-1)-i][1],
                    0.0f);
        this->tess.addVertex(tc, (void*) ((uintptr_t) theidx));
      }
      this->tess.endPolygon();
    }
  }
#undef ADD_TRIANGLE
#undef ADD_QUAD
#undef ADD_POINT
}

//
// generates per-verex normals for the extrusion.
//
void
SoVRMLExtrusionP::generateNormals(void)
{
  this->gen.reset(PUBLIC(this)->ccw.getValue());
  const SbVec3f * c = this->coord.getArrayPtr();
  const int32_t * iptr = this->idx.getArrayPtr();
  const int32_t * endptr = iptr + this->idx.getLength();

  while (iptr < endptr) {
    this->gen.beginPolygon();
    int32_t theidx = *iptr++;
    while (theidx >= 0) {
      this->gen.polygonVertex(c[theidx]);
      theidx = *iptr++;
    }
    this->gen.endPolygon();
  }
  this->gen.generate(PUBLIC(this)->creaseAngle.getValue());
}

//
// callback from the polygon tessellator
//
void
SoVRMLExtrusionP::tess_callback(void * v0, void * v1, void * v2, void * data)
{
  SoVRMLExtrusionP * thisp = (SoVRMLExtrusionP*) data;
  thisp->idx.append((int32_t)((uintptr_t)v0));
  thisp->idx.append((int32_t)((uintptr_t)v1));
  thisp->idx.append((int32_t)((uintptr_t)v2));
  thisp->idx.append(-1);
}

#undef PUBLIC
#undef PRIVATE

#endif // HAVE_VRML97
