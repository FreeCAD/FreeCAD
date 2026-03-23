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
  \class SoMFName SoMFName.h Inventor/fields/SoMFName.h
  \brief The SoMFName class is a container for SbName values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  needs to store arrays of names.

  \sa SoSFName
*/

// *************************************************************************

#include <Inventor/fields/SoMFName.h>

#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFName, SbName, const SbName &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFName::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFName);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

// This is implemented in the SoSFName class.
extern void sosfname_write_value(SoOutput * out, const SbName & val);

SbBool
SoMFName::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);

  // Reading as SbString instead of as SbName, because the semantics
  // of SoInput::read(SbName&) is to read token identifiers, such as
  // node or field names, and doesn't e.g. handle quotes as expected
  // for a "free-form" string.
  SbString s;
  SbBool ok = in->read(s);
  if (!ok) return FALSE;
  this->values[idx] = s;
  return TRUE;
}

void
SoMFName::write1Value(SoOutput * out, int idx) const
{
  sosfname_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set \a num \a strings from index \a start in this multiple-value
  field instance.
*/
void
SoMFName::setValues(const int start, const int numarg, const char * strings[])
{
  if(start+numarg > this->maxNum) this->allocValues(start+numarg);
  else if(start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[i+start] = SbName(strings[i]);
  this->valueChanged();
}

/*!
  Set this field to contain only a single name, given by \a str.
*/
void
SoMFName::setValue(const char * str)
{
  this->setValue(SbName(str));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFName field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
