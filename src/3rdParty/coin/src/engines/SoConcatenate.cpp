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
  \class SoConcatenate SoConcatenate.h Inventor/engines/SoConcatenate.h
  \brief The SoConcatenate class is used to concatenate several inputs into one output.

  \ingroup coin_engines

  Takes all the values from the 10 input multivalue fields in turn and
  concatenates them into the multivalue output.


  Note that this engine's output field deviates a little from the
  "standard" output mechanism of the majority of engine classes: the
  SoConcatenate::output is not a permanent SoEngineOutput instance,
  but a \e pointer to a SoEngineOutput instance.  The reason for this
  is that it is necessary to allocate the output field dynamically to
  make it match what the SoConcatenate::input is connected to since
  the type of the SoConcatenate::output always should be the same as
  the type of the SoConcatenate::input.


  \ENGINE_TYPELESS_FILEFORMAT

  \verbatim
  Concatenate {
    type <multivaluefieldtype>
    [...fields...]
  }
  \endverbatim
*/

#include <Inventor/engines/SoConcatenate.h>

#include "SbBasicP.h"

#include <Inventor/SbString.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoFields.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoSubEngineP.h"

/*!
  \var SoMField * SoConcatenate::input[10]
  The multivalue input fields which we will concatenate into the
  output.
*/
/*!
  \var SoEngineOutput * SoConcatenate::output

  (SoMField) This is the field output containing the concatenated
  values of all the input fields.

  The type of the field will of course match the type of the input
  field.
*/

// Can't use the standard SO_ENGINE_SOURCE macro, as this engine
// doesn't keep a class-global set of inputs and outputs: we need to
// make an instance of SoFieldData and SoEngineOutputData for every
// instance of the class, since the input and output fields are
// dynamically allocated.
SO_INTERNAL_ENGINE_SOURCE_DYNAMIC_IO(SoConcatenate);


/**************************************************************************/

// Default constructor. Leaves engine in invalid state. Should only be
// used from import code or copy code.
SoConcatenate::SoConcatenate(void)
{
  this->dynamicinput = NULL;
  this->dynamicoutput = NULL;
  for (int i=0; i < SoConcatenate::NUMINPUTS; i++) this->input[i] = NULL;
  this->output = NULL;
}

static SbBool
SoConcatenate_valid_type(SoType t)
{
  return (t.isDerivedFrom(SoMField::getClassTypeId()) &&
          t.canCreateInstance());
}


/*!
  Constructor. The type of the input/output is specified in \a type.
*/
SoConcatenate::SoConcatenate(SoType type)
{
  this->dynamicinput = NULL;
  this->dynamicoutput = NULL;
  for (int i=0; i < SoConcatenate::NUMINPUTS; i++) this->input[i] = NULL;
  this->output = NULL;

#if COIN_DEBUG
  if (!SoConcatenate_valid_type(type)) {
    SoDebugError::post("SoConcatenate::SoConcatenate",
                       "invalid type '%s' for input field, "
                       "field must be non-abstract and a multi-value type.",
                       type == SoType::badType() ? "badType" :
                       type.getName().getString());
  }
#endif // COIN_DEBUG

  this->initialize(type);
}


/*!
  \copybrief SoBase::initClass(void)
*/
void
SoConcatenate::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoConcatenate);
}

// Set up the input and output fields of the engine. This is done from
// either the non-default constructor or the readInstance() import
// code.
void
SoConcatenate::initialize(const SoType inputfieldtype)
{
  assert(this->input[0] == NULL);
  assert(SoConcatenate_valid_type(inputfieldtype));

  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoConcatenate);

  // Instead of SO_ENGINE_ADD_INPUT().
  this->dynamicinput = new SoFieldData(SoConcatenate::inputdata);
  for (int i=0; i < SoConcatenate::NUMINPUTS; i++) {
    this->input[i] = static_cast<SoMField *>(inputfieldtype.createInstance());
    this->input[i]->setNum(0);
    this->input[i]->setContainer(this);
    SbString s = "input";
    s.addIntString(i);
    this->dynamicinput->addField(this, s.getString(), this->input[i]);
  }

  // Instead of SO_ENGINE_ADD_OUTPUT().
  this->output = new SoEngineOutput;
  this->dynamicoutput = new SoEngineOutputData(SoConcatenate::outputdata);
  this->dynamicoutput->addOutput(this, "output", this->output, inputfieldtype);
  this->output->setContainer(this);
}

/*!
  Destructor.
*/
SoConcatenate::~SoConcatenate()
{
  delete this->dynamicinput;
  delete this->dynamicoutput;

  for (int i=0; i < SoConcatenate::NUMINPUTS; i++) delete this->input[i];
  delete this->output;
}

// Documented in superclass. Overridden to initialize type of input
// before reading.
SbBool
SoConcatenate::readInstance(SoInput * in, unsigned short flagsarg)
{
  // This code is identical to readInstance() of SoSelectOne and
  // SoGate, so migrate changes.

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
  if (!SoConcatenate_valid_type(inputtype)) {
    SoReadError::post(in, "Type \"%s\" for input field is not valid "
                      "(field must be non-abstract and a multi-value type).",
                      fieldname.getString());
    return FALSE;
  }

  this->initialize(inputtype);
  return SoEngine::readInstance(in, flagsarg);
}

// Documented in superclass. Overridden to write type of inputs.
void
SoConcatenate::writeInstance(SoOutput * out)
{
  // This code is identical to writeInstance() of SoSelectOne and
  // SoGate, so migrate changes.

  if (this->writeHeader(out, FALSE, TRUE)) return;

  SbBool binarywrite = out->isBinary();

  if (!binarywrite) out->indent();
  out->write("type");
  if (!binarywrite) out->write(' ');
  out->write(this->input[0]->getTypeId().getName());
  if (binarywrite) out->write(static_cast<unsigned int>(0));
  else out->write('\n');

  this->getFieldData()->write(out, this);
  this->writeFooter(out);
}

// Documented in superclass.
void
SoConcatenate::copyContents(const SoFieldContainer * from,
                            SbBool copyconnections)
{
  const SoConcatenate * concatenatesrc =
    coin_assert_cast<const SoConcatenate *>(from);
  if (concatenatesrc->input[0]) { this->initialize(concatenatesrc->input[0]->getTypeId()); }
  inherited::copyContents(from, copyconnections);
}

// Macro used to generate a function for the transfer of values from
// an SoMField to another (of the same type).
#define TRANSFER_FUNC(_fieldtype_) \
static void _fieldtype_##_transfer(SoMField * output, int outidx, SoMField * input) \
{ \
  _fieldtype_ * in = coin_assert_cast<_fieldtype_ *>(input); \
  assert(in != NULL); \
  coin_assert_cast<_fieldtype_ *>(output)->setValues(outidx, in->getNum(), in->getValues(0)); \
}

// Cover all known SoMField subclasses.
TRANSFER_FUNC(SoMFBitMask);
TRANSFER_FUNC(SoMFBool);
TRANSFER_FUNC(SoMFColor);
TRANSFER_FUNC(SoMFEngine);
TRANSFER_FUNC(SoMFEnum);
TRANSFER_FUNC(SoMFFloat);
TRANSFER_FUNC(SoMFInt32);
TRANSFER_FUNC(SoMFMatrix);
TRANSFER_FUNC(SoMFName);
TRANSFER_FUNC(SoMFNode);
TRANSFER_FUNC(SoMFPath);
TRANSFER_FUNC(SoMFPlane);
TRANSFER_FUNC(SoMFRotation);
TRANSFER_FUNC(SoMFShort);
TRANSFER_FUNC(SoMFString);
TRANSFER_FUNC(SoMFTime);
TRANSFER_FUNC(SoMFUInt32);
TRANSFER_FUNC(SoMFUShort);
TRANSFER_FUNC(SoMFVec2f);
TRANSFER_FUNC(SoMFVec3f);
TRANSFER_FUNC(SoMFVec4f);
#undef TRANSFER_FUNC

// documented in superclass
void
SoConcatenate::evaluate(void)
{
  // we can't use SO_ENGINE_OUTPUT here, so the functionality is
  // duplicated in the for-loops.

  int i;

  // we can do this check only once
  if (!this->output->isEnabled()) return;

  int inputstop = -1; // store the last field that has at least one value
  int numvalues = 0;  // store the total number of values
  for (i = 0; i < SoConcatenate::NUMINPUTS; i++) {
    int cnt = this->input[i]->getNum();
    if (cnt) {
      numvalues += cnt;
      inputstop = i;
    }
  }

  const int numconnections = this->output->getNumConnections();
  const SoType type = this->output->getConnectionType();

  for (i = 0; i < numconnections; i++) {
    SoMField * out = coin_assert_cast<SoMField *>((*this->output)[i]);
    if (!out->isReadOnly()) {
      int cnt = 0;
      out->setNum(numvalues);
      for (int j = 0; j <= inputstop; j++) {
        SoMField * in = coin_assert_cast<SoMField *>(input[j]);

        if (type == SoMFBitMask::getClassTypeId()) {
          // (Seems safer to use SoMFBitMask's own methods, and not
          // from the superclass SoMFEnum, even though that may be
          // valid.)
          SoMFBitMask_transfer(out, cnt, in);
        }
        else if (type == SoMFBool::getClassTypeId()) {
          SoMFBool_transfer(out, cnt, in);
        }
        else if (type == SoMFColor::getClassTypeId()) {
          SoMFColor_transfer(out, cnt, in);
        }
        else if (type == SoMFEngine::getClassTypeId()) {
          SoMFEngine_transfer(out, cnt, in);
        }
        else if (type == SoMFEnum::getClassTypeId()) {
          SoMFEnum_transfer(out, cnt, in);
        }
        else if (type == SoMFFloat::getClassTypeId()) {
          SoMFFloat_transfer(out, cnt, in);
        }
        else if (type == SoMFInt32::getClassTypeId()) {
          SoMFInt32_transfer(out, cnt, in);
        }
        else if (type == SoMFMatrix::getClassTypeId()) {
          SoMFMatrix_transfer(out, cnt, in);
        }
        else if (type == SoMFName::getClassTypeId()) {
          SoMFName_transfer(out, cnt, in);
        }
        else if (type == SoMFNode::getClassTypeId()) {
          SoMFNode_transfer(out, cnt, in);
        }
        else if (type == SoMFPath::getClassTypeId()) {
          SoMFPath_transfer(out, cnt, in);
        }
        else if (type == SoMFPlane::getClassTypeId()) {
          SoMFPlane_transfer(out, cnt, in);
        }
        else if (type == SoMFRotation::getClassTypeId()) {
          SoMFRotation_transfer(out, cnt, in);
        }
        else if (type == SoMFShort::getClassTypeId()) {
          SoMFShort_transfer(out, cnt, in);
        }
        else if (type == SoMFString::getClassTypeId()) {
          SoMFString_transfer(out, cnt, in);
        }
        else if (type == SoMFTime::getClassTypeId()) {
          SoMFTime_transfer(out, cnt, in);
        }
        else if (type == SoMFUInt32::getClassTypeId()) {
          SoMFUInt32_transfer(out, cnt, in);
        }
        else if (type == SoMFUShort::getClassTypeId()) {
          SoMFUShort_transfer(out, cnt, in);
        }
        else if (type == SoMFVec2f::getClassTypeId()) {
          SoMFVec2f_transfer(out, cnt, in);
        }
        else if (type == SoMFVec3f::getClassTypeId()) {
          SoMFVec3f_transfer(out, cnt, in);
        }
        else if (type == SoMFVec4f::getClassTypeId()) {
          SoMFVec4f_transfer(out, cnt, in);
        }
        else {
          // Slower fallback (copying by going through string
          // conversion and deconversion), in case of new
          // (user-defined) field types.
          const int num = in->getNum();
          SbString strval;
          for (int k = 0; k < num; k++) {
            in->get1(k, strval);
            out->set1(cnt + k, strval.getString());
          }
        }

        cnt += in->getNum();
      }
    }
  }
}
