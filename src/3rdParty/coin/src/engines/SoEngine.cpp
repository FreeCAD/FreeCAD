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
  \class SoEngine SoEngine.h Inventor/engines/SoEngine.h
  \brief SoEngine is the base class for Coin engines.

  \ingroup coin_engines

  Engines enable the application programmers to make complex
  connections between fields.

  The most common cases where you use engines are: 1) to constrain the
  values of a field with regard to the contents of one or more other
  fields in the scene graph, 2) as a convenient way to animate parts
  of the scene graph.

  The reference count of an engine will be increased by 1 for each
  connection made to one of its engine outputs, and decreased by one
  for a disconnect. See SoEngineOutput::addConnection() and
  SoEngineOutput::removeConnection(). When the reference count goes
  down to zero, the engine will automatically be destroyed, and
  subsequent attempts at using the engine will lead to a crash.

  If you want complete control over when an engine gets destructed,
  use SoBase::ref() and SoBase::unref() for explicit
  referencing/dereferencing.
*/

// *************************************************************************

#include <Inventor/engines/SoEngine.h>

#include "SbBasicP.h"

#include <Inventor/engines/SoEngines.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/engines/SoOutputData.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SoEngineList.h>
#include <Inventor/lists/SoEngineOutputList.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H
#include "coindefs.h" // COIN_STUB()
#ifdef COIN_THREADSAFE
#include "threads/recmutexp.h"
#endif // COIN_THREADSAFE

// *************************************************************************

// FIXME: document these properly. 20000405 mortene.
/*!
  \fn const SoEngineOutputData * SoEngine::getOutputData(void) const
  \COININTERNAL
*/
/*!
  \fn void SoEngine::evaluate(void)
  \COININTERNAL
*/

// *************************************************************************

SoType SoEngine::classTypeId STATIC_SOTYPE_INIT;

// *************************************************************************

/*!
  Default constructor.
*/
SoEngine::SoEngine(void)
{
  this->flags = 0;
}

/*!
  Destructor.
*/
SoEngine::~SoEngine()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngine::~SoEngine", "%p", this);
#endif // debug
}

// Overrides SoBase::destroy().
void
SoEngine::destroy(void)
{
#if COIN_DEBUG && 0 // debug
  SbName n = this->getName();
  SoType t = this->getTypeId();
  SoDebugError::postInfo("SoEngine::destroy", "start -- '%s' (%s)",
                         n.getString(),
                         t.getName().getString());
#endif // debug

  // evaluate() before we actually destruct. It would be too late
  // during the destructor, as the SoEngine::evaluate() method is pure
  // virtual.
  //
  // The explicit call here is done so attached fields will get the
  // chance to update before we die. SoField::disconnect() will
  // normally call SoEngine::evaluate(), but we disable that feature
  // by setting SoEngineOutput::isEnabled() to FALSE before
  // decoupling.

  // need to lock to avoid that evaluateWrapper() is called
  // simultaneously from more than one thread
#ifdef COIN_THREADSAFE
  cc_recmutex_internal_field_lock();
#endif // COIN_THREADSAFE
  this->evaluateWrapper();
#ifdef COIN_THREADSAFE
  cc_recmutex_internal_field_unlock();
#endif // COIN_THREADSAFE

  // SoBase destroy().
  inherited::destroy();

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngine::destroy", "done -- '%s' (%s)",
                         n.getString(),
                         t.getName().getString());
#endif // debug
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoEngine::initClass(void)
{
  SoEngine::classTypeId =
    SoType::createType(SoFieldContainer::getClassTypeId(), SbName("Engine"));

  SoEngine::initClasses();
}

/*!
  Initializes all engines. Automatically called from
  SoEngine::initClass() upon initialization of Coin.
*/
void
SoEngine::initClasses(void)
{
  SoNodeEngine::initClass();
  SoBoolOperation::initClass();
  SoCalculator::initClass();
  SoComposeVec2f::initClass();
  SoComposeVec3f::initClass();
  SoComposeVec4f::initClass();
  SoDecomposeVec2f::initClass();
  SoDecomposeVec3f::initClass();
  SoDecomposeVec4f::initClass();
  SoComposeRotation::initClass();
  SoComposeRotationFromTo::initClass();
  SoDecomposeRotation::initClass();
  SoComposeMatrix::initClass();
  SoDecomposeMatrix::initClass();
  SoComputeBoundingBox::initClass();
  SoConcatenate::initClass();
  SoCounter::initClass();
  SoElapsedTime::initClass();
  SoFieldConverter::initClass();
  SoGate::initClass();
  SoInterpolate::initClass();
    SoInterpolateFloat::initClass();
    SoInterpolateRotation::initClass();
    SoInterpolateVec2f::initClass();
    SoInterpolateVec3f::initClass();
    SoInterpolateVec4f::initClass();
  SoOnOff::initClass();
  SoOneShot::initClass();
  SoSelectOne::initClass();
  SoTimeCounter::initClass();
  SoTransformVec3f::initClass();
  SoTriggerAny::initClass();
  SoTexture2Convert::initClass();
  SoHeightMapToNormalMap::initClass();
}

/*!
  \copybrief SoBase::getClassTypeId(void)
*/
SoType
SoEngine::getClassTypeId(void)
{
  return SoEngine::classTypeId;
}

/*!
  Adds all outputs to list. Returns the number of outputs added to the
  list.
*/
int
SoEngine::getOutputs(SoEngineOutputList & l) const
{
  const SoEngineOutputData * outputs = this->getOutputData();
  if (outputs == NULL) return 0;

  int n = outputs->getNumOutputs();
  for (int i = 0; i < n; i++) {
    l.append(outputs->getOutput(this, i));
  }
  return n;
}

/*!
  Returns the output with name \a outputname, or \c NULL if no such
  output exists.
*/
SoEngineOutput *
SoEngine::getOutput(const SbName & outputname) const
{
  const SoEngineOutputData * outputs = this->getOutputData();
  if (outputs == NULL) return NULL;
  int n = outputs->getNumOutputs();
  for (int i = 0; i < n; i++) {
    if (outputs->getOutputName(i) == outputname)
      return outputs->getOutput(this, i);
  }
  return NULL;
}

/*!
  Sets \a outputname to the name of \a output. Returns \c FALSE if no
  such output is contained within the engine instance.
*/
SbBool
SoEngine::getOutputName(const SoEngineOutput * output,
                        SbName & outputname) const
{
  const SoEngineOutputData * outputs = this->getOutputData();
  if (outputs == NULL) return FALSE;

  int n = outputs->getNumOutputs();
  for (int i = 0; i < n; i++) {
    if (outputs->getOutput(this, i) == output) {
      outputname = outputs->getOutputName(i);
      return TRUE;
    }
  }
  return FALSE;
}

/*!
  Returns the engine named \a name, or \c NULL if no such engine
  exists.  If several engines have been registered under the same
  name, returns the \e last one which was registered.
*/
SoEngine *
SoEngine::getByName(const SbName & name)
{
  return static_cast<SoEngine *>(SoBase::getNamedBase(name, SoEngine::getClassTypeId()));
}

/*!
  Finds all engines named \a name. Returns the number of engines added
  to the \a el list.
*/
int
SoEngine::getByName(const SbName & name, SoEngineList & el)
{
  return SoBase::getNamedBases(name, el, SoEngine::getClassTypeId());
}

/*!
  Called when an input is changed. The default method does nothing,
  but subclasses may override this method to do The Right Thing
  when a specific field is changed.
*/
void
SoEngine::inputChanged(SoField * COIN_UNUSED_ARG(which))
{
}

// doc in parent
void
SoEngine::notify(SoNotList * nl)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngine::notify", "%p - %s, start",
                         this, this->getTypeId().getName().getString());
#endif // debug

  // Avoid recursive notification calls.
  if (this->isNotifying()) return;

  this->flags |= FLAG_ISNOTIFYING;

  // The notification invocation could stem from a value change in
  // whatever this engine is connected to, so we need to be evaluated
  // on the next attempted read on our output(s).
  this->flags |= FLAG_ISDIRTY;

  // Call inputChanged() only if we're being notified through one of
  // the engine's fields (lastrec == CONTAINER, set in
  // SoField::notify()).
  SoNotRec * lastrec = nl->getLastRec();
  if (lastrec && lastrec->getType() == SoNotRec::CONTAINER) {
    // Let engine know that a field changed, so we can recalculate
    // internal variables, if necessary.
    this->inputChanged(nl->getLastField());
  }

  // add ourself to the notification list
  SoNotRec rec(createNotRec());
  rec.setType(SoNotRec::ENGINE);
  nl->append(&rec);

  // Notify the slave fields connected to our engine outputs.
  const SoEngineOutputData * outputs = this->getOutputData();
  int numoutputs = outputs->getNumOutputs();
  for (int i = 0; i < numoutputs; i++)
    outputs->getOutput(this, i)->touchSlaves(nl, this->isNotifyEnabled());

  this->flags &= ~FLAG_ISNOTIFYING;

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngine::notify", "%p - %s, done",
                         this, this->getTypeId().getName().getString());
#endif // debug
}

/*!
  Triggers an engine evaluation.
*/
void
SoEngine::evaluateWrapper(void)
{
  const SoEngineOutputData * outputs = this->getOutputData();
  // For the engines which dynamically allocates input fields and
  // outputs [*], they can be destructed before there's any
  // SoEngineOutputData set up -- for instance upon an error on
  // import. So we need to check for a NULL value here.
  //
  // [*] (So far, that is: SoGate, SoConcatenate, SoSelectOne,
  //     SoConvertAll.)
  if (!outputs) { return; }

  if(!(this->flags & FLAG_ISDIRTY)) { return; }

  this->flags &= ~FLAG_ISDIRTY;

  int i, n = outputs->getNumOutputs();
  for (i = 0; i < n; i++) {
    outputs->getOutput(this, i)->prepareToWrite();
  }
  this->evaluate();
  for (i = 0; i < n; i++) {
    outputs->getOutput(this, i)->doneWriting();
  }
}

/*!
  Returns the SoFieldData class which holds information about inputs
  in this engine.
*/
const SoFieldData **
SoEngine::getInputDataPtr(void)
{
  return NULL; // base class has no output
}

/*!
  Returns the SoEngineOutputData class which holds information about
  the outputs in this engine.
*/
const SoEngineOutputData **
SoEngine::getOutputDataPtr(void)
{
  return NULL; // base class has no input
}

// Documented in superclass.
SbBool
SoEngine::readInstance(SoInput * in, unsigned short flagsarg)
{
  // FIXME: I believe there's code missing here for reading
  // SoUnknownEngine instances. 20000919 mortene.

  flagsarg |= SoBase::IS_ENGINE;
  return inherited::readInstance(in, flagsarg);
}

// Documented in superclass.
void
SoEngine::writeInstance(SoOutput * out)
{
  if (this->writeHeader(out, FALSE, TRUE)) return;
  inherited::writeInstance(out);
  this->writeFooter(out);
}

/*!
  Writes the types of engine outputs for extension engines
  (i.e. engines not built in to Coin).
*/
void
SoEngine::writeOutputTypes(SoOutput * COIN_UNUSED_ARG(out))
{
  COIN_STUB();
}

/*!
  Make a duplicate of this engine and return a pointer to the
  duplicate.

  Connections are shallow copied, i.e. the node or engine instance at
  the other end of the connection is \e not cloned. We just let the
  connection reference from the cloned engine refer to the same
  instance as the engine we've cloned ourselves from.

  Note that this is \e not the function the application programmer
  should override if she needs some special behavior during a copy
  operation (like copying the value of internal data not exposed as
  fields). For that purpose, override the copyContents() method. Your
  overridden copyContents() method should then \e both copy internal
  data as well as calling the parent superclass' copyContents() method
  for automatically handling of fields and other common data.
*/
SoEngine *
SoEngine::copy(void) const
{
  // This code is like a combination of SoNode::copy() and
  // SoNode::addToCopyDict() (minus the children traversal -- engines
  // don't have children).

  SoFieldContainer::initCopyDict();
  // This snippet is the same as SoNode::addToCopyDict().
  SoEngine * cp = coin_assert_cast<SoEngine *>(SoFieldContainer::checkCopy(this));
  if (!cp) {
    cp = static_cast<SoEngine *>(this->getTypeId().createInstance());
    assert(cp);
    SoFieldContainer::addCopy(this, cp);
  }
  // ref() to make sure the copy is not destructed while copying
  cp->ref();

  // Call findCopy() to have copyContents() run once.
  SoEngine * dummy = coin_assert_cast<SoEngine *>(SoFieldContainer::findCopy(this, TRUE));
  assert(dummy == cp);

  SoFieldContainer::copyDone();
  // unrefNoDelete() so that we return a copy with reference count 0
  cp->unrefNoDelete();
  return cp;
}

// Documented in superclass.
SoFieldContainer *
SoEngine::copyThroughConnection(void) const
{
  // Important note: _don't_ try to optimize by skipping the
  // checkCopy() call, as we're not supposed to create copies of
  // containers "outside" the part of the scene graph which is
  // involved in the copy operation.
  SoFieldContainer * connfc = SoFieldContainer::checkCopy(this);
  // if a copy has been made, return the findCopy instance (findCopy
  // will run copyContents() the first time it is called on an
  // instance).
  if (connfc) return SoFieldContainer::findCopy(this, TRUE);

  // If we're outside the scene graph.
  if (this->shouldCopy() == FALSE)
    return
      const_cast<SoFieldContainer *>
      (
       coin_assert_cast<const SoFieldContainer *>(this)
       );

  // Ok, make the first copy and return its pointer.
  SoEngine * cp = coin_assert_cast<SoEngine *>(SoFieldContainer::findCopy(this, TRUE));
  assert(cp);
  return cp;
}

/*!
  Returns whether this engine should be copied or simply referenced in
  a copy operation.

  Engines which are not really part of the scene graph should not be
  copied.
*/
SbBool
SoEngine::shouldCopy(void) const
{
  SbBool result = FALSE;

  SoFieldList fl;
  int nr = this->getFields(fl);
  for (int i=0; i < nr; i++) {
    if (fl[i]->referencesCopy()) {
      result = TRUE;
      break;
    }
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngine::shouldCopy", "%p - %s, result==%s",
                         this, this->getTypeId().getName().getString(),
                         result ? "TRUE" : "FALSE");
#endif // debug

  return result;
}

/*!
  Returns whether we're in a notification process. This is needed to
  avoid double notification when an engine enables outputs during
  inputChanged().
*/
SbBool
SoEngine::isNotifying(void) const
{
  return (this->flags & FLAG_ISNOTIFYING) != 0;
}

void
SoEngine::setDirty(void)
{
  this->flags |= FLAG_ISDIRTY;
}
