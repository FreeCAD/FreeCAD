//$ TEMPLATE MFNodeEnginePath(_TYPENAME_, _Typename_, _typename_)

/*!
  \class SoMF_Typename_ SoMF_Typename_.h Inventor/fields/SoMF_Typename_.h
  \brief The SoMF_Typename_ class is a container for _typename_s.
  \ingroup coin_fields

  This field container stores an array of pointers to _typename_s. It takes
  care of the necessary functionality for handling copy, import and
  export operations.

  Note that _typename_ pointers stored in field instances of this type may
  be \c NULL pointers.

  \sa So_Typename_, SoSF_Typename_

*/

// Type-specific define to be able to do #ifdef tests on type.  (Note:
// used to check the header file wrapper define, but that doesn't work
// with --enable-compact build.)
#define COIN_INTERNAL_SOMF_TYPENAME_

#include <Inventor/fields/SoMF_Typename_.h>
#include <Inventor/fields/SoSubFieldP.h>
#include <Inventor/fields/SoSF_Typename_.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoPath.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/nodes/SoNode.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

// These are the macros from SO_MFIELD_SOURCE_MALLOC we're
// using. What's missing is the SO_MFIELD_VALUE_SOURCE macro, which we
// need to implement "by hand" so reference counting and auditing
// comes out correctly.
SO_MFIELD_REQUIRED_SOURCE(SoMF_Typename_);
SO_MFIELD_CONSTRUCTOR_SOURCE(SoMF_Typename_);
SO_MFIELD_MALLOC_SOURCE(SoMF_Typename_, So_Typename_ *);
// Note that we're using the MALLOC versions (which just does
// bit-copying) of the macros, and not the ALLOC versions (which
// allocates with "new", so constructors are run). The reason for this
// is that its node/engine/path *pointers* that are simply bit-wise
// copied.


// Override from parent class.
void
SoMF_Typename_::initClass(void)
{
  SO_MFIELD_INTERNAL_INIT_CLASS(SoMF_Typename_);
}


// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

//// From the SO_MFIELD_VALUE_SOURCE macro, start. ///////////////////////////

// We can't use the macro invocation, as we need to take care of doing
// ref() and unref() on the _typename_s in the array.

int
SoMF_Typename_::fieldSizeof(void) const
{
  return sizeof(So_Typename_ *);
}

void *
SoMF_Typename_::valuesPtr(void)
{
  return (void *)this->values;
}

void
SoMF_Typename_::setValuesPtr(void * ptr)
{
  // We don't get any ref()'ing done here, or any notification
  // mechanisms set up -- so this function should _only_ be used for
  // initial setup of array memory.  In Coin, it's only used from
  // SoMField::allocValues().
  this->values = (So_Typename_ **)ptr;
}

int
SoMF_Typename_::find(So_Typename_ * value, SbBool addifnotfound)
{
  for (int i=0; i < this->num; i++) if ((*this)[i] == value) return i;

  if (addifnotfound) this->set1Value(this->num, value);
  return -1;
}

void
SoMF_Typename_::setValues(const int start, const int num, const So_Typename_ ** newvals)
{
  // Disable temporarily, so we under any circumstances will not send
  // more than one notification about the changes.
  SbBool notificstate = this->enableNotify(FALSE);
  // Important note: the notification state is reset at the end, so
  // this function should *not* have multiple return-points.

  // ref() new _typename_s before unref()-ing old ones, in case there are
  // common _typename_s (we don't want any premature destruction to happen).
  { for (int i=0; i < num; i++) if (newvals[i]) newvals[i]->ref(); }

  // We favor simplicity of code over performance here.
  { for (int i=0; i < num; i++)
    this->set1Value(start+i, (So_Typename_ *)newvals[i]); }

  // unref() to match the initial ref().
  { for (int i=0; i < num; i++) if (newvals[i]) newvals[i]->unref(); }

  // Finally, send notification.
  (void)this->enableNotify(notificstate);
  if (notificstate) this->valueChanged();
}

void
SoMF_Typename_::set1Value(const int idx, So_Typename_ * newval)
{
  // Disable temporarily, so we under no circumstances will send more
  // than one notification about the change.
  SbBool notificstate = this->enableNotify(FALSE);
  // Important note: the notification state is reset at the end, so
  // this function should *not* have multiple return-points.

  // Don't use getNum(), getValues() or operator[] to find old values,
  // since this might trigger a recursive evaluation call if the field
  // is connected.

  // Expand array if necessary.
  if (idx >= this->num) {
#ifdef COIN_INTERNAL_SOMFPATH
    for (int i = this->num; i <= idx; i++) this->pathheads.append(NULL);
#endif // COIN_INTERNAL_SOMFPATH
    this->setNum(idx + 1);
  }

  So_Typename_ * oldptr = this->values[idx];
  if (oldptr != newval) {
    if (oldptr) {
#ifdef COIN_INTERNAL_SOMFPATH
      SoNode * h = oldptr->getHead();
      // The path should be audited by us at all times. So don't use
      // SoMFPath to wrap SoTempPath or SoLightPath, for instance.
      assert(h==this->pathheads[idx] &&
             "Path head changed without notification!");
      if (h) {
        h->removeAuditor(this, SoNotRec::FIELD);
        h->unref();
      }
#endif // COIN_INTERNAL_SOMFPATH
      oldptr->removeAuditor(this, SoNotRec::FIELD);
      oldptr->unref();
    }

    if (newval) {
      newval->addAuditor(this, SoNotRec::FIELD);
      newval->ref();
#ifdef COIN_INTERNAL_SOMFPATH
      SoNode * h = newval->getHead();
      if (h) {
        h->addAuditor(this, SoNotRec::FIELD);
        h->ref();
      }
#endif // COIN_INTERNAL_SOMFPATH
    }

    this->values[idx] = newval;
#ifdef COIN_INTERNAL_SOMFPATH
    this->pathheads[idx] = newval ? newval->getHead() : NULL;
#endif // COIN_INTERNAL_SOMFPATH
  }

  // Finally, send notification.
  (void)this->enableNotify(notificstate);
  if (notificstate) this->valueChanged();
}

void
SoMF_Typename_::setValue(So_Typename_ * value)
{
  this->deleteAllValues();
  this->set1Value(0, value);
}

SbBool
SoMF_Typename_::operator==(const SoMF_Typename_ & field) const
{
  if (this == &field) return TRUE;
  if (this->getNum() != field.getNum()) return FALSE;

  const So_Typename_ ** const lhs = this->getValues(0);
  const So_Typename_ ** const rhs = field.getValues(0);
  for (int i = 0; i < num; i++) if (lhs[i] != rhs[i]) return FALSE;
  return TRUE;
}

/*!
  \copydoc SoMFFloat::deleteAllValues()
*/
void
SoMF_Typename_::deleteAllValues(void)
{
  // Don't use getNum(), but use this->num directly, since getNum()
  // might trigger a recursive evaluation call if the field
  // is connected.

  if (this->num) this->deleteValues(0);
}

// Overridden to handle unref() and removeAuditor().
void
SoMF_Typename_::deleteValues(int start, int num)
{
  // Note: this function overrides the one in SoMField, so if you do
  // any changes here, take a look at that method as well.

  if (num == -1) num = this->num - start;
  for (int i=start; i < start+num; i++) {
    So_Typename_ * n = this->values[i];
    if (n) {
      n->removeAuditor(this, SoNotRec::FIELD);
      n->unref();
    }
#ifdef COIN_INTERNAL_SOMFPATH
    SoNode * h = this->pathheads[start];
    this->pathheads.remove(start);
    if (h) {
      h->removeAuditor(this, SoNotRec::FIELD);
      h->unref();
    }
#endif // COIN_INTERNAL_SOMFPATH
  }

  inherited::deleteValues(start, num);
}

// Overridden to insert NULL pointers in new array slots.
void
SoMF_Typename_::insertSpace(int start, int num)
{
  // Disable temporarily so we don't send notification prematurely
  // from inherited::insertSpace().
  SbBool notificstate = this->enableNotify(FALSE);
  // Important note: the notification state is reset at the end, so
  // this function should *not* have multiple return-points.

  inherited::insertSpace(start, num);
  for (int i=start; i < start+num; i++) {
#ifdef COIN_INTERNAL_SOMFPATH
    this->pathheads.insert(NULL, start);
#endif // COIN_INTERNAL_SOMFPATH
    this->values[i] = NULL;
  }

  // Initialization done, now send notification.
  (void)this->enableNotify(notificstate);
  if (notificstate) this->valueChanged();
}

/*!
  \copydoc SoMFFloat::copyValue()
*/
void
SoMF_Typename_::copyValue(int to, int from)
{
  this->values[to] = this->values[from];
}

//// From the SO_MFIELD_VALUE_SOURCE macro, end. /////////////////////////////


// Import a single _typename_.
SbBool
SoMF_Typename_::read1Value(SoInput * in, int index)
{
  SoSF_Typename_ sf_typename_;
  SbBool result = sf_typename_.readValue(in);
  if (result) this->set1Value(index, sf_typename_.getValue());
  return result;
}

// Export a single _typename_.
void
SoMF_Typename_::write1Value(SoOutput * out, int idx) const
{
  // NB: This code is common for SoMFNode, SoMFPath and SoMFEngine.
  // That's why we check for the base type before writing.

  SoBase * base = (SoBase*) this->values[idx];
  if (base) {
    if (base->isOfType(SoNode::getClassTypeId())) {
      ((SoNode*)base)->writeInstance(out);
    }
    else if (base->isOfType(SoPath::getClassTypeId())) {
      SoWriteAction wa(out);
      wa.continueToApply((SoPath*)base);
    }
    else if (base->isOfType(SoEngine::getClassTypeId())) {
      ((SoEngine*)base)->writeInstance(out);
    }
  }
  else {
    out->write("NULL");
  }
}

#endif // DOXYGEN_SKIP_THIS


// Overridden from parent to propagate write reference counting to
// _typename_.
void
SoMF_Typename_::countWriteRefs(SoOutput * out) const
{
  inherited::countWriteRefs(out);

  for (int i = 0; i < this->getNum(); i++) {
    SoBase * base = this->values[i];
    if (base) {
      // NB: This code is common for SoMFNode, SoMFPath and SoMFEngine.
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
  }
}

// Override from parent to update our _typename_ pointer
// references. This is necessary so we do the Right Thing with regard
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
//    - operator=() calls setValues()
//    - we have a local copy (i.e. not from SoSubField.h) of setValues()
//      that sets up auditing and references the array items
//
// <mortene@sim.no>
void
SoMF_Typename_::fixCopy(SbBool copyconnections)
{
  // Disable temporarily, so we under no circumstances will send more
  // than one notification about the changes.
  SbBool notificstate = this->enableNotify(FALSE);
  // Important note: the notification state is reset at the end, so
  // this function should *not* have multiple return-points.

  for (int i=0; i < this->getNum(); i++) {
    So_Typename_ * n = (*this)[i];
    if (n) {
#if COIN_DEBUG
      n->assertAlive();
#endif // COIN_DEBUG
      // The set1Value() call below will automatically de-audit and
      // un-ref the old pointer value node reference we have in the
      // array, *before* re-inserting a copy.

#if defined(COIN_INTERNAL_SOMFNODE) || defined(COIN_INTERNAL_SOMFENGINE)
      SoFieldContainer * fc = SoFieldContainer::findCopy(n, copyconnections);
#if COIN_DEBUG
      if (fc) fc->assertAlive();
#endif // COIN_DEBUG
      if (fc) this->set1Value(i, (So_Typename_ *)fc);
#endif // COIN_INTERNAL_SOMFNODE || COIN_INTERNAL_SOMFENGINE

#ifdef COIN_INTERNAL_SOMFPATH
      this->set1Value(i, n->copy());
#endif // COIN_INTERNAL_SOMFPATH
    }
  }

  // Finally, send notification.
  (void)this->enableNotify(notificstate);
  if (notificstate) this->valueChanged();
}

// Override from SoField to check _typename_ pointer.
SbBool
SoMF_Typename_::referencesCopy(void) const
{
  if (inherited::referencesCopy()) return TRUE;

  for (int i=0; i < this->getNum(); i++) {
    So_Typename_ * item = (*this)[i];
    if (item) {
#if defined(COIN_INTERNAL_SOMFNODE) || defined(COIN_INTERNAL_SOMFENGINE)
      if (SoFieldContainer::checkCopy((SoFieldContainer *)item)) return TRUE;
#endif // COIN_INTERNAL_SOMFNODE || COIN_INTERNAL_SOMFENGINE
#ifdef COIN_INTERNAL_SOMFPATH
      if (item->getHead() && SoFieldContainer::checkCopy(item->getHead())) return TRUE;
#endif // COIN_INTERNAL_SOMFPATH
    }
  }

  return FALSE;
}

// Kill the type-specific define.
#undef COIN_INTERNAL_SOMF_TYPENAME_
