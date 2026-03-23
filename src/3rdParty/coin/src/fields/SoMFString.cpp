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
  \class SoMFString SoMFString.h Inventor/fields/SoMFString.h
  \brief The SoMFString class is a container for SbString values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  needs to store arrays of strings.

  \sa SoSFString

*/

// *************************************************************************

#include <Inventor/fields/SoMFString.h>

#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFString, SbString, const SbString &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFString::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFString);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFString::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);
  return in->read(this->values[idx]);
}

void
SoMFString::write1Value(SoOutput * out, int idx) const
{
  sosfstring_write_value(this, out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS

/*!
  Set field to contain \a num \a strings from index \a start,
  replacing any strings already present at the given indices.
*/
void
SoMFString::setValues(const int start, const int numarg, const char * strings[])
{
  if (start+numarg > this->maxNum) this->allocValues(start+numarg);
  else if (start+numarg > this->num) this->num = start+numarg;

  for (int i=0; i < numarg; i++) this->values[i+start] = SbString(strings[i]);
  this->valueChanged();
}

/*!
  Set field to contain a single string, from \a str.
*/
void
SoMFString::setValue(const char * str)
{
  this->setValue(SbString(str));
}

/*!
  Remove all text from \a fromchar on \a fromline and to \a tochar
  on \a toline, including all lines between \a fromline and \a toline.
  Merge \a fromline and \a toline after deletion.
*/
void
SoMFString::deleteText(const int fromline, const int fromchar,
                       const int toline, const int tochar)
{
#if COIN_DEBUG && 1 // debug
  if (fromline < 0 || toline >= this->getNum() || fromline > toline ||
      (fromline == toline && fromchar >= tochar) ||
      fromchar < 0 || fromchar >= (*this)[fromline].getLength() ||
      tochar < 0 || tochar >= (*this)[toline].getLength()) {
    SoDebugError::post("SoMFString::deleteText",
                       "invalid argument(s), [%d, %d - %d, %d]",
                       fromline, fromchar, toline, tochar);
    return;
  }
#endif // debug

  if (fromline == toline) {
    this->values[fromline].deleteSubString(fromchar, tochar);
  }
  else {
    this->values[fromline].deleteSubString(fromchar, -1);
    this->values[toline].deleteSubString(0, tochar);
    // Merge.
    this->values[fromline] += (*this)[toline];
    // Delete intermediate strings + toline string.
    this->deleteValues(fromline + 1, toline - fromline);
  }
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFString field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
