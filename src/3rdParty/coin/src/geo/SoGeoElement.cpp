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
  \class SoGeoElement SoGeoElement.h Inventor/elements/SoGeoElement.h
  \brief The SoGeoElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  \since Coin 2.5
*/

#include <Inventor/elements/SoGeoElement.h>
#include <Inventor/nodes/SoGeoOrigin.h>
#include <cassert>


class SoGeoElementP {
public:
  SoGeoOrigin * origin;
};

#define PRIVATE(obj) obj->pimpl

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoGeoElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGeoElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGeoElement, inherited);
}

/*!
  Constructor.
*/

SoGeoElement::SoGeoElement(void)
{
  PRIVATE(this) = new SoGeoElementP;
  PRIVATE(this)->origin = NULL;

  this->setTypeId(SoGeoElement::classTypeId);
  this->setStackIndex(SoGeoElement::classStackIndex);
}

/*!
  Destructor.
*/

SoGeoElement::~SoGeoElement()
{
  delete PRIVATE(this);
}

//! FIXME: write doc.

void
SoGeoElement::set(SoState * const state,
                  SoGeoOrigin * origin)
{
  SoGeoElement * element = (SoGeoElement *)
    SoReplacedElement::getElement(state, classStackIndex, origin);

  if (element) {
    element->setElt(origin);
  }
}

//! FIXME: write doc.

SoGeoOrigin *
SoGeoElement::get(SoState * const state)
{
  SoGeoElement * element = (SoGeoElement *)
    SoElement::getConstElement(state, classStackIndex);

  return PRIVATE(element)->origin;
}

// Doc from superclass

void
SoGeoElement::init(SoState * state)
{
  inherited::init(state);
  PRIVATE(this)->origin = NULL;
}

//! FIXME: doc
void
SoGeoElement::setElt(SoGeoOrigin * origin)
{
  PRIVATE(this)->origin = origin;
}

#undef PRIVATE

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoGeoElement::getClassStackIndex() != -1,
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
