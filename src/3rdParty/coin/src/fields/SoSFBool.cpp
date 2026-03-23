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
  \class SoSFBool SoSFBool.h Inventor/fields/SoSFBool.h
  \brief The SoSFBool class is a container for an SbBool value.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store a boolean on/off or TRUE/FALSE value, like for
  instance as the "on" field of the SoPointLight, SoSpotLight and
  SoDirectionalLight node classes.

  \sa SoMFBool
*/

// *************************************************************************

#include <Inventor/fields/SoSFBool.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>

#include "fields/SoSubFieldP.h"
#include "fields/shared.h"

// *************************************************************************

SO_SFIELD_SOURCE(SoSFBool, SbBool, SbBool);

// *************************************************************************

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFBool::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFBool);
}


// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFBool::readValue(SoInput * in)
{
  SbBool val;
  if (!sosfbool_read_value(in, val)) return FALSE;
  this->value = val;
  return TRUE;
}

void
SoSFBool::writeValue(SoOutput * out) const
{
  sosfbool_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFBool field;
  BOOST_CHECK_MESSAGE(SoSFBool::getClassTypeId() != SoType::badType(),
                      "SoSFBool class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

BOOST_AUTO_TEST_CASE(textinput)
{
  SbBool ok;
  SoSFBool field;
  ok = field.set("TRUE");
  BOOST_CHECK_MESSAGE(ok == TRUE, "did not accept 'TRUE'");
  BOOST_CHECK_EQUAL(field.getValue(), TRUE);
  ok = field.set("FALSE");
  BOOST_CHECK_MESSAGE(ok == TRUE, "did not accept 'FALSE'");
  BOOST_CHECK_EQUAL(field.getValue(), FALSE);

  TestSuite::ResetReadErrorCount();
  static const char * filters[] = { "Invalid value", NULL };
  TestSuite::PushMessageSuppressFilters(filters);
  ok = field.set("MAYBE"); // emits two error messages
  BOOST_CHECK_MESSAGE(ok == FALSE, "did accept 'MAYBE'");
  BOOST_CHECK_MESSAGE(TestSuite::GetReadErrorCount() == 1, "did not emit error");
  TestSuite::PopMessageSuppressFilters();
  TestSuite::ResetReadErrorCount();

  ok = field.set("0");
  BOOST_CHECK_MESSAGE(ok == TRUE, "did not accept '0'");
  BOOST_CHECK_EQUAL(field.getValue(), FALSE);
  ok = field.set("1");
  BOOST_CHECK_MESSAGE(ok == TRUE, "did not accept '1'");
  BOOST_CHECK_EQUAL(field.getValue(), TRUE);

  static const char * filters2[] = { "Illegal value", NULL };
  TestSuite::PushMessageSuppressFilters(filters2);
  ok = field.set("2");
  BOOST_CHECK_MESSAGE(ok == FALSE, "did accept '2'");
  TestSuite::PopMessageSuppressFilters();
  TestSuite::ResetReadErrorCount();
}

#endif // COIN_TEST_SUITE
