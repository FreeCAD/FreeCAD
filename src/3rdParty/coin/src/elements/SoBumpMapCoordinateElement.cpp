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
  \class SoBumpMapCoordinateElement Inventor/elements/SoBumpMapCoordinateElement.h
  \brief The SoBumpMapCoordinateElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoBumpMapCoordinateElement.h>

#include <cassert>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoNode.h>

#include "tidbitsp.h"
#include "SbBasicP.h"

/*!
  \fn SoBumpMapCoordinateElement::numcoords

  FIXME: write doc.
*/

/*!
  \fn SoBumpMapCoordinateElement::coords

  FIXME: write doc.
*/


SO_ELEMENT_SOURCE(SoBumpMapCoordinateElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoBumpMapCoordinateElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoBumpMapCoordinateElement, inherited);
}

/*!
  Destructor.
*/

SoBumpMapCoordinateElement::~SoBumpMapCoordinateElement(void)
{
}

//! FIXME: write doc.

void
SoBumpMapCoordinateElement::init(SoState * state)
{
  inherited::init(state);
  this->numcoords = 0;
  this->coords = NULL;
}

//! FIXME: write doc.

void
SoBumpMapCoordinateElement::set(SoState * state,
                                SoNode * node,
                                const int32_t numcoords,
                                const SbVec2f * coords)
{
  SoBumpMapCoordinateElement * elem = coin_assert_cast<SoBumpMapCoordinateElement *>
    (SoReplacedElement::getElement(state, classStackIndex, node));
  elem->coords = coords;
  elem->numcoords = numcoords;
}

//! FIXME: write doc.

const SbVec2f &
SoBumpMapCoordinateElement::get(const int index) const
{
  assert(index >= 0 && index < this->numcoords);
  return this->coords[index];
}


//! FIXME: write doc.

const SoBumpMapCoordinateElement *
SoBumpMapCoordinateElement::getInstance(SoState * state)
{
  return coin_assert_cast<const SoBumpMapCoordinateElement *>
    (getConstElement(state, classStackIndex));
}

//! FIXME: write doc.

int32_t
SoBumpMapCoordinateElement::getNum(void) const
{
  return this->numcoords;
}

//! FIXME: write doc.

const SbVec2f *
SoBumpMapCoordinateElement::getArrayPtr(void) const
{
  return this->coords;
}
