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
  \class SoMFRotation SoMFRotation.h Inventor/fields/SoMFRotation.h
  \brief The SoMFRotation class is a container for SbRotation values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store multiple rotation definitions.

  \sa SoSFRotation
*/

// *************************************************************************

#include <Inventor/fields/SoMFRotation.h>

#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_MFIELD_SOURCE(SoMFRotation, SbRotation, const SbRotation &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFRotation::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFRotation);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFRotation::read1Value(SoInput * in, int idx)
{
  SbRotation r;
  if (!sosfrotation_read_value(in, r)) return FALSE;
  this->set1Value(idx, r);
  return TRUE;
}

void
SoMFRotation::write1Value(SoOutput * out, int idx) const
{
  sosfrotation_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set the values of \a num rotations, starting from index \a start,
  using the quaternion quadruples from \a q.
*/
void
SoMFRotation::setValues(const int start, const int numarg, const float q[][4])
{
  if(start+numarg > this->maxNum) this->allocValues(start+numarg);
  else if(start+numarg > this->num) this->num = start+numarg;

  for(int i=0; i < numarg; i++) this->values[i+start].setValue(q[i]);
  this->valueChanged();
}

/*!
  Set the rotation at \a idx from the quaternion values.
*/
void
SoMFRotation::set1Value(const int idx, const float q0, const float q1,
                        const float q2, const float q3)
{
  this->set1Value(idx, SbRotation(q0, q1, q2, q3));
}

/*!
  Set the rotation at \a idx from the quaternion quadruple \a q.
*/
void
SoMFRotation::set1Value(const int idx, const float q[4])
{
  this->set1Value(idx, SbRotation(q));
}

/*!
  Set the rotation at \a idx from the rotation \a axis and \a angle.
*/
void
SoMFRotation::set1Value(const int idx, const SbVec3f & axis, const float angle)
{
  this->set1Value(idx, SbRotation(axis, angle));
}

/*!
  Set the field to a single rotation from the quaternion values.
*/
void
SoMFRotation::setValue(const float q0, const float q1,
                       const float q2, const float q3)
{
  this->setValue(SbRotation(q0, q1, q2, q3));
}

/*!
  Set the field to a single rotation from the quaternion quadruple \a q.
*/
void
SoMFRotation::setValue(const float q[4])
{
  this->setValue(SbRotation(q[0], q[1], q[2], q[3]));
}

/*!
  Set the field to a single rotation from the \a axis and \a angle.
*/
void
SoMFRotation::setValue(const SbVec3f & axis, const float angle)
{
  this->setValue(SbRotation(axis, angle));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFRotation field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
