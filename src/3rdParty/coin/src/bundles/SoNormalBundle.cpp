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
  \class SoNormalBundle SoNormalBundle.h Inventor/bundles/SoNormalBundle.h
  \brief The SoNormalBundle class simplifies normal handling.

  \ingroup coin_bundles

  This class is currently not used in Coin but is provided for
  API compatibility.
*/

#include <Inventor/bundles/SoNormalBundle.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <cassert>

#include "SbBasicP.h"

/*!
  Constructor.
*/
SoNormalBundle::SoNormalBundle(SoAction * action, SbBool forrendering)
  : SoBundle(action)
{
  this->state->push();
  this->node = action->getCurPathTail();
  this->generator = NULL;
  this->elem = SoNormalElement::getInstance(this->state);
  this->glelem = NULL;
  if (forrendering) {
    this->glelem = static_cast<const SoGLNormalElement *>(this->elem);
  }
}

/*!
  Destructor.
*/
SoNormalBundle::~SoNormalBundle()
{
  delete this->generator;
  this->state->pop();
}

/*!
  Returns FALSE if there are normals on the state. Otherwise
  initGenerator() is called with \a numneeded as argument,
  and TRUE is returned.
*/
SbBool 
SoNormalBundle::shouldGenerate(int numneeded)
{
  if (this->elem->getNum()) return FALSE;
  this->initGenerator(numneeded);
  return TRUE;
}

/*!
  Initializes the normal generator. \a initnum is a hint that
  should contain the approximate number of normals to be generated.
*/
void
SoNormalBundle::initGenerator(int initnum)
{
  delete this->generator;
  SbBool ccw = SoShapeHintsElement::getVertexOrdering(this->state)
    != SoShapeHintsElement::CLOCKWISE;
  this->generator = new SoNormalGenerator(ccw, initnum);
}

/*!
  Start polygon specification. Call polygonVertex() for each
  vertex in the polygon, and then endPolygon() to close the
  polygon.

  \sa polygonVertex(), endPolygon()
*/
void 
SoNormalBundle::beginPolygon(void)
{
  this->generator->beginPolygon();
}

/*!
  Call for each vertex in a polygon.
  \sa beginPolygon(), endPolygon()
*/
void 
SoNormalBundle::polygonVertex(const SbVec3f & v)
{
  this->generator->polygonVertex(v);
}

/*!
  Call to close a polygon.

  \sa beginPolygon(), polygonVertex()
*/
void 
SoNormalBundle::endPolygon(void)
{
  this->generator->endPolygon();
}

/*!
  Convenience method to specify a triangle. \a p1, \a p2, and \a p3
  are the triangles vertices.
*/
void 
SoNormalBundle::triangle(const SbVec3f & p1,
                         const SbVec3f & p2,
                         const SbVec3f & p3)
{
  this->generator->triangle(p1, p2, p3);
}

/*!
  Generate normals for the shape. \a startindex should always
  be 0 (the SoNonIndexedShape::startIndex field is obsoleted).
  \a addtostate should be true if the generated normals should be 
  pushed onto the current state.
*/
void 
SoNormalBundle::generate(int startindex, SbBool addtostate)
{
  // we don't support startindex != 0 
  // The SoNonIndexedShape::startIndex field has been obsoleted by
  // SGI so this is probably ok.
  assert(startindex == 0);
  this->generator->generate(SoCreaseAngleElement::get(this->state));

  if (addtostate) {
    this->set(this->generator->getNumNormals(),
              this->generator->getNormals());
  }
}

/*!
  Returns the number of generated normals.
*/
const SbVec3f * 
SoNormalBundle::getGeneratedNormals(void) const
{
  if (this->generator) {
    return this->generator->getNormals();
  }
  return NULL;
}

/*!
  Returns a pointer to the generated normals.
*/
int 
SoNormalBundle::getNumGeneratedNormals(void) const
{
  if (this->generator) {
    return this->generator->getNumNormals();
  }
  return 0;
}

/*!
  Can be used by nodes that generate their own normals. The state will
  be updated with the new normals, and the state will be popped again
  when the SoNormalBundle destructor is called.
*/
void 
SoNormalBundle::set(int32_t num, const SbVec3f * normals)
{
  SoNormalElement::set(state, this->node, num, normals);
  // refetch element since we pushed
  this->elem = SoNormalElement::getInstance(this->state);
  if (this->glelem) {
    this->glelem = static_cast<const SoGLNormalElement *>(this->elem);
  }
}

/*!
  Returns the \a index'th normal from the state.
*/
const SbVec3f & 
SoNormalBundle::get(int index) const
{
  return this->elem->get(index);
}

/*!
  Send the index'th normal to OpenGL.
*/
void 
SoNormalBundle::send(int index) const
{
  assert(this->glelem);
  this->glelem->send(index);
}
