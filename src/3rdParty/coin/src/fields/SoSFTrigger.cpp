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
  \class SoSFTrigger SoSFTrigger.h Inventor/fields/SoSFTrigger.h
  \brief The SoSFTrigger class is the "void" class used for detecting field changes.

  \ingroup coin_fields

  Connect this field to a master field (or engine output) to detect
  when the master field changes its value.

  This is useful if you want to automatically trigger an update from
  the node or engine (or other field container) this field is
  part of whenever another field changes -- and you are not particularly
  interested in the actual value of the master field.

  \sa SoFieldSensor
*/

#include <Inventor/fields/SoSFTrigger.h>

#include "coindefs.h"

#include "fields/SoSubFieldP.h"

SO_SFIELD_CONSTRUCTOR_SOURCE(SoSFTrigger);
// The following code is the internals of the
// SO_SFIELD_REQUIRED_SOURCE macro, with a minor modification
// (setValue() takes a void argument).
PRIVATE_TYPEID_SOURCE(SoSFTrigger);
PRIVATE_EQUALITY_SOURCE(SoSFTrigger);
// (Declarations hidden in SO_[S|M]FIELD_HEADER macro in header file,
// so don't use Doxygen commenting.)
#ifndef DOXYGEN_SKIP_THIS
const SoSFTrigger &
SoSFTrigger::operator=(const SoSFTrigger & COIN_UNUSED_ARG(field))
{
  this->setValue();
  return *this;
}
#endif // DOXYGEN_SKIP_THIS

/*!
  This field class does not actually contain any value, so we
  just trigger an update by calling touch() within this method.
 */
void
SoSFTrigger::setValue(void)
{
  this->touch();
}

/*!
  Field doesn't contain any value, so this method does nothing.
 */
void
SoSFTrigger::getValue(void) const
{
  // Does nothing.
}

/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFTrigger::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFTrigger);
}

void
SoSFTrigger::touch(void)
{
  // call SoField::startNotify() to notify our container
  SoField::startNotify();
}

/*!
  Since SoSFTrigger fields doesn't have any value, they are all
  equal. So this method always returns \c TRUE.
 */
int
SoSFTrigger::operator==(const SoSFTrigger & COIN_UNUSED_ARG(trigger)) const
{
  return 1;
}

/*!
  Since SoSFTrigger fields doesn't have any value, they are all
  equal. So this method always returns \c FALSE.
*/
int
SoSFTrigger::operator!=(const SoSFTrigger & COIN_UNUSED_ARG(trigger)) const
{
  return 0;
}

void
SoSFTrigger::startNotify(void)
{
  // if notification starts here, we should not notify out container
  SoFieldContainer * oldcontainer = this->getContainer();
  this->setContainer(NULL);
  inherited::startNotify();
  this->setContainer(oldcontainer);
}

void
SoSFTrigger::notify(SoNotList * l)
{
  inherited::notify(l);
}

// No value to read, so just return TRUE.
SbBool
SoSFTrigger::readValue(SoInput * COIN_UNUSED_ARG(in))
{
  return TRUE;
}

// No value to write, just return.
void
SoSFTrigger::writeValue(SoOutput * COIN_UNUSED_ARG(out)) const
{
  return;
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFTrigger field;
  BOOST_CHECK_MESSAGE(SoSFTrigger::getClassTypeId() != SoType::badType(),
                      "SoSFTrigger class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
