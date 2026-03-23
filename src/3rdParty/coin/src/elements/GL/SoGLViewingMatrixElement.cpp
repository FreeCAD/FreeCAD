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
  \class SoGLViewingMatrixElement Inventor/elements/SoGLViewingMatrixElement.h
  \brief The SoGLViewingMatrixElement class is used to store the current viewing matrix.

  \ingroup coin_elements

  The viewing matrix contains the inverse camera coordinate system
  matrix. The camera coordinate system is built from the field values
  in the current SoCamera (currently either SoPerspectiveCamera or
  SoOrthographicCamera) and any transformations prior to the camera in
  the scene graph.
*/

#include "coindefs.h"
#include <Inventor/elements/SoGLViewingMatrixElement.h>
#include <Inventor/actions/SoGLRenderAction.h>


#include <Inventor/elements/SoModelMatrixElement.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>
#include "elements/GL/SoResetMatrixElement.h"

SO_ELEMENT_SOURCE(SoGLViewingMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLViewingMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLViewingMatrixElement, inherited);
  SoResetMatrixElement::initClass();
}

/*!
  Destructor.
*/
SoGLViewingMatrixElement::~SoGLViewingMatrixElement(void)
{
}

// doc in parent
void
SoGLViewingMatrixElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr;
  this->mmidentity = TRUE;
}

// doc in parent
void
SoGLViewingMatrixElement::push(SoState * stateptr)
{
  inherited::push(stateptr);
  this->state = stateptr;
}

// doc in parent
void
SoGLViewingMatrixElement::pop(SoState * stateptr,
                              const SoElement * COIN_UNUSED_ARG(prevTopElement))
{
  this->capture(stateptr);
  this->updategl();
}

/*!
  Returns the node id of the current camera node. This is used by
  SoGLModelMatrixElement to detect a change in the viewing matrix
  inside an SoTransformSeparator node.
*/
SbUniqueId
SoGLViewingMatrixElement::getNodeId(SoState * const state)
{
  const SoReplacedElement *elem = (const SoReplacedElement*)
    SoElement::getConstElement(state, classStackIndex);
  return elem->getNodeId();
}

/*!
  Sets the current viewing matrix.
*/
void
SoGLViewingMatrixElement::setElt(const SbMatrix & matrix)
{
  inherited::setElt(matrix);
  this->modelmatrix = SoModelMatrixElement::get(this->state, this->mmidentity);
  if (this->state->isElementEnabled(SoResetMatrixElement::getClassStackIndex())) {
    SbMatrix mat = this->viewingMatrix;
    if (!this->mmidentity) {
      mat.multRight(this->modelmatrix);
      mat.multLeft(this->modelmatrix.inverse());
    }
    SoResetMatrixElement::set(this->state, mat);
  }
  this->updategl();
}

// sends the current viewing matrix to GL. Eliminates the model matrix
// part of the matrix, since a GL matrix is always located in 0 0 0.
void
SoGLViewingMatrixElement::updategl(void)
{
  SbMatrix mat = this->viewingMatrix;
  if (!this->mmidentity) {
    mat.multRight(this->modelmatrix);
  }
  glLoadMatrixf((float*)mat);
}

/*!
  Returns the matrix that should be used by SoGLModelMatrixElement
  when the transformation should be reset during traversal (typically
  when encountering an SoResetTransform node). It is not sufficient to
  simply load the viewing matrix, since the transformations that were
  applied before the camera needs to be accounted for.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SbMatrix
SoGLViewingMatrixElement::getResetMatrix(SoState * state)
{
  if (state->isElementEnabled(SoResetMatrixElement::getClassStackIndex())) {
    return SoResetMatrixElement::get(state);
  }
  const SoGLViewingMatrixElement * element = (const SoGLViewingMatrixElement *)
    SoElement::getConstElement(state, classStackIndex);

  SbMatrix mat = element->viewingMatrix;
  if (!element->mmidentity) {
    // first eliminate model matrix part of matrix
    mat.multRight(element->modelmatrix);
    // then move geometry to account for the transformations prior to
    // the camera
    mat.multLeft(element->modelmatrix.inverse());
  }
  return mat;
}
