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
  \class SoCacheHintElement Inventor/elements/SoCacheHintElement.h
  \brief The SoCacheHintElement class is yet to be documented.

  \ingroup coin_elements

  Please note that this is an experimental class. The API might change
  a lot before/if it is included in any official Coin release.
*/

#include <Inventor/elements/SoCacheHintElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>

#include <cassert>

#include "SbBasicP.h"
#include "coindefs.h"

// FIXME: make it possible to control this constant. pederb, 2005-01-10
#define VERTEX_ARRAY_LIMIT 0.51f

class SoCacheHintElementP {
public:
  float memvalue;
  float gfxvalue;
};

#define PRIVATE(obj) obj->pimpl

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoCacheHintElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoCacheHintElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoCacheHintElement, inherited);
}

/*!
  Constructor.
*/

SoCacheHintElement::SoCacheHintElement(void)
{
  PRIVATE(this) = new SoCacheHintElementP;
  this->setTypeId(SoCacheHintElement::classTypeId);
  this->setStackIndex(SoCacheHintElement::classStackIndex);
}

/*!
  Destructor.
*/

SoCacheHintElement::~SoCacheHintElement(void)
{
  delete PRIVATE(this);
}

//! FIXME: write doc.

void
SoCacheHintElement::init(SoState * state)
{
  inherited::init(state);
  PRIVATE(this)->memvalue = 0.5f;
  PRIVATE(this)->gfxvalue = 0.5f;
}

//! FIXME: write doc.

void
SoCacheHintElement::push(SoState * state)
{
  inherited::push(state);
  SoCacheHintElement * prev = coin_assert_cast<SoCacheHintElement * >
    (
     this->getNextInStack()
     );
  PRIVATE(this)->memvalue = PRIVATE(prev)->memvalue;
  PRIVATE(this)->gfxvalue = PRIVATE(prev)->gfxvalue;
}

void
SoCacheHintElement::pop(SoState * state, const SoElement * prevtopelement)
{
  inherited::pop(state, prevtopelement);
}

//! FIXME: write doc.

SbBool
SoCacheHintElement::matches(const SoElement * element) const
{
  const SoCacheHintElement * elem = coin_assert_cast<const SoCacheHintElement *>(element);
  return
    (PRIVATE(this)->memvalue == PRIVATE(elem)->memvalue) &&
    (PRIVATE(this)->gfxvalue == PRIVATE(elem)->gfxvalue);
}

//! FIXME: write doc.

SoElement *
SoCacheHintElement::copyMatchInfo() const
{
  SoCacheHintElement * elem = static_cast<SoCacheHintElement *>(
    getTypeId().createInstance()
    );
  PRIVATE(elem)->memvalue = PRIVATE(this)->memvalue;
  PRIVATE(elem)->gfxvalue = PRIVATE(this)->gfxvalue;
  return elem;
}

//! FIXME: write doc.

void
SoCacheHintElement::set(SoState * state,
                        SoNode * COIN_UNUSED_ARG(node),
                        const float memvalue,
                        const float gfxvalue)
{
  SoCacheHintElement * elem =
    coin_assert_cast<SoCacheHintElement * >
    (
     SoElement::getElement(state, classStackIndex)
     );

  PRIVATE(elem)->memvalue = memvalue;
  PRIVATE(elem)->gfxvalue = gfxvalue;

  SoShapeStyleElement::setVertexArrayRendering(state, memvalue >= VERTEX_ARRAY_LIMIT);
}

//! FIXME: write doc.

void
SoCacheHintElement::get(SoState * const state, float & memvalue, float & gfxvalue)
{
  const SoCacheHintElement * elem = coin_assert_cast<const SoCacheHintElement *>(
    SoElement::getConstElement(state, classStackIndex)
    );

  memvalue = PRIVATE(elem)->memvalue;
  gfxvalue = PRIVATE(elem)->gfxvalue;
}

#undef PRIVATE
#undef VERTEX_ARRAY_LIMIT
