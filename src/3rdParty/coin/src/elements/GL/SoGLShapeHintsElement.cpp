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
  \class SoGLShapeHintsElement Inventor/elements/SoGLShapeHintsElement.h
  \brief The SoGLShapeHintsElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

//
// handling of cases (from the original oiv man-pages):
//
// 1) ordered, solid   :   backface culling, ! twoside lighting
// 2) ordered, ! solid : ! backface culling,   twoside lighting
// 3) ! ordered        : ! backface culling, ! twoside lighting
//
// (this table of rendering rules is duplicated in the SoShapeHints
// classdoc)

//
// I find this strategy quite odd. My gut-feeling says that
// when vertexordering is unknown, two-side-lighting should be
// enabled, since it will be difficult to know if a normal
// points in or out of a polygon...
//

//
// nodes with automatically generated normals should probably
// force two-side lighting when vertexordering is unknown, since
// it is impossible to know if normals are pointing in or out.
//
// use SoGLShapeHintsElement::forceSend(twoside) for this purpose
//

#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoGLLazyElement.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoGLShapeHintsElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLShapeHintsElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLShapeHintsElement, inherited);
}

/*!
  Destructor.
*/

SoGLShapeHintsElement::~SoGLShapeHintsElement(void)
{
}

//! FIXME: write doc.

void
SoGLShapeHintsElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr;
}

//! FIXME: write doc.

void
SoGLShapeHintsElement::push(SoState * stateptr)
{
  inherited::push(stateptr);
  SoGLShapeHintsElement * prev =
    (SoGLShapeHintsElement *) this->getNextInStack();
  this->state = prev->state;
}

//! FIXME: write doc.

void
SoGLShapeHintsElement::pop(SoState * stateptr,
                           const SoElement * prevTopElement)
{
  inherited::pop(stateptr, prevTopElement);
}

//! FIXME: write doc.

void
SoGLShapeHintsElement::setElt(VertexOrdering vertexOrderingarg,
                              ShapeType shapeTypearg,
                              FaceType faceTypearg)
{
  inherited::setElt(vertexOrderingarg, shapeTypearg, faceTypearg);
  // do nothing since GL stuff is handled by SoGLLazyElement
}

/*!
  Update GL state. Use this is you only want to modify the
  two-side lighting feature.
*/

void
SoGLShapeHintsElement::forceSend(SoState * const state,
                                 const SbBool twoside)
{
  SoGLLazyElement::sendTwosideLighting(state, twoside);
}

/*!
  Update GL state. Use this if you don't care about the two-side
  lighting state.
*/

void
SoGLShapeHintsElement::forceSend(SoState * const state,
                                 const SbBool ccw, const SbBool cull)
{
  SoGLLazyElement::sendVertexOrdering(state, ccw ? SoLazyElement::CCW : SoLazyElement::CW);
  SoGLLazyElement::sendBackfaceCulling(state, cull);
}

//! FIXME: write doc.

void
SoGLShapeHintsElement::forceSend(SoState * const state,
                                 const SbBool ccw, const SbBool cull,
                                 const SbBool twoside)
{
  SoGLLazyElement::sendVertexOrdering(state, ccw ? SoLazyElement::CCW : SoLazyElement::CW);
  SoGLLazyElement::sendBackfaceCulling(state, cull);
  SoGLLazyElement::sendTwosideLighting(state, twoside);
}
