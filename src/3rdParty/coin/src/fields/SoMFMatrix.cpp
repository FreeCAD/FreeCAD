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
  \class SoMFMatrix SoMFMatrix.h Inventor/fields/SoMFMatrix.h
  \brief The SoMFMatrix class is a container for SbMatrix values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  needs to store matrices.

  \sa SoSFMatrix
*/

// *************************************************************************

#include <Inventor/fields/SoMFMatrix.h>

#include <cassert>

#include <Inventor/SoOutput.h>
#include <Inventor/SoInput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFMatrix, SbMatrix, const SbMatrix &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFMatrix::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFMatrix);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFMatrix::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);

  float * ptr = this->values[idx][0];
  for (int i = 0; i < 16; i++) {
    if (!in->read(ptr[i])) return FALSE;
  }
  return TRUE;
}

void
SoMFMatrix::write1Value(SoOutput * out, int idx) const
{
  out->incrementIndent();
  if (idx == 0) out->incrementIndent();
  sosfmatrix_write_value(out, (*this)[idx]);
  if (idx == 0) out->decrementIndent();
  out->decrementIndent();
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set field value array to a single matrix with the given components.
*/
void
SoMFMatrix::setValue(const float a11, const float a12,
                     const float a13, const float a14,
                     const float a21, const float a22,
                     const float a23, const float a24,
                     const float a31, const float a32,
                     const float a33, const float a34,
                     const float a41, const float a42,
                     const float a43, const float a44)
{
  this->setValue(SbMatrix(a11,a12,a13,a14, a21,a22,a23,a24,
                          a31,a32,a33,a34, a41,a42,a43,a44));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFMatrix field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
