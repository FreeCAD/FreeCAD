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
  \class SoSFBox2f SoSFBox2f.h Inventor/fields/SoSFBox2f.h
  \brief The SoSFBox2f class is a container for an SbBox2f vector.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a box.

  \COIN_CLASS_EXTENSION
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/fields/SoSFBox2f.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFBox2f, SbBox2f, const SbBox2f &);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFBox2f::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFBox2f);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS


SbBool
SoSFBox2f::readValue(SoInput * in)
{
  float min[2];
  float max[2];
  if (!in->read(min[0]) ||
      !in->read(min[1]) ||
      !in->read(max[0]) ||
      !in->read(max[1])) {
    SoReadError::post(in, "Couldn't read SoSFBox2f");
    return FALSE;
  }
  this->setValue(min[0], min[1], max[0], max[1]);
  return TRUE;
}

void
SoSFBox2f::writeValue(SoOutput * out) const
{
  float min[2];
  float max[2];
  SbBox2f b = this->getValue();
  b.getBounds(min[0], min[1], max[0], max[1]);

  out->write(min[0]);
  if (!out->isBinary()) out->write(' ');
  out->write(min[1]);
  if (!out->isBinary()) out->write(' ');
  out->write(max[0]);
  if (!out->isBinary()) out->write(' ');
  out->write(max[1]);
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

/*!
  Set value of vector.
*/
void
SoSFBox2f::setValue(float xmin, float ymin, float xmax, float ymax)
{
  this->setValue(SbBox2f(xmin, ymin, xmax, ymax));
}


/*!
  Set value of vector.
*/
void
SoSFBox2f::setValue(const SbVec2f & minvec, const SbVec2f & maxvec)
{
  this->setValue(SbBox2f(minvec, maxvec));
}


/*!
  Set value of vector.
*/
void
SoSFBox2f::getValue(SbBox2f & box) const
{
  box = value;
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFBox2f field;
  BOOST_CHECK_MESSAGE(SoSFBox2f::getClassTypeId() != SoType::badType(),
                      "SoSFBox2f class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
