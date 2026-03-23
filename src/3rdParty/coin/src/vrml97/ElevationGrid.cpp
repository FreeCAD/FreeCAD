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
  \class SoVRMLElevationGrid SoVRMLElevationGrid.h Inventor/VRMLnodes/SoVRMLElevationGrid.h
  \brief The SoVRMLElevationGrid class is used to represent elevation grids.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  ElevationGrid {
    eventIn      MFFloat  set_height
    exposedField SFNode   color             NULL
    exposedField SFNode   normal            NULL
    exposedField SFNode   texCoord          NULL
    field        MFFloat  height            []      # (-inf,inf)
    field        SFBool   ccw               TRUE
    field        SFBool   colorPerVertex    TRUE
    field        SFFloat  creaseAngle       0       # [0,inf]
    field        SFBool   normalPerVertex   TRUE
    field        SFBool   solid             TRUE
    field        SFInt32  xDimension        0       # [0,inf)
    field        SFFloat  xSpacing          1.0     # (0,inf)
    field        SFInt32  zDimension        0       # [0,inf)
    field        SFFloat  zSpacing          1.0     # (0,inf)
  }
  \endverbatim

  The ElevationGrid node specifies a uniform rectangular grid of
  varying height in the Y=0 plane of the local coordinate system. The
  geometry is described by a scalar array of height values that
  specify the height of a surface above each point of the grid.

  The xDimension and zDimension fields indicate the number of elements
  of the grid height array in the X and Z directions. Both xDimension
  and zDimension shall be greater than or equal to zero. If either the
  xDimension or the zDimension is less than two, the ElevationGrid
  contains no quadrilaterals. The vertex locations for the rectangles
  are defined by the height field and the xSpacing and zSpacing
  fields:

  \li The height field is an xDimension by zDimension array of scalar 
  values representing the height above the grid for each vertex.
 
  \li The xSpacing and zSpacing fields indicate the distance
  between vertices in the X and Z directions respectively, and shall be greater
  than zero.

  Thus, the vertex corresponding to the point P[i, j] on the grid is
  placed at:

  \verbatim
    P[i,j].x = xSpacing × i
    P[i,j].y = height[ i + j × xDimension]
    P[i,j].z = zSpacing × j     

    where 0 <= i < xDimension and 0 <= j < zDimension, and 
    P[0,0] is height[0] units above/below the origin of the local
    coordinate system
  \endverbatim

  The set_height eventIn allows the height MFFloat field to be changed
  to support animated ElevationGrid nodes.  

  The color field specifies per-vertex or per-quadrilateral colours
  for the ElevationGrid node depending on the value of colorPerVertex.
  If the color field is NULL, the ElevationGrid node is rendered with
  the overall attributes of the Shape node enclosing the ElevationGrid
  node (see 4.14, Lighting model).  
  
  The colorPerVertex field
  determines whether colours specified in the color field are applied
  to each vertex or each quadrilateral of the ElevationGrid node. If
  colorPerVertex is FALSE and the color field is not NULL, the color
  field shall specify a Color node containing at least
  (xDimension-1)×(zDimension-1) colours; one for each quadrilateral,
  ordered as follows: 

  \verbatim
    QuadColor[i,j] = Color[ i + j × (xDimension-1)]

    where 0 <= i < xDimension-1 and 0 <= j < zDimension-1, and
    QuadColor[i,j] is the colour for the quadrilateral defined by
    height[i+j×xDimension], height[(i+1)+j×xDimension],
    height[(i+1)+(j+1)×xDimension] and height[i+(j+1)×xDimension] 
  \endverbatim
  
  If
  colorPerVertex is TRUE and the color field is not NULL, the color
  field shall specify a Color node containing at least xDimension ×
  zDimension colours, one for each vertex, ordered as follows:

  \verbatim
    VertexColor[i,j] = Color[ i + j × xDimension] 

    where 0 <= i < xDimension and 0 <= j < zDimension, and 
    VertexColor[i,j] is the colour for the vertex defined by 
    height[i+j×xDimension] 
  \endverbatim

  The normal field specifies per-vertex or per-quadrilateral normals
  for the ElevationGrid node. If the normal field is NULL, the browser
  shall automatically generate normals, using the creaseAngle field to
  determine if and how normals are smoothed across the surface (see
  4.6.3.5, Crease angle field). 

  The normalPerVertex field determines whether normals are applied to
  each vertex or each quadrilateral of the ElevationGrid node
  depending on the value of normalPerVertex. If normalPerVertex is
  FALSE and the normal node is not NULL, the normal field shall
  specify a Normal node containing at least
  (xDimension-1)×(zDimension-1) normals; one for each quadrilateral,
  ordered as follows: 

  \verbatim
    QuadNormal[i,j] = Normal[ i + j × (xDimension-1)] 

    where 0 <= i < xDimension-1 and 0 <= j < zDimension-1, and 
    QuadNormal[i,j] is the normal for the quadrilateral 
    defined by height[i+j×xDimension], height[(i+1)+j×xDimension], 
    height[(i+1)+(j+1)×xDimension] and height[i+(j+1)×xDimension] 
  \endverbatim

  If normalPerVertex is TRUE and the normal field is not NULL, the
  normal field shall specify a Normal node containing at least
  xDimension × zDimension normals; one for each vertex, ordered as
  follows:

  \verbatim
    VertexNormal[i,j] = Normal[ i + j × xDimension] 
    
    where 0 <= i < xDimension and 0 <= j < zDimension, and
    VertexNormal[i,j] is the normal for the vertex defined by
    height[i+j×xDimension] 
  \endverbatim

  The texCoord field specifies per-vertex texture coordinates for the
  ElevationGrid node. If texCoord is NULL, default texture coordinates
  are applied to the geometry. The default texture coordinates range
  from (0,0) at the first vertex to (1,1) at the last vertex. The S
  texture coordinate is aligned with the positive X-axis, and the T
  texture coordinate with positive Z-axis. If texCoord is not NULL, it
  shall specify a TextureCoordinate node containing at least
  (xDimension)×(zDimension) texture coordinates; one for each vertex,
  ordered as follows:

  \verbatim
    VertexTexCoord[i,j] = TextureCoordinate[ i + j × xDimension] 

    where 0 <= i < xDimension and 0 <= j < zDimension, and 
    VertexTexCoord[i,j] is the texture coordinate for the vertex 
    defined by height[i+j×xDimension] 
  \endverbatim

  The ccw, solid, and creaseAngle fields are described in 4.6.3,
  Shapes and geometry.  By default, the quadrilaterals are defined
  with a counterclockwise ordering.  Hence, the Y-component of the
  normal is positive. Setting the ccw field to FALSE reverses the
  normal direction. Backface culling is enabled when the solid field
  is TRUE.  See Figure 6.5 for a depiction of the ElevationGrid node.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/ElevationGrid.gif">
  Figure 6.5
  </center>

*/


/*!
  \var SoSFBool SoVRMLElevationGrid::ccw
  Specifies whether generated triangles are counterclockwise. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLElevationGrid::solid
  Specifies whether backface culling should be done. Default value is TRUE.
*/

/*!
  \var SoSFFloat SoVRMLElevationGrid::creaseAngle
  Specifies the normal calculation crease angle.
*/

/*!
  \var SoSFInt32 SoVRMLElevationGrid::zDimension
  Specifies the number of points in the z dimension.
*/

/*!
  \var SoSFInt32 SoVRMLElevationGrid::xDimension
  Specifies the number of points in the x dimension.
*/

/*!
  \var SoSFFloat SoVRMLElevationGrid::zSpacing
  Specifies the spacing between points in the z dimension. Default value is 1.0.
*/

/*!
  \var SoSFFloat SoVRMLElevationGrid::xSpacing
  Specifies the spacing between points in the x dimension. Default value is 1.0.
*/

/*!
  \var SoMFFloat SoVRMLElevationGrid::height
  Contains the array of height values.
*/

/*!
  \var SoSFNode SoVRMLElevationGrid::texCoord
  Can contain an SoVRMLTextureCoordinate node containing one texture coordinate per grid point.
*/

/*!
  \var SoSFNode SoVRMLElevationGrid::normal
  Can contain an SoVRMLTextureCoordinate node containing normals for the grid.
*/

/*!
  \var SoSFNode SoVRMLElevationGrid::color
  Can contain an SoVRMLColor node containing grid colors.
*/

/*!
  \var SoSFBool SoVRMLElevationGrid::colorPerVertex
  Specifies whether colors should be applied per vertex. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLElevationGrid::normalPerVertex
  Specifies whether normals should be applied per vertex. Default value is TRUE.
*/

#include <Inventor/VRMLnodes/SoVRMLElevationGrid.h>
#include "coindefs.h"

#include <cfloat>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/VRMLnodes/SoVRMLColor.h>
#include <Inventor/VRMLnodes/SoVRMLNormal.h>
#include <Inventor/VRMLnodes/SoVRMLTextureCoordinate.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbRWMutex.h>
#endif // HAVE_THREADS

#include "nodes/SoSubNodeP.h"

// *************************************************************************

class SoVRMLElevationGridP {
public:
  SoVRMLElevationGridP(void)
    : dirty(TRUE),
      ngen(TRUE)
#ifdef COIN_THREADSAFE
      , mutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  { }

  SbBool dirty;
  SoNormalGenerator ngen;
  SoVRMLElevationGrid::Binding nbind;

#ifdef COIN_THREADSAFE
  SbRWMutex mutex;
  void readLockNormalCache(void) { this->mutex.readLock(); }
  void readUnlockNormalCache(void) { this->mutex.readUnlock(); }
  void writeLockNormalCache(void) { this->mutex.writeLock(); }
  void writeUnlockNormalCache(void) { this->mutex.writeUnlock(); }
#else // ! COIN_THREADSAFE
  void readLockNormalCache(void) { }
  void readUnlockNormalCache(void) { }
  void writeLockNormalCache(void) { }
  void writeUnlockNormalCache(void) { }
#endif // ! COIN_THREADSAFE
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLElevationGrid);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLElevationGrid::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLElevationGrid, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLElevationGrid::SoVRMLElevationGrid(void)
{
  PRIVATE(this) = new SoVRMLElevationGridP;

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLElevationGrid);

  SO_VRMLNODE_ADD_FIELD(ccw, (TRUE));
  SO_VRMLNODE_ADD_FIELD(solid, (TRUE));
  SO_VRMLNODE_ADD_FIELD(creaseAngle, (0.0f));
  SO_VRMLNODE_ADD_FIELD(zDimension, (0));
  SO_VRMLNODE_ADD_FIELD(xDimension, (0));
  SO_VRMLNODE_ADD_FIELD(zSpacing, (1.0f));
  SO_VRMLNODE_ADD_FIELD(xSpacing, (1.0f));
  SO_VRMLNODE_ADD_EMPTY_MFIELD(height);
  SO_VRMLNODE_ADD_EXPOSED_FIELD(texCoord, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(normal, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(color, (NULL));
  SO_VRMLNODE_ADD_FIELD(colorPerVertex, (TRUE));
  SO_VRMLNODE_ADD_FIELD(normalPerVertex, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLElevationGrid::~SoVRMLElevationGrid(void)
{
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLElevationGrid::GLRender(SoGLRenderAction * action)
{
  const int xdim = this->xDimension.getValue();
  const int zdim = this->zDimension.getValue();
  if (xdim < 2 || zdim < 2) return;

  if (this->height.getNum() < xdim*zdim) {
    SoDebugError::postWarning("SoVRMLElevationGrid::GLRender",
                              "Too few height values. "
                              "Expected %d values, got %d",
                              xdim*zdim, this->height.getNum());
    return;
  }

  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();
  state->push();

  // update state with color information
  SoNode * node = this->color.getValue();
  if (node) node->GLRender(action);

  this->setupShapeHints(state, this->ccw.getValue(), this->solid.getValue());

  SoMaterialBundle mb(action);

  SbBool dotex = SoGLMultiTextureEnabledElement::get(state);
  SbBool donorm = !mb.isColorOnly();

  Binding nbind = this->findNormalBinding();
  Binding mbind = this->findMaterialBinding();

  if (!donorm) nbind = OVERALL;

  const SbVec2f * tcoords = NULL;
  SoVRMLTextureCoordinate * tnode = (SoVRMLTextureCoordinate*)
    this->texCoord.getValue();

  if (tnode) tcoords = tnode->point.getValues(0);

  mb.sendFirst();

  SbBool normalcache = FALSE;
  const SbVec3f * normals = NULL;
  if (nbind != OVERALL) {
    SoVRMLNormal * nnode = (SoVRMLNormal*) this->normal.getValue();
    if (nnode) normals = nnode->vector.getValues(0);
    if (normals == NULL) {
      normals = this->updateNormalCache(nbind);
      normalcache = TRUE;
    }
  }

  const float * h = this->height.getValues(0);

  float currz = 0.0f;

  const float zspace = this->zSpacing.getValue();
  const float xspace = this->xSpacing.getValue();

  int idx = 0;
  int idx2 = xdim;

  SbBool drawasstrip = TRUE;
  if (nbind == PER_VERTEX && normalcache) drawasstrip = FALSE; // crease angle normals
  if (nbind == PER_QUAD && mbind == PER_VERTEX) drawasstrip = FALSE;

  if (nbind <= PER_QUAD && mbind <= PER_QUAD) {
    SoGLLazyElement::sendFlatshading(state, TRUE);
  }

  // FIXME: Probably too slow. Need several rendering loops
  if (drawasstrip) {
    for (int z = 0; z < zdim-1; z++) {
      float currx = 0.0f;
      const float * nexth = h + xdim;
      float nextz = currz + zspace;

      glBegin(GL_QUAD_STRIP);

      if (nbind == PER_VERTEX) {
        glNormal3fv(normals[idx].getValue());
      }
      if (dotex) {
        if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
        else glTexCoord2f(0.0f, float(z)/float(zdim-1));
      }
      if (mbind == PER_VERTEX) {
        mb.send(idx, TRUE);
      }
      glVertex3f(currx, h[0], currz);
      idx++;

      if (nbind == PER_VERTEX) {
        glNormal3fv(normals[idx2].getValue());
      }
      if (dotex) {
        if (tcoords) glTexCoord2fv(tcoords[idx2].getValue());
        else glTexCoord2f(0.0f, float(z+1)/float(zdim-1));
      }
      if (mbind == PER_VERTEX) {
        mb.send(idx2, TRUE);
      }
      glVertex3f(currx, nexth[0], nextz);
      idx2++;
      currx += xspace;

      for (int x = 1; x < xdim; x++) {
        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
          else glTexCoord2f(float(x) / float(xdim-1), float(z)/float(zdim-1));
        }
        if (nbind == PER_VERTEX) {
          glNormal3fv(normals[idx].getValue());
        }
        if (mbind == PER_VERTEX) {
          mb.send(idx, TRUE);
        }
        glVertex3f(currx, h[x], currz);
        idx++;

        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx2].getValue());
          else glTexCoord2f(float(x) / float(xdim-1), float(z+1)/float(zdim-1));
        }
        if (nbind == PER_VERTEX) {
          glNormal3fv(normals[idx2].getValue());
        }
        else if (nbind == PER_QUAD) {
          glNormal3fv(normals[0].getValue());
          normals++;
        }
        if (mbind != OVERALL) {
          mb.send(idx2, TRUE);
        }
        glVertex3f(currx, nexth[x], nextz);
        idx2++;
        currx += xspace;
      }
      currz += zspace;
      nextz += zspace;
      h += xdim;
      nexth += xdim;
      glEnd();
    }
  }
  else {
    glBegin(GL_QUADS);

    for (int z = 0; z < zdim-1; z++) {
      float currx = 0.0f;
      float nextz = currz + zspace;

      int mcnt = 0;

      for (int x = 0; x < xdim-1; x++) {
        // vertex 1
        idx = x + (z+1) * xdim;
        if (nbind == PER_VERTEX) {
          if (normalcache) {
            glNormal3fv(normals->getValue());
            normals++;
          }
          else {
            glNormal3fv(normals[idx].getValue());
          }
        }
        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
          else glTexCoord2f(float(x) / float(xdim-1), float(z+1)/float(zdim-1));
        }
        if (mbind == PER_VERTEX) {
          mb.send(idx, TRUE);
        }
        glVertex3f(currx, h[idx], nextz);

        // vertex 2
        idx++; // use next on this line
        if (nbind == PER_VERTEX) {
          if (normalcache) {
            glNormal3fv(normals->getValue());
            normals++;
          }
          else {
            glNormal3fv(normals[idx].getValue());
          }
        }
        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
          else glTexCoord2f(float(x+1)/float(xdim-1), float(z+1)/float(zdim-1));
        }
        if (mbind == PER_VERTEX) {
          mb.send(idx, TRUE);
        }
        glVertex3f(currx+xspace, h[idx], nextz);

        // vertex 3
        idx -= xdim; // use previous line
        if (nbind == PER_VERTEX) {
          if (normalcache) {
            glNormal3fv(normals->getValue());
            normals++;
          }
          else {
            glNormal3fv(normals[idx].getValue());
          }
        }
        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
          else glTexCoord2f(float(x+1)/float(xdim-1), float(z)/float(zdim-1));
        }
        if (mbind == PER_VERTEX) {
          mb.send(idx, TRUE);
        }
        glVertex3f(currx+xspace, h[idx], currz);

        // vertex 4
        idx--; // use previous on this line
        if (nbind == PER_QUAD  || (nbind == PER_VERTEX && normalcache)) {
          glNormal3fv(normals->getValue());
          normals++;
        }
        else if (nbind == PER_VERTEX) {
          glNormal3fv(normals[idx].getValue());
        }
        if (dotex) {
          if (tcoords) glTexCoord2fv(tcoords[idx].getValue());
          else glTexCoord2f(float(x)/float(xdim-1), float(z)/float(zdim-1));
        }
        if (mbind == PER_VERTEX) {
          mb.send(idx, TRUE);
        }
        else if (mbind == PER_QUAD) {
          mb.send(mcnt++, TRUE);
        }
        glVertex3f(currx, h[idx], currz);
        currx += xspace;
      }

      currz += zspace;
      nextz += zspace;
    }
    glEnd();
  }

  if (normalcache) PRIVATE(this)->readUnlockNormalCache();
  state->pop();
}

// Doc in parent
void
SoVRMLElevationGrid::rayPick(SoRayPickAction * action)
{
  // FIXME: implement optimized version
  inherited::rayPick(action);
}

// Doc in parent
void
SoVRMLElevationGrid::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (this->shouldPrimitiveCount(action)) {
    action->addNumTriangles((xDimension.getValue()-1)*(zDimension.getValue()-1)*2);
  }
}

// Doc in parent
void
SoVRMLElevationGrid::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                                 SbBox3f & bbox,
                                 SbVec3f & center)
{
  // FIXME: consider caching minh, maxh in the private class.

  const int n = this->height.getNum();
  if (n == 0) return;

  const float * h = this->height.getValues(0);

  float minh, maxh;
  minh = maxh = h[0];
  for (int i = 1; i < n; i++) {
    if (h[i] < minh) minh = h[i];
    else if (h[i] > maxh) maxh = h[i];
  }

  bbox.setBounds(0.0f, minh, 0.0f,
                 this->xDimension.getValue() * this->xSpacing.getValue(),
                 maxh,
                 this->zDimension.getValue() * this->zSpacing.getValue());

  center = bbox.getCenter();
}

// Doc in parent
void
SoVRMLElevationGrid::generatePrimitives(SoAction * action)
{
  const int xdim = this->xDimension.getValue();
  const int zdim = this->zDimension.getValue();
  if (xdim < 2 || zdim < 2) return;

  if (this->height.getNum() != xdim*zdim) {
    SoDebugError::postWarning("SoVRMLElevationGrid::generatePrimitives",
                              "Wrong number of height values.");
    return;
  }

  SoState * state = action->getState();

  SbBool donorm = SoLazyElement::getLightModel(state) !=
    SoLazyElement::BASE_COLOR;

  Binding nbind = this->findNormalBinding();
  Binding mbind = this->findMaterialBinding();

  if (!donorm) nbind = OVERALL;

  const SbVec2f * tcoords = NULL;
  SoVRMLTextureCoordinate * tnode = (SoVRMLTextureCoordinate*)
    this->texCoord.getValue();

  if (tnode) tcoords = tnode->point.getValues(0);

  SbBool normalcache = FALSE;
  const SbVec3f * normals = NULL;
  if (nbind != OVERALL) {
    SoVRMLNormal * nnode = (SoVRMLNormal*) this->normal.getValue();
    if (nnode) normals = nnode->vector.getValues(0);
    if (normals == NULL) {
      // updateNormalCache will readLock the normal cache. We unlock
      // at the end of this function.
      normals = this->updateNormalCache(nbind);
      normalcache = TRUE;
    }
  }

  const float * h = this->height.getValues(0);
  const float * nexth = h + xdim;

  float currz = 0.0f;

  const float zspace = this->zSpacing.getValue();
  const float xspace = this->xSpacing.getValue();

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  SoFaceDetail faceDetail;

  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0, 1, 0);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;
  vertex.setNormal(*currnormal);

  this->beginShape(action, QUADS, &faceDetail);

  int idx;
  int cnt = 0;

  for (int z = 0; z < zdim-1; z++) {
    float currx = 0.0f;
    for (int x = 0; x < xdim-1; x++) {

      // vertex 1
      idx = x+(z+1)*xdim;
      SbVec3f v0(currx, nexth[x], currz+zspace);
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(idx);
        vertex.setMaterialIndex(idx);
      }
      else if (mbind == PER_QUAD) {
        pointDetail.setMaterialIndex(cnt);
        vertex.setMaterialIndex(cnt);
      }
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(idx);
        if (normalcache) {
          vertex.setNormal(*currnormal);
          currnormal++;
        }
        else {
          vertex.setNormal(normals[idx]);
        }
      }
      else if (nbind == PER_QUAD) {
        pointDetail.setNormalIndex(cnt);
        vertex.setNormal(*currnormal);
        currnormal++;
      }
      pointDetail.setTextureCoordIndex(idx);
      pointDetail.setCoordinateIndex(idx);
      if (tcoords) vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(v0);
      this->shapeVertex(&vertex);

      // vertex 2
      SbVec3f v1(currx+xspace, nexth[x+1], currz+zspace);
      idx++;
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(idx);
        vertex.setMaterialIndex(idx);
      }
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(idx);
        if (normalcache) {
          vertex.setNormal(*currnormal);
          currnormal++;
        }
        else {
          vertex.setNormal(normals[idx]);
        }
      }
      pointDetail.setTextureCoordIndex(idx);
      pointDetail.setCoordinateIndex(idx);
      if (tcoords) vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(v1);
      this->shapeVertex(&vertex);

      // vertex 3
      SbVec3f v2(currx+xspace, h[x+1], currz);
      idx -= xdim;
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(idx);
        vertex.setMaterialIndex(idx);
      }
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(idx);
        if (normalcache) {
          vertex.setNormal(*currnormal);
          currnormal++;
        }
        else {
          vertex.setNormal(normals[idx]);
        }
      }
      pointDetail.setTextureCoordIndex(idx);
      pointDetail.setCoordinateIndex(idx);
      if (tcoords) vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(v2);
      this->shapeVertex(&vertex);

      // vertex 4
      SbVec3f v3(currx, h[x], currz);
      idx--;
      if (mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(idx);
        vertex.setMaterialIndex(idx);
      }
      if (nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(idx);
        if (normalcache) {
          vertex.setNormal(*currnormal);
          currnormal++;
        }
        else {
          vertex.setNormal(normals[idx]);
        }
      }
      pointDetail.setCoordinateIndex(idx);
      if (tcoords) vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(v3);
      this->shapeVertex(&vertex);

      currx += xspace;
      faceDetail.incFaceIndex();
    }
    h += xdim;
    nexth += xdim;
    currz += zspace;
    cnt++;
  }
  this->endShape();

  if (normalcache) PRIVATE(this)->readUnlockNormalCache();
}

//
// private method used to find the material/color binding.
//
SoVRMLElevationGrid::Binding
SoVRMLElevationGrid::findMaterialBinding(void) const
{
  Binding binding = OVERALL;
  if (this->color.getValue()) {
    if (this->colorPerVertex.getValue()) binding = PER_VERTEX;
    else binding = PER_QUAD;
  }
  return binding;
}

//
// private method used to find the normal binding.
//
SoVRMLElevationGrid::Binding
SoVRMLElevationGrid::findNormalBinding(void) const
{
  Binding binding = PER_VERTEX;
  if (this->normal.getValue()) {
    if (!this->normalPerVertex.getValue()) binding = PER_QUAD;
  }
  return binding;
}

// Doc in parent
void
SoVRMLElevationGrid::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->height ||
      f == &this->creaseAngle ||
      f == &this->ccw ||
      f == &this->xDimension ||
      f == &this->zDimension ||
      f == &this->xSpacing ||
      f == &this->zSpacing) {
    PRIVATE(this)->dirty = TRUE;
  }
  inherited::notify(list);
}

//
// private method that recalculates the normal cache
//
const SbVec3f *
SoVRMLElevationGrid::updateNormalCache(Binding & nbind)
{
  PRIVATE(this)->readLockNormalCache();

  if (PRIVATE(this)->dirty) {
    PRIVATE(this)->readUnlockNormalCache();
    PRIVATE(this)->writeLockNormalCache();
    // FIXME: optimize by using a specialized algorithm for
    // calculating the normals.
    PRIVATE(this)->ngen.reset(this->ccw.getValue());

    const int xdim = this->xDimension.getValue();
    const int zdim = this->zDimension.getValue();
    if (xdim < 2 || zdim < 2) return NULL;

    const float * h = this->height.getValues(0);

    const float * nexth = h + xdim;

    float currz = 0.0f;

    const float zspace = this->zSpacing.getValue();
    const float xspace = this->xSpacing.getValue();

    for (int z = 0; z < zdim-1; z++) {
      float currx = 0.0f;
      for (int x = 0; x < xdim-1; x++) {

        SbVec3f v0(currx, nexth[x], currz+zspace);
        SbVec3f v1(currx+xspace, nexth[x+1], currz+zspace);
        SbVec3f v2(currx+xspace, h[x+1], currz);
        SbVec3f v3(currx, h[x], currz);

        PRIVATE(this)->ngen.quad(v0, v1, v2, v3);
        currx += xspace;
      }
      h += xdim;
      nexth += xdim;
      currz += zspace;
    }

    if (this->creaseAngle.getValue() <= FLT_EPSILON) {
      PRIVATE(this)->nbind = PER_QUAD;
      PRIVATE(this)->ngen.generatePerFace();
    }
    else {
      PRIVATE(this)->nbind = PER_VERTEX;
      PRIVATE(this)->ngen.generate(this->creaseAngle.getValue());
    }
    PRIVATE(this)->dirty = FALSE;
    PRIVATE(this)->writeUnlockNormalCache();
    PRIVATE(this)->readLockNormalCache();
  }

  // cache is read locked when we get here
  nbind = PRIVATE(this)->nbind;
  return PRIVATE(this)->ngen.getNormals();
}

#undef PRIVATE

#endif // HAVE_VRML97
