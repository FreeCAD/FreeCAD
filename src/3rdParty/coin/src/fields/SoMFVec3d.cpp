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
  \class SoMFVec3d SoMFVec3d.h Inventor/fields/SoMFVec3d.h
  \brief The SoMFVec3d class is a container for SbVec3d vectors.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store an array of vectors with three elements.

  \sa SbVec3d, SoSFVec3d, SoMFVec3f
*/

// *************************************************************************

#include <Inventor/fields/SoMFVec3d.h>

#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFVec3d, SbVec3d, const SbVec3d &);

SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFVec3d, SbVec3d, SbVec3d);
SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFVec3d, SbVec3d, double);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFVec3d::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFVec3d);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFVec3d::read1Value(SoInput * in, int idx)
{
  SbVec3d v;
  if (!sosfvec3d_read_value(in, v)) return FALSE;
  this->set1Value(idx, v);
  return TRUE;
}

void
SoMFVec3d::write1Value(SoOutput * out, int idx) const
{
  sosfvec3d_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set \a num vector array elements from the \a xyz array, reading from
  index 0 in the given double-triplet vector array and storing values
  from index \a start in this SoMFVec3d's array.

  If \a start + \a num is larger than the current number of elements
  in the SoMFVec3d field, this method will automatically expand the
  field to accommodate all given double-vector element values.
*/
void
SoMFVec3d::setValues(int start, int numarg, const double xyz[][3])
{
  if (start+numarg > this->maxNum) this->allocValues(start+numarg);
  else if (start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[start+i] = SbVec3d(xyz[i]);
  this->valueChanged();
}

/*!
  Set the vector at \a idx.
*/
void
SoMFVec3d::set1Value(int idx, double x, double y, double z)
{
  this->set1Value(idx, SbVec3d(x, y, z));
}

/*!
  Set the vector at \a idx.
*/
void
SoMFVec3d::set1Value(int idx, const double xyz[3])
{
  this->set1Value(idx, SbVec3d(xyz));
}

/*!
  Set this field to contain a single vector with the given
  element values.
*/
void
SoMFVec3d::setValue(double x, double y, double z)
{
  this->setValue(SbVec3d(x, y, z));
}

/*!
  Set this field to contain a single vector with the given
  element values.
*/
void
SoMFVec3d::setValue(const double xyz[3])
{
  if (xyz == NULL) this->setNum(0);
  else this->setValue(SbVec3d(xyz));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFVec3d field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
