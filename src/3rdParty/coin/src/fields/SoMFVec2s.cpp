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
  \class SoMFVec2s SoMFVec2s.h Inventor/fields/SoMFVec2s.h
  \brief The SoMFVec2s class is a container for SbVec2s vectors.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store an array of vectors with two elements.

  This field supports application data sharing through a
  setValuesPointer() method. See SoMField documentation for
  information on how to use this function.

  \sa SbVec2s, SoSFVec2s
  \COIN_CLASS_EXTENSION
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/fields/SoMFVec2s.h>

#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFVec2s, SbVec2s, SbVec2s);

SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFVec2s, SbVec2s, SbVec2s);
SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFVec2s, SbVec2s, short);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFVec2s::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFVec2s);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFVec2s::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);
  return
    in->read(this->values[idx][0]) &&
    in->read(this->values[idx][1]);
}

void
SoMFVec2s::write1Value(SoOutput * out, int idx) const
{
  sosfvec2s_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set \a num vector array elements from \a xy, starting at index
  \a start.
*/
void
SoMFVec2s::setValues(int start, int numarg, const short xy[][2])
{
  if (start+numarg > this->maxNum) this->allocValues(start+numarg);
  else if (start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[start+i] = SbVec2s(xy[i]);
  this->valueChanged();
}

/*!
  Set the vector at \a idx.
*/
void
SoMFVec2s::set1Value(int idx, short x, short y)
{
  this->set1Value(idx, SbVec2s(x, y));
}

/*!
  Set the vector at \a idx.
*/
void
SoMFVec2s::set1Value(int idx, const short xy[2])
{
  this->set1Value(idx, SbVec2s(xy));
}

/*!
  Set this field to contain a single vector with the given
  element values.
*/
void
SoMFVec2s::setValue(short x, short y)
{
  this->setValue(SbVec2s(x, y));
}

/*!
  Set this field to contain a single vector with the given
  element values.
*/
void
SoMFVec2s::setValue(const short xy[2])
{
  if (xy == NULL) this->setNum(0);
  else this->setValue(SbVec2s(xy));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFVec2s field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
