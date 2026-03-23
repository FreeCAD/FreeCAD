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
  \class SoSFVec4ub SoSFVec4ub.h Inventor/fields/SoSFVec4ub.h
  \brief The SoSFVec4ub class is a container for an SbVec4ub vector.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a single vector with four elements.

  \sa SbVec4ub, SoMFVec4ub
  \COIN_CLASS_EXTENSION
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/fields/SoSFVec4ub.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFVec4ub, SbVec4ub, SbVec4ub);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFVec4ub::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFVec4ub);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFVec4ub::readValue(SoInput * in)
{
  return
    in->readByte(this->value[0]) &&
    in->readByte(this->value[1]) &&
    in->readByte(this->value[2]) &&
    in->readByte(this->value[3]);
}

void
SoSFVec4ub::writeValue(SoOutput * out) const
{
  sosfvec4ub_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS


/*!
  Set value of vector.
*/
void
SoSFVec4ub::setValue(uint8_t x, uint8_t y, uint8_t z, uint8_t w)
{
  this->setValue(SbVec4ub(x, y, z, w));
}

/*!
  Set value of vector.
*/
void
SoSFVec4ub::setValue(const uint8_t xyzw[4])
{
  this->setValue(SbVec4ub(xyzw));
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoSFVec4ub::getClassTypeId() != SoType::badType(),
                      "SoSFVec4ub class not initialized");
  SoSFVec4ub field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "SoSFVec4ub object wrongly initialized");
  // no default value initialization to test
  field.setValue(1, 2, 3, 4);
  BOOST_CHECK_EQUAL(field.getValue(), SbVec4ub(1, 2, 3, 4));
}

BOOST_AUTO_TEST_CASE(textinput)
{
  TestSuite::ResetReadErrorCount();
  SbBool ok;
  SoSFVec4ub field;
  ok = field.set("1 2 3 4");
  BOOST_CHECK_MESSAGE(ok == TRUE, "SoSFVec4ub read error");
  BOOST_CHECK_EQUAL(field.getValue(), SbVec4ub(1, 2, 3, 4));
  BOOST_CHECK_EQUAL(TestSuite::GetReadErrorCount(), 0);
  TestSuite::ResetReadErrorCount();
}

#endif // COIN_TEST_SUITE
