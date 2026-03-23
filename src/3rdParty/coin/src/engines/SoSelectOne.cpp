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
  \class SoSelectOne SoSelectOne.h Inventor/engines/SoSelectOne.h
  \brief The SoSelectOne class is used to select one value from a set of values.

  \ingroup coin_engines

  The output field will be the index'th value of the input multivalue
  field.


  Note that this engine's output field deviates a little from the
  "standard" output mechanism of the majority of engine classes: the
  SoSelectOne::output is not a permanent SoEngineOutput instance, but
  a \e pointer to a SoEngineOutput instance.  The reason for this is
  that it is necessary to allocate the output field dynamically to
  make it match what the SoSelectOne::input is connected to since the
  type of the SoSelectOne::output always should be the same as the
  type of the SoSelectOne::input.


  \ENGINE_TYPELESS_FILEFORMAT

  \verbatim
  SelectOne {
    type <multivaluefieldtype>
    [...fields...]
  }
  \endverbatim
*/

#include <Inventor/engines/SoSelectOne.h>

#include <cstring>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoFields.h>
#include <Inventor/lists/SoEngineOutputList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoSubEngineP.h"
#include "SbBasicP.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strstr;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/*!
  \var SoMField * SoSelectOne::input
  The multivalue input field which we will select a single value from
  for our output.
*/
/*!
  \var SoSFInt32 SoSelectOne::index
  Index of the value from the input field which will be put on the
  output.
*/
/*!
  \var SoEngineOutput * SoSelectOne::output
  (SoSField) This is the singlevalue field output containing the index'th
  value of SoSelectOne::input.

  The type of the field will of course match the type of the input field,
  i.e. if SoSelectOne::input is an SoMFFloat, SoSelectOne::output will
  be an SoSFFloat etc.
*/

// Can't use the standard SO_ENGINE_SOURCE macro, as this engine
// doesn't keep a class-global set of inputs and outputs: we need to
// make an instance of SoFieldData and SoEngineOutputData for every
// instance of the class, since the input and output fields are
// dynamically allocated.
SO_INTERNAL_ENGINE_SOURCE_DYNAMIC_IO(SoSelectOne);

static SbBool
SoSelectOne_valid_type(SoType t)
{
  return (t.isDerivedFrom(SoMField::getClassTypeId()) &&
          t.canCreateInstance());
}

/*!
  Constructor. Sets the type of the input field. The input field must
  be of type SoMField.
*/
SoSelectOne::SoSelectOne(SoType inputtype)
{
  this->dynamicinput = NULL;
  this->dynamicoutput = NULL;
  this->input = NULL;
  this->output = NULL;

#if COIN_DEBUG
  if (!SoSelectOne_valid_type(inputtype)) {
    SoDebugError::post("SoSelectOne::SoSelectOne",
                       "invalid type '%s' for input field, "
                       "field must be non-abstract and a multi-value type.",
                       inputtype == SoType::badType() ? "badType" :
                       inputtype.getName().getString());
  }
#endif // COIN_DEBUG

  this->initialize(inputtype);
}

// Default constructor. Leaves engine in invalid state. Should only be
// used from import code or copy code.
SoSelectOne::SoSelectOne(void)
{
  this->dynamicinput = NULL;
  this->dynamicoutput = NULL;
  this->input = NULL;
  this->output = NULL;
}

// Set up the input and output fields of the engine. This is done from
// either the non-default constructor or the readInstance() import
// code.
void
SoSelectOne::initialize(const SoType inputfieldtype)
{
  assert(this->input == NULL);
  assert(SoSelectOne_valid_type(inputfieldtype));

  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoSelectOne);
  SO_ENGINE_ADD_INPUT(index, (0));

  // Instead of SO_ENGINE_ADD_INPUT().
  this->input = static_cast<SoMField *>(inputfieldtype.createInstance());
  this->input->setNum(0);
  this->input->setContainer(this);
  this->dynamicinput = new SoFieldData(SoSelectOne::inputdata);
  this->dynamicinput->addField(this, "input", this->input);

  SbString multiname = inputfieldtype.getName().getString();
  // Built-in fields always start with the "MF", but we try to handle
  // user-defined fields as well.
  const char * ptr = strstr(multiname.getString(), "MF");
  assert(ptr != NULL && "invalid input field type");
  const ptrdiff_t offset = ptr - multiname.getString();
  SbString singlename = (offset == 0) ? SbString("") : multiname.getSubString(0, (int)offset - 1);
  singlename += 'S';
  singlename += multiname.getSubString((int)offset + 1);

  SoType outputtype = SoType::fromName(singlename);
  assert(outputtype != SoType::badType() &&
         outputtype.isDerivedFrom(SoSField::getClassTypeId()) &&
         "invalid input field type");

  // Instead of SO_ENGINE_ADD_OUTPUT().
  this->output = new SoEngineOutput;
  this->dynamicoutput = new SoEngineOutputData(SoSelectOne::outputdata);
  this->dynamicoutput->addOutput(this, "output", this->output, outputtype);
  this->output->setContainer(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoSelectOne::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoSelectOne);
}

SoSelectOne::~SoSelectOne()
{
  delete this->dynamicinput;
  delete this->dynamicoutput;

  delete this->input;
  delete this->output;
}

// Documented in superclass.
void
SoSelectOne::evaluate(void)
{
  int idx = this->index.getValue();

  if (idx == 0 && this->input->getNum() == 0) {
    // Nil is the no-op value (also the default initial value).
    SO_ENGINE_OUTPUT((*output), SoSField, setDirty(FALSE));
  }
  else if (idx >= 0 && idx < this->input->getNum()) {

    // Macro used to generate the right casts for copying field values
#define IF_TYPE(_var_, _fieldtype_) \
    if(_var_ == SoMF##_fieldtype_::getClassTypeId() ) \
    { \
        SO_ENGINE_OUTPUT((*output), SoSF##_fieldtype_, setValue((*(coin_assert_cast<SoMF##_fieldtype_ *>(this->input)))[idx])); \
    }
    // end of macro

    SoType type = this->input->getTypeId();
    IF_TYPE(type,BitMask)
    else IF_TYPE(type,Bool)
    else IF_TYPE(type,Color)
    else IF_TYPE(type,Engine)
    else IF_TYPE(type,Enum)
    else IF_TYPE(type,Float)
    else IF_TYPE(type,Int32)
    else IF_TYPE(type,Matrix)
    else IF_TYPE(type,Name)
    else IF_TYPE(type,Node)
    else IF_TYPE(type,Path)
    else IF_TYPE(type,Plane)
    else IF_TYPE(type,Rotation)
    else IF_TYPE(type,Short)
    else IF_TYPE(type,String)
    else IF_TYPE(type,Time)
    else IF_TYPE(type,UInt32)
    else IF_TYPE(type,UShort)
    else IF_TYPE(type,Vec2f)
    else IF_TYPE(type,Vec3f)
    else IF_TYPE(type,Vec4f)
    else {
      // fall back for user defined types, and built-in types not
      // covered by the above (if any)
      SbString valuestring;
      this->input->get1(idx, valuestring);
      SO_ENGINE_OUTPUT((*output), SoSField, set(valuestring.getString()));
    }
#undef IF_TYPE
  }
#if COIN_DEBUG
  else {
    SoDebugError::post("SoSelectOne::evaluate", "invalid index %d", idx);
  }
#endif // COIN_DEBUG
}

// Documented in superclass.
SbBool
SoSelectOne::readInstance(SoInput * in, unsigned short flagsarg)
{
  // This code is identical to readInstance() of SoGate and
  // SoConcatenate, so migrate changes.

  SbName tmp;
  if (!in->read(tmp) || tmp != "type") {
    SoReadError::post(in,
                      "\"type\" keyword is missing, erroneous format for "
                      "engine class '%s'.",
                      this->getTypeId().getName().getString());
    return FALSE;
  }
  // need to use an SbString here, because SoInput::read( SbName & )
  // reads in '"MyName"' as is instead of as 'MyName'.
  SbString fieldname;
  if (!in->read(fieldname)) {
    SoReadError::post(in, "Couldn't read input type for engine.");
    return FALSE;
  }
  SoType inputtype = SoType::fromName(fieldname);
  if (!SoSelectOne_valid_type(inputtype)) {
    SoReadError::post(in, "Type \"%s\" for input field is not valid "
                      "(field must be non-abstract and a multi-value type).",
                      fieldname.getString());
    return FALSE;
  }

  this->initialize(inputtype);
  return SoEngine::readInstance(in, flagsarg);
}

// Documented in superclass.
void
SoSelectOne::writeInstance(SoOutput * out)
{
  // This code is identical to writeInstance() of SoGate and
  // SoConcatenate, so migrate changes.

  if (this->writeHeader(out, FALSE, TRUE)) return;

  SbBool binarywrite = out->isBinary();

  if (!binarywrite) out->indent();
  out->write("type");
  if (!binarywrite) out->write(' ');
  out->write(this->input->getTypeId().getName());
  if (binarywrite) out->write(static_cast<unsigned int>(0));
  else out->write('\n');

  this->getFieldData()->write(out, this);
  this->writeFooter(out);
}

// Documented in superclass.
void
SoSelectOne::copyContents(const SoFieldContainer * from,
                          SbBool copyconnections)
{
  const SoSelectOne * selectonesrc = coin_assert_cast<const SoSelectOne *>(from);
  if (selectonesrc->input) { this->initialize(selectonesrc->input->getTypeId()); }
  inherited::copyContents(from, copyconnections);
}
