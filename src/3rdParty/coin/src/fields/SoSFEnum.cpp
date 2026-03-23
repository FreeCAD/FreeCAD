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
  \class SoSFEnum SoSFEnum.h Inventor/fields/SoSFEnum.h
  \brief The SoSFEnum class is a container for an enum value.

  \ingroup coin_fields

  This field is used where nodes, engines or other field containers
  needs to store one particular value out of an enumerated set.

  A field of this type stores its value to file as the symbolic
  name, rather than the actual integer value.

  SoSFEnum instances are initialized on an instance basis, usually in
  the constructor of the field container with the macros
  SO_NODE_DEFINE_ENUM_VALUE(enumtype, symbolvalue) and
  SO_NODE_SET_SF_ENUM_TYPE(enumfield, enumtype) for nodes, or for
  engines; SO_ENGINE_DEFINE_ENUM_VALUE() and
  SO_ENGINE_SET_SF_ENUM_TYPE().

  Example initialization from the constructor of the SoCone class:
  \code
  SO_NODE_DEFINE_ENUM_VALUE(Part, SIDES);
  SO_NODE_DEFINE_ENUM_VALUE(Part, BOTTOM);
  SO_NODE_DEFINE_ENUM_VALUE(Part, ALL);
  SO_NODE_SET_SF_ENUM_TYPE(parts, Part); \endcode

  \sa SoMFEnum, SoSFBitMask
*/

// *************************************************************************

#include <Inventor/fields/SoSFEnum.h>

#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

/*!
  \var int SoSFEnum::numEnums
  Number of enumeration mappings.
*/
/*!
  \var SbName * SoSFEnum::enumNames
  Array of enumeration names. Maps 1-to-1 with the enumValues.
*/
/*!
  \var int * SoSFEnum::enumValues
  Array of enumeration values. Maps 1-to-1 with the enumNames.
*/
/*!
  \var SbBool SoSFEnum::legalValuesSet
  Is \c TRUE if a set of enum name-to-value mappings has been set.
*/

// *************************************************************************

PRIVATE_TYPEID_SOURCE(SoSFEnum);
PRIVATE_EQUALITY_SOURCE(SoSFEnum);

// *************************************************************************

// (Declarations hidden in SO_[S|M]FIELD_HEADER macro in header file,
// so don't use Doxygen commenting.)
#ifndef DOXYGEN_SKIP_THIS

/* Constructor. */
SoSFEnum::SoSFEnum(void)
{
  this->enumValues = NULL;
  this->enumNames  = NULL;
  this->numEnums = 0;
  this->legalValuesSet = FALSE;
}

/* Destructor. */
SoSFEnum::~SoSFEnum()
{
  delete[] this->enumValues;
  delete[] this->enumNames;
}

// *************************************************************************

/* Copy enumeration mappings and \a field value. */
const SoSFEnum &
SoSFEnum::operator=(const SoSFEnum & field)
{
  this->setEnums(field.numEnums, field.enumValues, field.enumNames);
  this->setValue(field.getValue());
  return *this;
}

#endif // DOXYGEN_SKIP_THIS


/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFEnum::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFEnum);
}

/*!
  Makes a set of \a num enumeration \a names map to integer values,
  given by \a vals.
*/
void
SoSFEnum::setEnums(const int num, const int * vals, const SbName * names)
{
  delete[] this->enumValues;
  delete[] this->enumNames;

  this->enumValues = new int[num];
  this->enumNames = new SbName[num];
  this->numEnums = num;
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
SoSFEnum::findEnumValue(const SbName & name, int & val)
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
SoSFEnum::findEnumName(int valuearg, const SbName *& name) const
{
  // Look through values table for one that matches
  for (int i = 0; i < this->numEnums; i++) {
    if (valuearg == this->enumValues[i]) {
      name = &(this->enumNames[i]);
      return TRUE;
    }
  }
  return FALSE;
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFEnum::readValue(SoInput * in)
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
  this->value = val;
  return TRUE;
}

void
SoSFEnum::writeValue(SoOutput * out) const
{
  const SbName *enumname;
  if (findEnumName(this->getValue(), enumname)) {
    out->write(const_cast<char *>(enumname->getString()));
    return;
  }
  // If we don't have any legal values for this field,
  // pass through read integer values.
  if (!this->legalValuesSet) {
    out->write(this->getValue());
    return;
  }

#if COIN_DEBUG
  // An unknown enumeration value will usually be an indication of a
  // more serious bug, so we elaborate a bit on the error to aid early
  // debugging.  mortene.

  SbName name;
  const SoFieldContainer * thecontainer = this->getContainer();
  const SbBool fname = thecontainer && thecontainer->getFieldName(this, name);
  SbString s("");
  if (fname) { s.sprintf(" \"%s\"", name.getString()); }

  SoDebugError::post("SoSFEnum::writeValue",
                     "Illegal enumeration value %d in field%s",
                     this->getValue(), s.getString());
#endif // COIN_DEBUG
}

/*!
  Compare this field with \a f and return \c TRUE if they are
  equal.
*/
int
SoSFEnum::operator==(const SoSFEnum & f) const
{
  // Check for value mismatch first.
  if (this->getValue() != f.getValue()) return FALSE;

  // Check that we have the same enumeration mappings.
  if (this->numEnums != f.numEnums) return FALSE;
  for (int i = 0; i < this->numEnums; i++) {
    if (this->enumValues[i] != f.enumValues[i]) return FALSE;
    if (this->enumNames[i] != f.enumNames[i]) return FALSE;
  }

  return TRUE;
}

/*!
  Set the value of this field by specifying an enumeration integer value.
*/
void
SoSFEnum::setValue(int newvalue)
{
  this->value = newvalue;
  this->valueChanged();
}

#endif // DOXYGEN_SKIP_THIS

/*!
  Set the value of this field by specifying an enumeration string value.
*/
void
SoSFEnum::setValue(const SbName name)
{
  int val;
  if (this->findEnumValue(name, val)) {
    this->setValue(val);
  }
  else {
#if COIN_DEBUG
    SoDebugError::post("SoSFEnum::setValue",
                       "Unknown enum '%s'", name.getString());
#endif // COIN_DEBUG
  }
}

/*!
  Returns the number of enum names the SoSFEnum object understands.

  Note that this API method is not part of the original SGI Inventor
  2.1 API, and is an extension specific to Coin.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
int
SoSFEnum::getNumEnums(void) const
{
  return this->numEnums;
}

/*!
  Returns the value of the Nth enum this SoSFEnum object understands,
  and mutates \a name to contain the Nth enum's name.

  Note that this API method is not part of the original SGI Inventor
  2.1 API, and is an extension specific to Coin.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
int
SoSFEnum::getEnum(const int idx, SbName & name) const
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
  SoSFEnum field;
  BOOST_CHECK_MESSAGE(SoSFEnum::getClassTypeId() != SoType::badType(),
                      "SoSFEnum class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
