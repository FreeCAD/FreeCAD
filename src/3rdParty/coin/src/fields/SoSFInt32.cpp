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
  \class SoSFInt32 SoSFInt32.h Inventor/fields/SoSFInt32.h
  \brief The SoSFInt32 class is a container for a 32-bit integer value.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a single 32-bit integer value.

  \sa SoMFInt32
*/

// *************************************************************************

#include <Inventor/fields/SoSFInt32.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFInt32, int32_t, int32_t);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFInt32::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFInt32);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFInt32::readValue(SoInput * in)
{
  int tmp;
  if (!in->read(tmp)) return FALSE;
  this->value = tmp;
  return TRUE;
}

// Write integer value to output stream. Also used from SoMFInt32
// class.
void
sosfint32_write_value(SoOutput * out, int32_t val)
{
  out->write(static_cast<int>(val));
}

void
SoSFInt32::writeValue(SoOutput * out) const
{
  sosfint32_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFInt32 field;
  BOOST_CHECK_MESSAGE(SoSFInt32::getClassTypeId() != SoType::badType(),
                      "SoSFInt32 class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
