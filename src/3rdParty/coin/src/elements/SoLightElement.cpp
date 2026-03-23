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
  \class SoLightElement Inventor/elements/SoLightElement.h
  \brief The SoLightElement class manages the currently active light sources.

  \ingroup coin_elements
*/

#include "SbBasicP.h"

#include <Inventor/elements/SoLightElement.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/lists/SbList.h>
#include <cassert>

/*!
  \fn SoLightElement::lights
  List of current light nodes.
*/

/*!
  \fn SoLightElement::matrixlist

  List of matrices to map from world coordinates to view reference
  coordinates. To avoid getting a hugs element (sizeof), this
  list is only allocated in the bottom element, and the pointer
  to this list is passed along to the other elements.
*/


// need a custom constructor to disable refcounting in node list
SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoLightElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLightElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoLightElement, inherited);
}

/*!
  Constructor.
*/
SoLightElement::SoLightElement(void)
{
  this->setTypeId(SoLightElement::classTypeId);
  this->setStackIndex(SoLightElement::classStackIndex);
  // this is safe since a node should never be deleted while it's
  // active during a traversal
  this->lights.addReferences(FALSE);
}

/*!
  Destructor.
*/
SoLightElement::~SoLightElement(void)
{
  if (this->didalloc.state) delete this->matrixlist;
}


/*!
  Adds \a light to the list of active lights. \a matrix should
  transform the light from the world coordinate system to
  the view reference coordinate system.
*/
void
SoLightElement::add(SoState * const state, SoLight * const light,
                    const SbMatrix & matrix)
{
  SoLightElement * elem =
    coin_safe_cast<SoLightElement*>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    int i = elem->lights.getLength();
    elem->lights.append(light);
    elem->addNodeId(light);
    if (i >= elem->matrixlist->getLength())
      elem->matrixlist->append(matrix);
    else
      (*elem->matrixlist)[i] = matrix;
  }
}

/*!
  Returns the list of light nodes.
*/
const SoNodeList &
SoLightElement::getLights(SoState * const state)
{
  const SoLightElement * elem = coin_assert_cast<const SoLightElement *>
    (
    SoElement::getConstElement(state, classStackIndex)
    );
  return elem->lights;
}

/*!
  Get matrix which transforms light \a index from the world
  coordinate system to the view reference system.
*/
const SbMatrix &
SoLightElement::getMatrix(SoState * const state, const int index)
{
  const SoLightElement * elem = coin_assert_cast<const SoLightElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  assert(index >= 0 && index < elem->matrixlist->getLength());
  return elem->matrixlist->getArrayPtr()[index];
}

// doc from parent
void
SoLightElement::init(SoState * state)
{
  inherited::init(state);
  this->matrixlist = new SbList <SbMatrix>;
  this->didalloc.state = TRUE;
}

// Documented in superclass. Overridden to copy lights to the new top
// of stack. Also copies node ids.
void
SoLightElement::push(SoState * state)
{
  inherited::push(state);

  SoLightElement * const prev =
    coin_assert_cast<SoLightElement *>
    (
     this->getNextInStack()
     );
  this->lights.truncate(0);
  const int numLights = prev->lights.getLength();
  int i;
  for (i = 0; i < numLights; i++)
    this->lights.append(prev->lights[ i ]);
  this->matrixlist = prev->matrixlist; // just pass pointer to list
  this->copyNodeIds(prev);
}
