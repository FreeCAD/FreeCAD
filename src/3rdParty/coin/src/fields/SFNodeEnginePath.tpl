//$ TEMPLATE SFNodeEnginePath(_TYPENAME_, _Typename_, _typename_)

/*!
  \class SoSF_Typename_ SoSF_Typename_.h Inventor/fields/SoSF_Typename_.h
  \brief The SoSF_Typename_ class is a container for a single _typename_.
  \ingroup coin_fields

  This field container stores a pointer to a Coin _typename_. It takes care
  of the necessary functionality for handling copy, import and export
  operations.

  Note that the _typename_ pointer stored in a field instance of this type
  may be a \c NULL pointer.

  \sa So_Typename_, SoMF_Typename_

*/

// Type-specific define to be able to do #ifdef tests on type.  (Note:
// used to check the header file wrapper define, but that doesn't work
// with --enable-compact build.)
#define COIN_INTERNAL_SOSF_TYPENAME_

#include <Inventor/fields/SoSF_Typename_.h>
#include <Inventor/fields/SoSubFieldP.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/SoOutput.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG


// Can't use SO_SFIELD_SOURCE() because we need to modify setValue()
// to ref and unref the passed _typename_.
SO_SFIELD_REQUIRED_SOURCE(SoSF_Typename_);


// Override from parent class.
void
SoSF_Typename_::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSF_Typename_);
}

// (Declarations hidden in SO_[S|M]FIELD_HEADER macro in header file,
// so don't use Doxygen commenting.)
#ifndef DOXYGEN_SKIP_THIS

/* Constructor, sets initial _typename_ pointer to a \c NULL pointer. */
SoSF_Typename_::SoSF_Typename_(void)
{
  this->value = NULL;
#ifdef COIN_INTERNAL_SOSFPATH
  this->head = NULL;
#endif // COIN_INTERNAL_SOSFPATH
}

/* Destructor, dereferences the current _typename_ pointer if necessary. */
SoSF_Typename_::~SoSF_Typename_(void)
{
  this->enableNotify(FALSE);
  this->setValue(NULL);
}

#endif // DOXYGEN_SKIP_THIS


// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

// Store the \a newval _typename_ pointer in this field. If \a newval is not
// \c NULL, will add 1 to the reference count of the _typename_.
void
SoSF_Typename_::setValue(So_Typename_ * newval)
{
  // Don't use getValue() to find oldptr, since this might trigger a
  // recursive evaluation call if the field is connected.
  So_Typename_ * oldptr = this->value;
  if (oldptr == newval) return;

  if (oldptr) {
#ifdef COIN_INTERNAL_SOSFPATH
    SoNode * h = oldptr->getHead();
    // The path should be audited by us at all times. So don't use
    // SoSFPath to wrap SoTempPath or SoLightPath, for instance.
    assert(h==this->head && "Path head changed without notification!");
    if (h) {
      h->removeAuditor(this, SoNotRec::FIELD);
      h->unref();
    }
#endif // COIN_INTERNAL_SOSFPATH
    oldptr->removeAuditor(this, SoNotRec::FIELD);
    oldptr->unref();
  }

  if (newval) {
    newval->addAuditor(this, SoNotRec::FIELD);
    newval->ref();
#ifdef COIN_INTERNAL_SOSFPATH
    this->head = newval->getHead();
    if (this->head) {
      this->head->addAuditor(this, SoNotRec::FIELD);
      this->head->ref();
    }
#endif // COIN_INTERNAL_SOSFPATH
  }

  this->value = newval;
  this->valueChanged();
}

// Compares to see if the \a field points to the same _typename_ as this
// field does, and returns \c TRUE if this is the case.
//
// Be aware that this method does \e not check for _typename_/subgraph
// equality if the pointers are not the same, so \c FALSE is returned
// even though the contents of the _typename_/subgraph are equal.
SbBool
SoSF_Typename_::operator==(const SoSF_Typename_ & field) const
{
  return (this->getValue() == field.getValue());
}

// Import _typename_.
SbBool
SoSF_Typename_::readValue(SoInput * in)
{
  SoBase * baseptr;
  if (!SoBase::read(in, baseptr, So_Typename_::getClassTypeId())) return FALSE;

  if (in->eof()) {
    SoReadError::post(in, "Premature end of file");
    return FALSE;
  }

  this->setValue((So_Typename_ *)baseptr);
  return TRUE;
}

// Export _typename_.
void
SoSF_Typename_::writeValue(SoOutput * out) const
{
  // NB: This code is common for SoSFNode, SoSFPath and SoSFEngine.
  // That's why we check the base type before writing.
  SoBase * base = this->getValue();
  if (base) {
    if (base->isOfType(SoNode::getClassTypeId())) {
      ((SoNode*)base)->writeInstance(out);
    }
    else if (base->isOfType(SoPath::getClassTypeId())) {
      SoWriteAction wa(out);
      wa.continueToApply((SoPath *)base);
    }
    else if (base->isOfType(SoEngine::getClassTypeId())) {
      ((SoEngine *)base)->writeInstance(out);
    }
    else {
      assert(0 && "strange internal error");
    }
  }
  else {
    // This actually works for both ASCII and binary formats.
    out->write("NULL");
  }
}

#endif // DOXYGEN_SKIP_THIS


// Overridden from parent to propagate write reference counting to
// _typename_.
void
SoSF_Typename_::countWriteRefs(SoOutput * out) const
{
  inherited::countWriteRefs(out);

  SoBase * base = this->getValue();
  if (base == NULL) return;

  // NB: This code is common for SoSFNode, SoSFPath and SoSFEngine.
  // That's why we check the base type before writing/counting

  if (base->isOfType(SoNode::getClassTypeId())) {
    ((SoNode*)base)->writeInstance(out);
  }
  else if (base->isOfType(SoEngine::getClassTypeId())) {
    ((SoEngine*)base)->addWriteReference(out);
  }
  else if (base->isOfType(SoPath::getClassTypeId())) {
    SoWriteAction wa(out);
    wa.continueToApply((SoPath*)base);
  }
}

// Override from parent to update our _typename_ pointer
// reference. This is necessary so we do the Right Thing with regard
// to the copyconnections flag.
//
// Note that we have to unplug auditing and the reference counter
// addition we made during the copy process.
//
// For reference for future debugging sessions, copying of this field
// goes like this:
//
//    - copyFrom() is called (typically from SoFieldData::overlay())
//    - copyFrom() calls operator=()
//    - operator=() calls setValue()
//    - we have a local copy (i.e. not from SoSubField.h) of setValue()
//      that sets up auditing and references the item
//
// <mortene@sim.no>
void
SoSF_Typename_::fixCopy(SbBool copyconnections)
{
  So_Typename_ * n = this->getValue();
  if (!n) return;

#if COIN_DEBUG
  n->assertAlive();
#endif // COIN_DEBUG

  // The setValue() call below will automatically de-audit and un-ref
  // the old pointer-value reference we have, *before* re-inserting a
  // copy.

#if defined(COIN_INTERNAL_SOSFNODE) || defined(COIN_INTERNAL_SOSFENGINE)
  SoFieldContainer * fc = SoFieldContainer::findCopy(n, copyconnections);
#if COIN_DEBUG
  if (fc) fc->assertAlive();
#endif // COIN_DEBUG
  if (fc) this->setValue((So_Typename_ *)fc);
#endif // COIN_INTERNAL_SOSFNODE || COIN_INTERNAL_SOSFENGINE

#ifdef COIN_INTERNAL_SOSFPATH
  this->setValue(n->copy());
#endif // COIN_INTERNAL_SOSFPATH
}

// Override from SoField to check _typename_ pointer.
SbBool
SoSF_Typename_::referencesCopy(void) const
{
  if (inherited::referencesCopy()) return TRUE;

  SoBase * n = this->getValue();
  if (!n) return FALSE;

  if (n->isOfType(SoNode::getClassTypeId()) ||
      n->isOfType(SoEngine::getClassTypeId())) {
    if (SoFieldContainer::checkCopy((SoFieldContainer *)n)) return TRUE;
  }
  else if (n->isOfType(SoPath::getClassTypeId())) {
    SoPath * p = (SoPath *)n;
    if (p->getHead() == NULL) return FALSE;
    if (SoFieldContainer::checkCopy(p->getHead())) return TRUE;
  }
  else {
    assert(0 && "strange internal error");
  }

  return FALSE;
}

// Kill the type-specific define.
#undef COIN_INTERNAL_SOSF_TYPENAME_
