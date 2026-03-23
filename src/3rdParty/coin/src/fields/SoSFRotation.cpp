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
  \class SoSFRotation SoSFRotation.h Inventor/fields/SoSFRotation.h
  \brief The SoSFRotation class is a container for an SbRotation.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a single rotation definition.

  Fields of this type stores their value to file as a rotation axis
  vector plus a rotation angle: "axis0 axis1 axis2 angle".


  Note that there is one \e very common mistake that is easy to make
  when setting the value of an SoSFRotation field, and that is to
  inadvertently use the wrong SbRotation constructor. This example
  should clarify the problem:

  \code
  mytransformnode->rotation.setValue(0, 0, 1, 1.5707963f);
  \endcode

  The programmer clearly tries to set a PI/2 rotation around the Z
  axis, but this will fail, as the SbRotation constructor invoked
  above is the one that takes as arguments the 4 floats of a \e
  quaternion. What the programmer almost certainly wanted to do was to
  use the SbRotation constructor that takes a rotation vector and a
  rotation angle, which is invoked like this:

  \code
  mytransformnode->rotation.setValue(SbVec3f(0, 0, 1), 1.5707963f);
  \endcode

  \sa SoMFRotation
*/

// *************************************************************************

#include <Inventor/fields/SoSFRotation.h>

#include <Inventor/SbVec3f.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFRotation, SbRotation, const SbRotation &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFRotation::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFRotation);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFRotation::readValue(SoInput * in)
{
  SbRotation r;
  if (!sosfrotation_read_value(in, r)) return FALSE;
  this->setValue(r);
  return TRUE;
}

void
SoSFRotation::writeValue(SoOutput * out) const
{
  sosfrotation_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Return value of rotation as an \a axis and an \a angle around this
  \a axis.
*/
void
SoSFRotation::getValue(SbVec3f & axis, float & angle) const
{
  this->getValue().getValue(axis, angle);
}

/*!
  Set the rotation from a set of quaternion values.
*/
void
SoSFRotation::setValue(const float q0, const float q1,
                       const float q2, const float q3)
{
  this->setValue(SbRotation(q0, q1, q2, q3));
}

/*!
  Set the rotation from a set of quaternion values.
*/
void
SoSFRotation::setValue(const float q[4])
{
  this->setValue(SbRotation(q));
}

/*!
  Set the rotation from an \a axis and an \a angle around
  this \a axis.
*/
void
SoSFRotation::setValue(const SbVec3f & axis, const float angle)
{
  this->setValue(SbRotation(axis, angle));
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFRotation field;
  BOOST_CHECK_MESSAGE(SoSFRotation::getClassTypeId() != SoType::badType(),
                      "SoSFRotation class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
