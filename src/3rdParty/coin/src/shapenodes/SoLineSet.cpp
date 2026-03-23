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
  \class SoLineSet SoLineSet.h Inventor/nodes/SoLineSet.h
  \brief The SoLineSet class is used to render and organize non-indexed polylines.

  \ingroup coin_nodes

  Polylines are specified using the numVertices field. Coordinates,
  normals, materials and texture coordinates are fetched in order from
  the current state or from the vertexProperty node if set. For
  example, if numVertices is set to [3, 4, 2], this node would specify
  a line through coordinates 0, 1 and 2, a line through coordinates 3, 4, 5
  and 6, and finally a single line segment between coordinates 7 and 8.

  Here's a very simple usage example:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
     Coordinate3 {
        point [ 0 0 0, 1 1 1, 2 1 1, 2 2 1, 2 2 2, 2 2 3, 2 3 2, 2 3 3, 3 3 3 ]
     }

     LineSet {
        numVertices [ 3, 4, 2 ]
     }
  }
  \endverbatim


  Binding PER_VERTEX, PER_FACE, PER_PART or OVERALL can be set for
  material, and normals. The default material binding is OVERALL. The
  default normal binding is PER_VERTEX. If no normals are set, the
  line set will be rendered with lighting disabled.

  The width of the rendered lines can be controlled through the
  insertion of an SoDrawStyle node in front of SoLineSet node(s) in
  the scene graph.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    LineSet {
        vertexProperty NULL
        startIndex 0
        numVertices -1
    }
  \endcode

  \sa SoIndexedLineSet
*/

#include <Inventor/nodes/SoLineSet.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/misc/SoState.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/details/SoLineDetail.h>

#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"

/*!
  \var SoMFInt32 SoLineSet::numVertices
  Used to specify polylines. Each entry specifies the number of coordinates
  in a line. The coordinates are taken in order from the state or from
  the vertexProperty node.
*/

// *************************************************************************

SO_NODE_SOURCE(SoLineSet);

/*!
  Constructor.
*/
SoLineSet::SoLineSet()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoLineSet);

  SO_NODE_ADD_FIELD(numVertices, (-1));
}

/*!
  Destructor.
*/
SoLineSet::~SoLineSet()
{
}

// doc from parent
void
SoLineSet::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  int32_t numvertices = 0;
  for (int i=0; i < this->numVertices.getNum(); i++)
    numvertices += this->numVertices[i];
  inherited::computeCoordBBox(action, numvertices, box, center);
}

//
// translates the current material binding to the internal Binding enum
//
SoLineSet::Binding
SoLineSet::findMaterialBinding(SoState * const state) const
{
  Binding binding = OVERALL;
  SoMaterialBindingElement::Binding matbind =
    SoMaterialBindingElement::get(state);

  switch (matbind) {
  case SoMaterialBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoMaterialBindingElement::PER_VERTEX:
  case SoMaterialBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX;
    break;
  case SoMaterialBindingElement::PER_PART:
  case SoMaterialBindingElement::PER_PART_INDEXED:
    binding = PER_SEGMENT;
    break;
  case SoMaterialBindingElement::PER_FACE:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_LINE;
    break;
  default:
    binding = OVERALL;
#if COIN_DEBUG
    SoDebugError::postWarning("SoLineSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates the current normal binding to the internal Binding enum
//
SoLineSet::Binding
SoLineSet::findNormalBinding(SoState * const state) const
{
  Binding binding = PER_VERTEX;

  SoNormalBindingElement::Binding normbind =
    SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoNormalBindingElement::PER_VERTEX:
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX;
    break;
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_PART_INDEXED:
    binding = PER_SEGMENT;
    break;
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_LINE;
    break;
  default:
    binding = PER_VERTEX;
#if COIN_DEBUG
    SoDebugError::postWarning("SoLineSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

namespace { namespace SoGL { namespace LineSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_LINE = 1,
    PER_SEGMENT = 2,
    PER_VERTEX = 3
  };

  template < int NormalBinding,
             int MaterialBinding,
             int TexturingEnabled >
  static void GLRender(const SoGLCoordinateElement * coords,
                       const SbVec3f *normals,
                       SoMaterialBundle * mb,
                       const SoTextureCoordinateBundle * tb,
                       int32_t idx,
                       const int32_t *ptr,
                       const int32_t *end,
                       SbBool needNormals,
                       SbBool drawPoints)
  {
    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = coords->is3D();
    if (is3d) {
      coords3d = coords->getArrayPtr3();
    }
    else {
      coords4d = coords->getArrayPtr4();
    }

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_)                                        \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_)); \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f * currnormal = &dummynormal;
    if (normals) currnormal = normals;
    if ((AttributeBinding)NormalBinding == OVERALL) {
      if (needNormals)
        glNormal3fv((const GLfloat *)currnormal);
    }

    int matnr = 0;
    int texnr = 0;

    if ((AttributeBinding)NormalBinding == PER_SEGMENT ||
        (AttributeBinding)MaterialBinding == PER_SEGMENT) {

      if (drawPoints) glBegin(GL_POINTS);
      else glBegin(GL_LINES);

      while (ptr < end) {
        int n = *ptr++;
        if (n < 2) {
          idx += n;
          continue;
        }
        if ((AttributeBinding)MaterialBinding == PER_LINE ||
            (AttributeBinding)MaterialBinding == PER_VERTEX) {
          mb->send(matnr++, TRUE);
        }
        if ((AttributeBinding)NormalBinding == PER_LINE ||
            (AttributeBinding)NormalBinding == PER_VERTEX) {
          currnormal = normals++;
          glNormal3fv((const GLfloat*)currnormal);
        }
        if (TexturingEnabled == TRUE) {
          tb->send(texnr++, coords->get3(idx), *currnormal);
        }

        while (--n) {
          if ((AttributeBinding)MaterialBinding == PER_SEGMENT) {
            mb->send(matnr++, TRUE);
          }
          if ((AttributeBinding)NormalBinding == PER_SEGMENT) {
            currnormal = normals++;
            glNormal3fv((const GLfloat*)currnormal);
          }
          SEND_VERTEX(idx);
          idx++;

          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = normals++;
            glNormal3fv((const GLfloat *)currnormal);
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            mb->send(matnr++, TRUE);
          }
          if (TexturingEnabled == TRUE) {
            tb->send(texnr++, coords->get3(idx), *currnormal);
          }
          SEND_VERTEX(idx);
        }
        idx++;
      }
      glEnd();

    } else { // NBINDING==PER_SEGMENT || MBINDING==PER_SEGMENT

      if (drawPoints) glBegin(GL_POINTS);
      while (ptr < end) {
        int n = *ptr++;
        if (n < 2) {
          idx += n; // FIXME: is this correct?
          continue;
        }
        n -= 2;
        if (!drawPoints) glBegin(GL_LINE_STRIP);

        if ((AttributeBinding)NormalBinding != OVERALL) {
          currnormal = normals++;
          glNormal3fv((const GLfloat *)currnormal);
        }
        if ((AttributeBinding)MaterialBinding != OVERALL) {
          mb->send(matnr++, TRUE);
        }
        if (TexturingEnabled == TRUE) {
          tb->send(texnr++, coords->get3(idx), *currnormal);
        }
        SEND_VERTEX(idx);
        idx++;
        do {
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = normals++;
            glNormal3fv((const GLfloat *)currnormal);
          }
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            mb->send(matnr++, TRUE);
          }
          if (TexturingEnabled == TRUE) {
            tb->send(texnr++, coords->get3(idx), *currnormal);
          }
          SEND_VERTEX(idx);
          idx++;
        } while (n--);
        if (!drawPoints) glEnd();
      }
    }
    if (drawPoints) glEnd();
#undef SEND_VERTEX
  }

} } } // namespace

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoLineSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoLineSet, SO_FROM_INVENTOR_1);
}

#define SOGL_LINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::LineSet::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_LINESET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) {                                                      \
    SOGL_LINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else {                                                              \
    SOGL_LINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_LINESET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) {                                            \
  case SoGL::LineSet::OVERALL:                                          \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::LineSet::OVERALL, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_LINE:                                         \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::LineSet::PER_LINE, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_SEGMENT:                                      \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::LineSet::PER_SEGMENT, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_VERTEX:                                       \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::LineSet::PER_VERTEX, texturing, args); \
    break;                                                              \
  default:                                                              \
    assert(!"invalid materialbinding argument");                        \
    break;                                                              \
  }

#define SOGL_LINESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) {                                              \
  case SoGL::LineSet::OVERALL:                                          \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG2(SoGL::LineSet::OVERALL, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_LINE:                                         \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG2(SoGL::LineSet::PER_LINE, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_SEGMENT:                                      \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG2(SoGL::LineSet::PER_SEGMENT, materialbinding, texturing, args); \
    break;                                                              \
  case SoGL::LineSet::PER_VERTEX:                                       \
    SOGL_LINESET_GLRENDER_RESOLVE_ARG2(SoGL::LineSet::PER_VERTEX, materialbinding, texturing, args); \
    break;                                                              \
  default:                                                              \
    assert(!"invalid normalbinding argument");                          \
    break;                                                              \
  }

#define SOGL_LINESET_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_LINESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)

// doc from parent
void
SoLineSet::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SbBool didpush = FALSE;

  if (this->vertexProperty.getValue()) {
    state->push();
    didpush = TRUE;
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (didpush) {
      state->pop();
    }
    return;
  }

  int32_t idx = this->startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = this->numVertices.getValues(0);
  const int32_t * end = ptr + this->numVertices.getNum();
  if ((end - ptr == 1) && ptr[0] == 0) {
    if (didpush) state->pop();
    return; // nothing to render
  }
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);


  SoMaterialBundle mb(action);
  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  SbBool doTextures = tb.needCoordinates();

  const SoCoordinateElement * tmp;
  const SbVec3f * normals;

  SbBool needNormals = !mb.isColorOnly() || tb.isFunction();

  SoVertexShape::getVertexData(state, tmp, normals,
                               needNormals);
  if (normals == NULL && needNormals) {
    needNormals = FALSE;
    if (!didpush) {
      state->push();
      didpush = TRUE;
    }
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  }

  const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

  Binding mbind = findMaterialBinding(action->getState());


  Binding nbind;
  if (!needNormals) nbind = OVERALL;
  else nbind = findNormalBinding(action->getState());

  mb.sendFirst(); // make sure we have the correct material

  SbBool drawPoints =
    SoDrawStyleElement::get(state) == SoDrawStyleElement::POINTS;

  SOGL_LINESET_GLRENDER(nbind, mbind, doTextures, (coords,
                                                   normals,
                                                   &mb,
                                                   &tb,
                                                   idx,
                                                   ptr,
                                                   end,
                                                   needNormals,
                                                   drawPoints));

  if (didpush)
    state->pop();

  int numv = this->numVertices.getNum();
  // send approx number of lines for autocache handling
  sogl_autocache_update(state, numv ?
                        (this->numVertices[0]-1)*numv : 0, FALSE);
}

#undef SOGL_LINESET_GLRENDER_CALL_FUNC
#undef SOGL_LINESET_GLRENDER_RESOLVE_ARG3
#undef SOGL_LINESET_GLRENDER_RESOLVE_ARG2
#undef SOGL_LINESET_GLRENDER_RESOLVE_ARG1
#undef SOGL_LINESET_GLRENDER

// Documented in superclass.
SbBool
SoLineSet::generateDefaultNormals(SoState * , SoNormalCache * nc)
{
  // not possible to generate normals for LineSet
  nc->set(0, NULL);
  return TRUE;
}

// Documented in superclass.
SbBool
SoLineSet::generateDefaultNormals(SoState *, SoNormalBundle *)
{
  return FALSE;
}

// doc from parent
void
SoLineSet::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
  // notify open (if any) bbox caches about lines in this shape
  SoBoundingBoxCache::setHasLinesOrPoints(action->getState());
}

// doc from parent
void
SoLineSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int32_t dummyarray[1];
  const int32_t *ptr = this->numVertices.getValues(0);
  const int32_t *end = ptr + this->numVertices.getNum();

  if ((end-ptr == 1) && (*ptr == 0)) return;

  this->fixNumVerticesPointers(action->getState(), ptr, end, dummyarray);

  if (action->canApproximateCount()) {
    const ptrdiff_t diff = end - ptr;
    action->addNumLines((int)diff);
  }
  else {
    int cnt = 0;
    while (ptr < end) {
      cnt += *ptr++ - 1;
    }
    action->addNumLines(cnt);
  }
}

// doc from parent
void
SoLineSet::generatePrimitives(SoAction *action)
{
  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  const SoCoordinateElement *coords;
  const SbVec3f * normals;
  SbBool doTextures;
  SbBool needNormals = TRUE;

  SoVertexShape::getVertexData(action->getState(), coords, normals,
                               needNormals);

  if (normals == NULL) needNormals = FALSE;

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  Binding mbind = findMaterialBinding(action->getState());
  Binding nbind = findNormalBinding(action->getState());

  if (!needNormals) nbind = OVERALL;

  SoPrimitiveVertex vertex;
  SoLineDetail lineDetail;
  SoPointDetail pointDetail;

  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;
  if (nbind == OVERALL && needNormals) {
    vertex.setNormal(*currnormal);
  }

  int32_t idx = this->startIndex.getValue();
  int32_t dummyarray[1];
  const int32_t * ptr = this->numVertices.getValues(0);
  const int32_t * end = ptr + this->numVertices.getNum();
  this->fixNumVerticesPointers(state, ptr, end, dummyarray);

  int normnr = 0;
  int matnr = 0;
  int texnr = 0;

  if (nbind == PER_SEGMENT || mbind == PER_SEGMENT) {
    this->beginShape(action, SoShape::LINES, &lineDetail);

    while (ptr < end) {
      int n = *ptr++;
      if (n < 2) {
        idx += n;
        continue;
      }
      if (nbind == PER_LINE || nbind == PER_VERTEX) {
        pointDetail.setNormalIndex(normnr);
        currnormal = &normals[normnr++];
        vertex.setNormal(*currnormal);
      }
      if (mbind == PER_LINE || mbind == PER_VERTEX) {
        pointDetail.setMaterialIndex(matnr);
        vertex.setMaterialIndex(matnr++);
      }
      if (doTextures) {
        if (tb.isFunction())
          vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
        else {
          pointDetail.setTextureCoordIndex(texnr);
          vertex.setTextureCoords(tb.get(texnr++));
        }
      }
      while (--n) {
        if (nbind == PER_SEGMENT) {
          pointDetail.setNormalIndex(normnr);
          currnormal = &normals[normnr++];
          vertex.setNormal(*currnormal);
        }
        if (mbind == PER_SEGMENT) {
          pointDetail.setMaterialIndex(matnr);
          vertex.setMaterialIndex(matnr++);
        }
        pointDetail.setCoordinateIndex(idx);
        vertex.setPoint(coords->get3(idx++));
        this->shapeVertex(&vertex);

        if (nbind == PER_VERTEX) {
          pointDetail.setNormalIndex(normnr);
          currnormal = &normals[normnr++];
          vertex.setNormal(*currnormal);
        }
        if (mbind == PER_VERTEX) {
          pointDetail.setMaterialIndex(matnr);
          vertex.setMaterialIndex(matnr++);
        }
        if (doTextures) {
          if (tb.isFunction())
            vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
          else {
            pointDetail.setTextureCoordIndex(texnr);
            vertex.setTextureCoords(tb.get(texnr++));
          }
        }
        pointDetail.setCoordinateIndex(idx);
        vertex.setPoint(coords->get3(idx));
        this->shapeVertex(&vertex);
        lineDetail.incPartIndex();
      }
      lineDetail.incLineIndex();
      idx++; // next (poly)line should use the next index
    }
    this->endShape();
  }
  else {
    while (ptr < end) {
      lineDetail.setPartIndex(0);
      int n = *ptr++;
      if (n < 2) {
        idx += n;
        continue;
      }
      n -= 2;
      this->beginShape(action, SoShape::LINE_STRIP, &lineDetail);
      if (nbind != OVERALL) {
        pointDetail.setNormalIndex(normnr);
        currnormal = &normals[normnr++];
        vertex.setNormal(*currnormal);
      }
      if (mbind != OVERALL) {
        pointDetail.setMaterialIndex(matnr);
        vertex.setMaterialIndex(matnr++);
      }
      if (doTextures) {
        if (tb.isFunction())
          vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
        else {
          pointDetail.setTextureCoordIndex(texnr);
          vertex.setTextureCoords(tb.get(texnr++));
        }
      }
      pointDetail.setCoordinateIndex(idx);
      vertex.setPoint(coords->get3(idx++));
      this->shapeVertex(&vertex);
      do {
        if (nbind == PER_VERTEX) {
          pointDetail.setNormalIndex(normnr);
          currnormal = &normals[normnr++];
          vertex.setNormal(*currnormal);
        }
        if (mbind == PER_VERTEX) {
          pointDetail.setMaterialIndex(matnr);
          vertex.setMaterialIndex(matnr++);
        }
        if (doTextures) {
          if (tb.isFunction())
            vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
          else {
            pointDetail.setTextureCoordIndex(texnr);
            vertex.setTextureCoords(tb.get(texnr++));
          }
        }
        pointDetail.setCoordinateIndex(idx);
        vertex.setPoint(coords->get3(idx++));
        this->shapeVertex(&vertex);
        lineDetail.incPartIndex();
      } while (n--);
      this->endShape();
      lineDetail.incLineIndex();
    }
  }
  if (this->vertexProperty.getValue())
    state->pop();
}
