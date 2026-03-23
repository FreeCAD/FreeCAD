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
  \class SoEngineOutputData SoOutputData.h Inventor/engines/SoOutputData.h
  \brief The SoEngineOutputData class is a container for a prototype set of outputs.

  \ingroup coin_engines

  This class is instantiated once for each class of objects which use
  outputs, and which need to be able to import and export them.

  Each output of a class is stored with the name and type it has been
  given within its "owner" class and a pointer offset to the dynamic
  instance of the output itself.

  It is unlikely that application programmers should need to use any
  of the methods of this class directly.
*/

/* IMPORTANT NOTE:
 * If you make any changes (bugfixes, improvements) in this class,
 * remember to also check the SoFieldData class, as this code is heavily
 * based on that class.
 */

#include <Inventor/engines/SoOutputData.h>

#include "coindefs.h" // COIN_STUB()
#include "SbBasicP.h"

#include <Inventor/engines/SoEngine.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SoType.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG
#include "threads/threadsutilp.h"

#ifndef DOXYGEN_SKIP_THIS // Don't document internal classes.

class SoOutputDataEntry {
public:
  SoOutputDataEntry(const char * n, SoType t, ptrdiff_t offset)
    : name(n), type(t), ptroffset(offset) { }
  SoOutputDataEntry(const SoOutputDataEntry * oe) { this->copy(oe); }
  SoOutputDataEntry(const SoOutputDataEntry & oe) { this->copy(&oe); }

  int operator==(const SoOutputDataEntry * oe) const
  {
    return ((this->name == oe->name) && (this->ptroffset == oe->ptroffset) &&
            (this->type == oe->type));
  }
  int operator!=(const SoOutputDataEntry * oe) const { return ! operator==(oe); }
  int operator==(const SoOutputDataEntry & oe) const { return operator==(&oe); }
  int operator!=(const SoOutputDataEntry & oe) const { return ! operator==(&oe); }

  SbName name;
  SoType type;
  ptrdiff_t ptroffset;

private:
  void copy(const SoOutputDataEntry * oe)
  {
    this->name = oe->name;
    this->type = oe->type;
    this->ptroffset = oe->ptroffset;
  }
};

#endif // DOXYGEN_SKIP_THIS

/*!
  Constructor.
*/
SoEngineOutputData::SoEngineOutputData()
{
}

/*!
  Copy constructor.
*/
SoEngineOutputData::SoEngineOutputData(const SoEngineOutputData *data)
{
  if (data) {
    int n = data->outputlist.getLength();
    for (int i = 0; i < n; i++) {
      this->outputlist.append(new SoOutputDataEntry(data->outputlist[i]));
    }
  }
}

/*!
  Constructor. Supply the approximated number of outputs in the engine
  to optimize memory allocation.
*/
SoEngineOutputData::SoEngineOutputData(int approxnum)
  : outputlist(approxnum)
{
}

/*!
  Destructor.
*/
SoEngineOutputData::~SoEngineOutputData()
{
  for (int i = 0; i < this->outputlist.getLength(); i++) {
    delete this->outputlist[i];
  }
}

/*!
  Add a new output to our internal list.

  The \a name and \a type will be stored along with a pointer offset
  between \a base and \a output, which will be valid for all instances
  of the class type of \a base.
*/
void
SoEngineOutputData::addOutput(const SoEngine *base, const char *name,
                              const SoEngineOutput *output, SoType type)
{
  CC_GLOBAL_LOCK;
  if (!this->hasOutput(name)) {
    this->addOutputInternal(coin_assert_cast<const SoFieldContainer *>(base), name, output, type);
  }
  CC_GLOBAL_UNLOCK;
}

/*!
  Returns the number of outputs contained within this instance.
*/
int
SoEngineOutputData::getNumOutputs(void) const
{
  return this->outputlist.getLength();
}

/*!
  Returns the name of the output at \a index.
*/
const SbName &
SoEngineOutputData::getOutputName(int index) const
{
  return this->outputlist[index]->name;
}

/*!
  Returns a pointer to the output at \a index within the \a engine
  instance.
*/
SoEngineOutput *
SoEngineOutputData::getOutput(const SoEngine *engine, int index) const
{
  return this->getOutputInternal(coin_assert_cast<const SoFieldContainer*>(engine), index);
}

/*!
  Returns the internal index value of \a output in \a engine. If \a output
  is not part of \a engine, it returns -1.
*/
int
SoEngineOutputData::getIndex(const SoEngine *engine,
                             const SoEngineOutput *output) const
{
  return this->getIndexInternal(coin_assert_cast<const SoFieldContainer*>(engine), output);
}

/*!
  Returns the type of the output at \a index.
*/
const SoType &
SoEngineOutputData::getType(int index) const
{
  return this->outputlist[index]->type;
}

/*!
  FIXME: doc
*/
SbBool
SoEngineOutputData::readDescriptions(SoInput * COIN_UNUSED_ARG(in), SoEngine * COIN_UNUSED_ARG(engine)) const
{
  COIN_STUB();
  return FALSE;
}

/*
  FIXME: doc.
*/
void
SoEngineOutputData::writeDescriptions(SoOutput * COIN_UNUSED_ARG(out),
                                  SoEngine *COIN_UNUSED_ARG(engine)
                                  ) const
{
  COIN_STUB();
}

/*!
  \overload
*/
void
SoEngineOutputData::addOutput(const SoNodeEngine * base, const char *name,
                              const SoEngineOutput * output, SoType type)
{
  CC_GLOBAL_LOCK;
  if (!this->hasOutput(name)) {
    this->addOutputInternal(coin_assert_cast<const SoFieldContainer *>(base), name, output, type);
  }
  CC_GLOBAL_UNLOCK;
}

/*!
  \overload
*/
SoEngineOutput *
SoEngineOutputData::getOutput(const SoNodeEngine * engine, int index) const
{
  return this->getOutputInternal(coin_assert_cast<const SoFieldContainer *>(engine), index);
}

/*!
  \overload
*/
int
SoEngineOutputData::getIndex(const SoNodeEngine * engine, const SoEngineOutput * output) const
{
  return this->getIndexInternal(coin_assert_cast<const SoFieldContainer *>(engine), output);
}

// does the actual job of adding an engine output
void
SoEngineOutputData::addOutputInternal(const SoFieldContainer * base, const char *name,
                                      const SoEngineOutput * output, SoType type)
{
  const ptrdiff_t range =
    reinterpret_cast<const char *>(output) -
    reinterpret_cast<const char *>(base);
  this->outputlist.append(new SoOutputDataEntry(name, type, range));

#if COIN_DEBUG
  // FIXME: this is an ugly design flaw, which doesn't seem easily
  // resolvable while still keeping compatibility. 20000915 mortene.
  if (type.isDerivedFrom(SoType::fromName("SFEnum")) ||
      type.isDerivedFrom(SoType::fromName("MFEnum"))) {
    static SbBool warn = TRUE;
    if (warn) {
      warn = FALSE; // Warn only once.
      SoDebugError::postWarning("SoEngineOutputData::addOutput",
                                "Using as engine output a field which has "
                                "enum type is not advisable, as it contains "
                                "more state than just the value of the field "
                                "(i.e. the name<->value hash mapping must "
                                "also be considered in certain situations). "
                                "This is a design flaw.");
    }
  }
#endif // COIN_DEBUG
}

// does the actual job of returning an engine output
SoEngineOutput *
SoEngineOutputData::getOutputInternal(const SoFieldContainer * base, int index) const
{
  assert(index >= 0 && index < this->outputlist.getLength());
  const char * outputptr = reinterpret_cast<const char *>(base);
  outputptr += this->outputlist[index]->ptroffset;
  return
    const_cast<SoEngineOutput *>
    (
     reinterpret_cast<const SoEngineOutput *>(outputptr)
     );
}

// does the actual job of returning an engine output index
int
SoEngineOutputData::getIndexInternal(const SoFieldContainer * base, const SoEngineOutput * output) const
{
  const ptrdiff_t ptroffset =
    reinterpret_cast<const char *>(output) -
    reinterpret_cast<const char *>(base);

  for (int i = 0; i < this->outputlist.getLength(); i++) {
    if (this->outputlist[i]->ptroffset == ptroffset) return i;
  }
  return -1;
}

/*!
  \internal
  \since Coin 2.3
*/
SbBool
SoEngineOutputData::hasOutput(const char * name) const
{
  for (int i = 0; i < this->outputlist.getLength(); i++) {
    if (this->outputlist[i]->name == name) return TRUE;
  }
  return FALSE;
}
