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
  \class SoSFVec3d SoSFVec3d.h Inventor/fields/SoSFVec3d.h
  \brief The SoSFVec3d class is a container for an SbVec3d vector.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a single vector with three elements.

  \sa SbVec3d, SoMFVec3d
*/

// *************************************************************************

#include <Inventor/fields/SoSFVec3d.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFVec3d, SbVec3d, const SbVec3d &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFVec3d::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFVec3d);
}

// *************************************************************************

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFVec3d::readValue(SoInput * in)
{
  SbVec3d v;
  if (!sosfvec3d_read_value(in, v)) return FALSE;
  this->setValue(v);
  return TRUE;
}

void
SoSFVec3d::writeValue(SoOutput * out) const
{
  sosfvec3d_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS

/*!
  Set value of vector.
*/
void
SoSFVec3d::setValue(const double x, const double y, const double z)
{
  this->setValue(SbVec3d(x, y, z));
}

/*!
  Set value of vector.
*/
void
SoSFVec3d::setValue(const double xyz[3])
{
  this->setValue(SbVec3d(xyz));
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFVec3d field;
  BOOST_CHECK_MESSAGE(SoSFVec3d::getClassTypeId() != SoType::badType(),
                      "SoSFVec3d class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
