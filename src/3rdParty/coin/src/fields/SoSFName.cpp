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
  \class SoSFName SoSFName.h Inventor/fields/SoSFName.h
  \brief The SoSFName class is a container for an SbName.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a single name string.

  Fields of this type stores their value to file as a the name string
  within quotes.

  \sa SoMFName
*/

// *************************************************************************

#include <Inventor/fields/SoSFName.h>

#include <Inventor/fields/SoSFString.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFName, SbName, const SbName &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFName::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFName);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFName::readValue(SoInput * in)
{
  // Reading as SbString instead of as SbName, because the semantics
  // of SoInput::read(SbName&) is to read token identifiers, such as
  // node or field names, and doesn't e.g. handle quotes as expected
  // for a "free-form" string.
  SbString s;
  SbBool ok = in->read(s);
  if (!ok) return FALSE;
  this->value = s;
  return TRUE;
}

// Write SbName value to output stream. Also used from SoMFName class.
void
sosfname_write_value(SoOutput * out, const SbName & val)
{
  out->write(val);
}

void
SoSFName::writeValue(SoOutput * out) const
{
  sosfname_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set value of field.
*/
void
SoSFName::setValue(const char * const name)
{
  this->setValue(SbName(name));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFName field;
  BOOST_CHECK_MESSAGE(SoSFName::getClassTypeId() != SoType::badType(),
                      "SoSFName class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
