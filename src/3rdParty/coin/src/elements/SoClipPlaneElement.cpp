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
  \class SoClipPlaneElement Inventor/elements/SoClipPlaneElement.h
  \brief The SoClipPlaneElement class is used to manage the clip plane stack.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/nodes/SoNode.h>

#include "SbBasicP.h"

//
// constructor for the internal class
//
SoClipPlaneElement::so_plane_data::so_plane_data(const SbPlane &planeref, const SbMatrix &matrix)
{
  this->plane = this->wcPlane = planeref;
  this->wcPlane.transform(matrix);
}

/*!
  \fn SoClipPlaneElement::planes
  List of currently active planes.
*/

/*!
  \fn SoClipPlaneElement::startIndex
  Index of first clip plane in this element. Used to disable clip planes
  in SoGLClipPlaneElement::pop().
*/

SO_ELEMENT_SOURCE(SoClipPlaneElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoClipPlaneElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoClipPlaneElement, inherited);
}

/*!
  Destructor.
*/
SoClipPlaneElement::~SoClipPlaneElement()
{
}

/*!
  Adds \a plane as an active plane. Calls addToElt() to do the job.
*/
void
SoClipPlaneElement::add(SoState * const state,
                        SoNode * const node,
                        const SbPlane & plane)
{
  SoClipPlaneElement * element =
    coin_safe_cast<SoClipPlaneElement * >
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (element) {
    element->addToElt(plane, SoModelMatrixElement::get(state));
    if (node) element->addNodeId(node);
  }
}

/*!
  Returns the current (top-of-stack) element.
*/
const SoClipPlaneElement *
SoClipPlaneElement::getInstance(SoState * const state)
{
  return coin_assert_cast<const SoClipPlaneElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
}

/*!
  Returns the current number of active clipping planes.
*/
int
SoClipPlaneElement::getNum() const
{
  return this->planes.getLength();
}

/*!
  Returns the \a index'th plane.
*/
const SbPlane &
SoClipPlaneElement::get(const int index,
                        const SbBool inworldspace) const
{
  assert(index >= 0 && index < this->planes.getLength());
  if (inworldspace) return this->planes.getArrayPtr()[index].wcPlane;
  return this->planes.getArrayPtr()[index].plane;
}

/*!
  This method adds the clipping plane, \a plane, to an instance.
  \a modelmatrix is the current model matrix.
*/
void
SoClipPlaneElement::addToElt(const SbPlane &plane,
                             const SbMatrix &modelMatrix)
{
  SoClipPlaneElement::so_plane_data data(plane, modelMatrix);
  this->planes.append(data);
}

// doc from parent
void
SoClipPlaneElement::init(SoState * state)
{
  inherited::init(state);
  this->planes.truncate(0);
  this->startIndex = 0;
}

// Documented in superclass. Overridden to copy planes into the new
// top of stack, since planes are accumulated. Also copies accumulated
// node ids.
void
SoClipPlaneElement::push(SoState * state)
{
  inherited::push(state);

  SoClipPlaneElement * const prev =
    coin_assert_cast<SoClipPlaneElement *>(this->getNextInStack());

  this->planes.truncate(0);
  for (int i = 0; i < prev->planes.getLength(); i++) {
    this->planes.append(prev->planes[i]);
  }
  this->startIndex = prev->planes.getLength();
  this->copyNodeIds(prev);
}
