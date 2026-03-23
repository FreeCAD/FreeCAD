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
  \class SoMFEnum SoMFEnum.h Inventor/fields/SoMFEnum.h
  \brief The SoMFEnum class is a container for a set of enumerated values.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  need to store values constrained to be from an enumerated set.

  A field of this type stores its values to file as the symbolic
  names, rather than the actual integer values.

  \sa SoSFEnum, SoMFBitMask
*/

// *************************************************************************

#include <Inventor/fields/SoMFEnum.h>

#include <cassert>

#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

/*!
  \var int SoMFEnum::numEnums
  Number of enumeration mappings.
*/
/*!
  \var SbName * SoMFEnum::enumNames
  Array of enumeration names. Maps 1-to-1 with the enumValues.
*/
/*!
  \var int * SoMFEnum::enumValues
  Array of enumeration values. Maps 1-to-1 with the enumNames.
*/
/*!
  \var SbBool SoMFEnum::legalValuesSet
  Is \c TRUE if a set of enum name-to-value mappings has been set.
*/

// *************************************************************************

SO_MFIELD_REQUIRED_SOURCE(SoMFEnum);
SO_MFIELD_MALLOC_SOURCE(SoMFEnum, int);
SO_MFIELD_VALUE_SOURCE(SoMFEnum, int, int);

// *************************************************************************

// (Declarations hidden in SO_[S|M]FIELD_HEADER macro in header file,
// so don't use Doxygen commenting.)
#ifndef DOXYGEN_SKIP_THIS

/* Constructor. */
SoMFEnum::SoMFEnum(void)
{
  this->values = NULL;
  this->legalValuesSet = FALSE;
  this->numEnums = 0;
  this->enumValues = NULL;
  this->enumNames = NULL;
}

/* Destructor. */
SoMFEnum::~SoMFEnum()
{
  this->enableNotify(FALSE); /* Avoid notifying destructed containers. */
  this->deleteAllValues();
  delete[] this->enumValues;
  delete[] this->enumNames;
}

#endif // DOXYGEN_SKIP_THIS


/*!
  \copydetails SoField::initClass(void)
*/
void
SoMFEnum::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMFEnum);
}


// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoMFEnum::read1Value(SoInput * in, int idx)
{
  SbName n;
  int val;

  // Read mnemonic value as a character string identifier
  if (!in->read(n, TRUE)) {
    // If we don't have any legal values for this field,
    // give some slack and accept integer values.
    if (this->legalValuesSet || !in->read(val)) {
      SoReadError::post(in, "Couldn't read enumeration name");
      return FALSE;
    }
  }
  else {
    if (!this->findEnumValue(n, val)) {
      // If we read an enum field written by an extension node,
      // we won't have any defined enum values. This is indicated by
      // this->legalValuesSet == FALSE. If this is the case, define
      // any encountered enum values on the fly but keep legalValuesSet
      // to FALSE in order not to fool built-in enum field to accept
      // illegal values.
      if (!this->legalValuesSet) {
        int *newvalues = new int[this->numEnums+1];
        SbName *newnames = new SbName[this->numEnums+1];
        int i;
        for (i = 0; i < this->numEnums; i++) {
          newvalues[i] = this->enumValues[i];
          newnames[i] = this->enumNames[i];
        }
        newvalues[i] = i;
        newnames[i] = n;
        delete[] this->enumValues;
        delete[] this->enumNames;
        this->enumValues = newvalues;
        this->enumNames = newnames;
        this->numEnums += 1;
        val = i;
      }
      else {
        SoReadError::post(in, "Unknown enumeration value \"%s\"", n.getString());
        return FALSE;
      }
    }
  }

  assert(idx < this->maxNum);
  this->values[idx] = val;
  return TRUE;
}

// FIXME: this should share code with SoSFEnum::writeValue(). 20031205 mortene.
void
SoMFEnum::write1Value(SoOutput * out, int idx) const
{
  int val = (*this)[idx];
  const SbName *enumname;
  if (findEnumName(val, enumname)) {
    out->write(const_cast<char *>(enumname->getString()));
    return;
  }
  // If we don't have any legal values for this field,
  // pass through read integer values.
  if (!this->legalValuesSet) {
    out->write(val);
    return;
  }

#if COIN_DEBUG
  SoDebugError::post("SoMFEnum::writeValue", "Illegal value (%d) in field", val);
#endif // COIN_DEBUG
}

#endif // DOXYGEN_SKIP_THIS


/*!
  Set this field to contain a single value by specifying an
  enumeration string.
*/
void
SoMFEnum::setValue(const SbName name)
{
  int val;
  if (this->findEnumValue(name, val)) {
    this->setValue(val);
  }
#if COIN_DEBUG
  else {
    SoDebugError::post("SoMFEnum::setValue",
                       "Unknown enum '%s'", name.getString());
  }
#endif // COIN_DEBUG
}

/*!
  Set the value at \a idx to the enumeration value represented
  by \a name.
*/
void
SoMFEnum::set1Value(const int idx, const SbName name)
{
  int val;
  if(this->findEnumValue(name, val)) {
    this->set1Value(idx, val);
  }
#if COIN_DEBUG
  else {
    SoDebugError::post("SoMFEnum::setValue",
                       "Unknown enum '%s'", name.getString());
  }
#endif // COIN_DEBUG
}

/*!
  Makes a set of \a num enumeration \a names map to \a vals.
*/
void
SoMFEnum::setEnums(const int numarg, const int * const vals,
                    const SbName * const names)
{
  delete[] this->enumValues;
  delete[] this->enumNames;

  this->enumValues = new int[numarg];
  this->enumNames = new SbName[numarg];
  this->numEnums = numarg;
  this->legalValuesSet = TRUE;

  for (int i = 0; i < this->numEnums; i++) {
    this->enumValues[i] = vals[i];
    this->enumNames[i] = names[i];
  }
}

/*!
  Return in \a val the enumeration value which matches the given
  enumeration \a name.

  Returns \c TRUE if \a name is a valid enumeration string, otherwise
  \c FALSE.
*/
SbBool
SoMFEnum::findEnumValue(const SbName & name, int & val)
{
  // Look through names table for one that matches
  for (int i = 0; i < this->numEnums; i++) {
    if (name == this->enumNames[i]) {
      val = this->enumValues[i];
      return TRUE;
    }
  }
  return FALSE;
}

/*!
  Set the enumeration \a name which matches the given enumeration
  value.

  Returns \c TRUE if \a value is a valid enumeration value, otherwise
  \c FALSE.
*/
SbBool
SoMFEnum::findEnumName(int value, const SbName * & name) const
{
  // Look through values table for one that matches
  for (int i = 0; i < this->numEnums; i++) {
    if (value == this->enumValues[i]) {
      name = &(this->enumNames[i]);
      return TRUE;
    }
  }
  return FALSE;
}

/*!
  Returns the number of enum names the SoSFEnum object understands.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
int
SoMFEnum::getNumEnums(void) const
{
  return this->numEnums;
}

/*!
  Returns the value of the Nth enum this SoSFEnum object understands,
  and mutates \a name to contain the Nth enum's name.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
int
SoMFEnum::getEnum(const int idx, SbName & name) const
{
  if ( COIN_DEBUG && (idx < 0 || idx >= this->numEnums) ) {
    SoDebugError::post("SoSFEnum::getEnum", "idx (%d) out of range", idx);
    return -1;
  }
  name = this->enumNames[idx];
  return this->enumValues[idx];
}


#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoMFEnum field;
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
  BOOST_CHECK_EQUAL(field.getNum(), 0);
}

#endif // COIN_TEST_SUITE
