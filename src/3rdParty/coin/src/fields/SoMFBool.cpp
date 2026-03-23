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
  \class SoMFBool SoMFBool.h Inventor/fields/SoMFBool.h
  \brief The SoMFBool class is a container for SbBool values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store multiple boolean on/off or TRUE/FALSE values.

  This field supports application data sharing through a
  setValuesPointer() method. See SoMField documentation for
  information on how to use this function.

  \sa SoSFBool
*/

// *************************************************************************

#include <Inventor/fields/SoMFBool.h>

#include <cassert>

#include <Inventor/errors/SoDebugError.h>

#include "fields/shared.h"
#include "fields/SoSubFieldP.h"

// *************************************************************************

SO_MFIELD_SOURCE_MALLOC(SoMFBool, SbBool, SbBool);

SO_MFIELD_SETVALUESPOINTER_SOURCE(SoMFBool, SbBool, SbBool);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFBool::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFBool);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFBool::read1Value(SoInput * in, int idx)
{
  assert(idx < this->maxNum);
  SbBool val;
  if (!sosfbool_read_value(in, val)) return FALSE;
  this->values[idx] = val;
  return TRUE;
}

void
SoMFBool::write1Value(SoOutput * out, int idx) const
{
  sosfbool_write_value(out, (*this)[idx]);
}

#endif // DOXYGEN_SKIP_THIS


// Don't store the default single value on each line for ASCII format
// export.
int
SoMFBool::getNumValuesPerLine(void) const
{
  return 8;
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFBool field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

BOOST_AUTO_TEST_CASE(array_ops)
{
  SoMFBool field;
  field.set1Value(0, TRUE);
  field.set1Value(1, FALSE);
  field.set1Value(2, TRUE);
  BOOST_CHECK_EQUAL(field.getNum(), 3);
  field.deleteValues(1,1);
  BOOST_CHECK_EQUAL(field.getNum(), 2);
  BOOST_CHECK_EQUAL(field[0], TRUE);
  BOOST_CHECK_EQUAL(field[1], TRUE);
}

#endif // COIN_TEST_SUITE
