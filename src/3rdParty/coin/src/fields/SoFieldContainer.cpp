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
  \class SoFieldContainer SoFieldContainer.h Inventor/fields/SoFieldContainer.h
  \brief The SoFieldContainer class is a base class for all classes that
  contain fields.

  \ingroup coin_general

  The classes containing fields in Coin are the node and engine
  classes, so they are all subclasses of SoFieldContainer.

  SoFieldContainer provides methods for reading, writing, comparing
  for equality, doing copy operations, etc on fields.

  \sa SoField
*/

// *************************************************************************

#include <Inventor/fields/SoFieldContainer.h>

#include <cstring>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/fields/SoFieldData.h>
#include <Inventor/fields/SoMField.h>
#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFImage3.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/threads/SbStorage.h>

#include <Inventor/SbTypeInfo.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/SbTime.h>
#undef COIN_INTERNAL
#include <Inventor/SbLinear.h>
#include <Inventor/SbVec.h>
#define COIN_INTERNAL

#include "tidbitsp.h"
#include "misc/SbHash.h"
#include "io/SoWriterefCounter.h"
#include "coindefs.h" // COIN_STUB()
#include "SbBasicP.h"

// *************************************************************************

/*!
  \var SbBool SoFieldContainer::isBuiltIn

  Flag for storing whether or not this class instance is a built-in
  class or not. By knowing the difference between a class which is
  native Coin or a user extension, it is possible to automatically
  store and read extension nodes and engines.
*/

// *************************************************************************

extern "C" {
static void sofieldcontainer_userdata_cleanup(void);
static void sofieldcontainer_construct_copydict(void * closure);
static void sofieldcontainer_destruct_copydict(void * closure);
static void sofieldcontainer_copydict_cleanup(void);
static void SoFieldContainer_cleanupClass(void);
}

SoType SoFieldContainer::classTypeId STATIC_SOTYPE_INIT;

// used by setUserData() and getUserData()
typedef SbHash<const SoFieldContainer *, void *> UserDataMap;
static UserDataMap * sofieldcontainer_userdata_dict = NULL;

void
sofieldcontainer_userdata_cleanup(void)
{
  delete sofieldcontainer_userdata_dict;
  sofieldcontainer_userdata_dict = NULL;
}

// *************************************************************************

#define FLAG_DONOTIFY      0x01
#define FLAG_FIRSTINSTANCE 0x02

// *************************************************************************

/*!
  Constructor.
*/
SoFieldContainer::SoFieldContainer(void)
  : isBuiltIn(TRUE), donotify(FLAG_DONOTIFY) // HACK warning: donotify is used as a bitmask
{
}
/*!
  Destructor.
*/
SoFieldContainer::~SoFieldContainer()
{
  // "Dangling" userdata can cause bugs if new objects derived from
  // SoFieldContainer are subsequently fitted into the same memory
  // area that this destructed instance is placed at. So we must
  // remove our entry (if any -- we can just ignore the return value,
  // no harm is done if no data was set for this instance).
  (void)sofieldcontainer_userdata_dict->erase(this);
}

// *************************************************************************
// Use a stack of dictionaries when copying nodes to allow recursive
// copying.

// typedef SbHash<const SoFieldContainer *, const SoFieldContainer *> SoFieldContainerCopyMap;

#define SOFIELDCONTAINER_COPYDICT_DEBUG (COIN_DEBUG && 0)

class SoFieldContainerCopyMap : public SbHash<const SoFieldContainer *, const SoFieldContainer *> {
  typedef SbHash<const SoFieldContainer *, const SoFieldContainer *> inherited;
public:
  SoFieldContainerCopyMap(void) : inherited() {
#if SOFIELDCONTAINER_COPYDICT_DEBUG
    SoDebugError::postInfo("SoFieldContainerCopyMap::SoFieldContainerCopyMap",
                           "%p", this);
#endif // DEBUG
  }
  ~SoFieldContainerCopyMap(void) {
#if SOFIELDCONTAINER_COPYDICT_DEBUG
    SoDebugError::postInfo("SoFieldContainerCopyMap::~SoFieldContainerCopyMap",
                           "%p", this);
#endif // DEBUG
  }

  SbBool put(const SoFieldContainer * orig, const SoFieldContainer * copy) {
#if SOFIELDCONTAINER_COPYDICT_DEBUG
    SoDebugError::postInfo("SoFieldContainerCopyMap::put",
                           "%p === %p (setting)", orig, copy);
#endif // DEBUG
    return inherited::put(orig, copy);
  }
  SbBool get(const SoFieldContainer * orig, const SoFieldContainer * & copy)
  {
    SbBool ok = inherited::get(orig, copy);
#if SOFIELDCONTAINER_COPYDICT_DEBUG
    SoDebugError::postInfo("SoFieldContainerCopyMap::get",
                           "%p ::= %p%s", orig, copy,
                           ok ? " (found)" : " (NOT FOUND)");
#endif // DEBUG
    return ok;
  }
};


typedef SbHash<const SoFieldContainer *, SbBool> ContentsCopiedMap;

typedef struct {
  SbList<SoFieldContainerCopyMap *> * copiedinstancestack;
  SbList<ContentsCopiedMap *> * contentscopiedstack;
} sofieldcontainer_copydict;

void
sofieldcontainer_construct_copydict(void * closure)
{
  sofieldcontainer_copydict * data = static_cast<sofieldcontainer_copydict *>(closure);

  data->copiedinstancestack = new SbList<SoFieldContainerCopyMap *>;
  data->contentscopiedstack = new SbList<ContentsCopiedMap *>;
}

void
sofieldcontainer_destruct_copydict(void * closure)
{
  sofieldcontainer_copydict * data = static_cast<sofieldcontainer_copydict *>(closure);
  delete data->copiedinstancestack;
  delete data->contentscopiedstack;
}

// use thread local storage to store copydict in threadsafe version of
// Coin
static SbStorage * sofieldcontainer_copydictstorage;

void
sofieldcontainer_copydict_cleanup(void)
{
  delete sofieldcontainer_copydictstorage;
}

static sofieldcontainer_copydict *
sofieldcontainer_get_copydict(void)
{
  return static_cast<sofieldcontainer_copydict *>(sofieldcontainer_copydictstorage->get());
}

// *************************************************************************

/*!
  \copybrief SoBase::initClass(void)
*/

void
SoFieldContainer::initClass(void)
{
  // Make sure we init this class only once.
  assert(SoFieldContainer::classTypeId == SoType::badType());
  // Make sure parent class has been initialized.
  assert(inherited::getClassTypeId() != SoType::badType());

  SoFieldContainer::classTypeId =
    SoType::createType(inherited::getClassTypeId(), "FieldContainer", NULL);

  sofieldcontainer_userdata_dict = new UserDataMap;
  coin_atexit(sofieldcontainer_userdata_cleanup, CC_ATEXIT_NORMAL);

  sofieldcontainer_copydictstorage =
    new SbStorage(sizeof(sofieldcontainer_copydict),
                  sofieldcontainer_construct_copydict,
                  sofieldcontainer_destruct_copydict);

  coin_atexit(sofieldcontainer_copydict_cleanup, CC_ATEXIT_NORMAL);
  coin_atexit(SoFieldContainer_cleanupClass, CC_ATEXIT_NORMAL);
}

void
SoFieldContainer_cleanupClass(void)
{
  SoFieldContainer::cleanupClass();
}

/*!
  This static method cleans up static data for the SoFieldContainer class.
*/
void
SoFieldContainer::cleanupClass(void)
{
  SoFieldContainer::classTypeId STATIC_SOTYPE_INIT;
}


/*!
  \copybrief SoBase::getClassTypeId(void)
*/
SoType
SoFieldContainer::getClassTypeId(void)
{
  return SoFieldContainer::classTypeId;
}

/*!
  This method sets all fields which we are parenting to their respective
  default values.
*/
void
SoFieldContainer::setToDefaults(void)
{
  const SoFieldData * fd = this->getFieldData();
  if (!fd) return;

  // Allocate a fresh template to retrieve values from.
  const SoFieldContainer * from =
    static_cast<const SoFieldContainer *>(this->getTypeId().createInstance());
  from->ref();
  fd->overlay(this, from, FALSE);
  from->unref();

  SoFieldList l;
  int n = this->getFields(l);
  for (int i=0; i < n; i++) l[i]->setDefault(TRUE);
}

/*!
  This method checks to see if the fields of this container have their
  default values, and returns \c TRUE if that is the case.
*/
SbBool
SoFieldContainer::hasDefaultValues(void) const
{
  const SoFieldData * fd = this->getFieldData();
  if (!fd) return TRUE;

  // Allocate a fresh template to compare with.
  const SoFieldContainer * fc =
    static_cast<const SoFieldContainer *>(this->getTypeId().createInstance());
  fc->ref();
  SbBool hasdefaultvalues = fd->isSame(this, fc);
  fc->unref();
  return hasdefaultvalues;
}

/*!
  This method returns \c TRUE if the values of the fields of this and
  \a container are equal.
*/
SbBool
SoFieldContainer::fieldsAreEqual(const SoFieldContainer * container) const
{
  const SoFieldData * fd0 = this->getFieldData();

  if (fd0 == NULL) {
    if (container->getFieldData() == NULL) return TRUE;
    return FALSE;
  }

  return fd0->isSame(this, container);
}


/*!
  \COININTERNAL

  This method copies the field values from container into this. The
  fields are assumed to be of the same type. The \a copyconnections
  flag decides whether the field connections are to be copied as well.
*/
void
SoFieldContainer::copyFieldValues(const SoFieldContainer * container,
                                  SbBool copyconnections)
{
  const SoFieldData * fd0 = this->getFieldData();

  if (fd0 == NULL) {
    if (container->getFieldData() == NULL) return;
#if COIN_DEBUG
    SoDebugError::postInfo("SoFieldContainer::copyFieldValues",
                           "tried to copy from fieldcontainer of wrong type");
    return;
#endif // COIN_DEBUG
  }

  fd0->overlay(this, container, copyconnections);
}


/*!
  This method parses the values of one or more fields from the
  \a fielddata string.

  The fields must be in the same format as the Open Inventor file
  format.  \c TRUE is returned upon success, and \c FALSE otherwise.

  \sa get()
 */
SbBool
SoFieldContainer::set(const char * const fielddata)
{
  return this->set(fielddata, NULL);
}

/*!
  This methods stores the field data in the given \a fielddata string.
  The format of the returned string follows the Open Inventor file
  format.

  \sa set()
*/
void
SoFieldContainer::get(SbString & fielddata)
{
  this->get(fielddata, NULL);
}


/*!
  This method adds the fields in this container to the \a l argument,
  and returns the number of fields added.

  The virtual nature of this function means that it can be overridden to
  e.g. hide private fields.
*/
int
SoFieldContainer::getFields(SoFieldList & fields) const
{
  const SoFieldData * fielddata = this->getFieldData();
  if ( fielddata == NULL ) return 0;
  const int numfields = fielddata->getNumFields();
  int fieldsadded = 0;
  for ( int i = 0; i < numfields; i++ ) {
    SoField * field = fielddata->getField(this, i);
    assert(field != NULL);
    switch ( field->getFieldType() ) {
    // these field types are ignored
    case SoField::EVENTIN_FIELD:
    case SoField::EVENTOUT_FIELD:
      break;

    default:
      fields.append(field);
      fieldsadded++;
      break;
    }
  }
  return fieldsadded;
}

/*!
  Returns VRML eventIn and eventOut fields in addition to the
  "ordinary" fields.

  \sa getFields()
 */
int
SoFieldContainer::getAllFields(SoFieldList & fields) const
{
  const SoFieldData * fielddata = this->getFieldData();
  if ( fielddata == NULL ) return 0;
  int numfields = fielddata->getNumFields();
  for ( int i=0; i < numfields; i++ )
    fields.append(fielddata->getField(this, i));
  return numfields;
}

/*!
  This method returns a pointer to the field with \a name, or
  \c NULL if no such field exists.
*/
SoField *
SoFieldContainer::getField(const SbName & name) const
{
  const SoFieldData * fields = this->getFieldData();
  if (!fields) return NULL;

  const int numfields = fields->getNumFields();
  for (int i = 0; i < numfields; i++) {
    if (name == fields->getFieldName(i))
      return fields->getField(this, i);
  }

  return NULL;
}

/*!
  This method returns a pointer to the eventIn with name \a name,
  or \c NULL if no such eventIn exists.
*/
SoField *
SoFieldContainer::getEventIn(const SbName & name) const
{
  SoField * f = this->getField(name);
  return (f && (f->getFieldType() == SoField::EVENTIN_FIELD)) ? f : NULL;
}

/*!
  This method returns a pointer to the eventOut with name \a name,
  or \c NULL if no such eventOut exists.
*/
SoField *
SoFieldContainer::getEventOut(const SbName & name) const
{
  SoField * f = this->getField(name);
  return (f && (f->getFieldType() == SoField::EVENTOUT_FIELD)) ? f : NULL;
}


/*!
  Finds the name of the given \a field and returns the value in the
  \a name argument.

  \c TRUE is returned if the field is contained within this instance,
  and \c FALSE otherwise.
*/
SbBool
SoFieldContainer::getFieldName(const SoField * const field,
                               SbName & name) const
{
  const SoFieldData * fields = this->getFieldData();
  if (!fields) return FALSE;
  int idx = fields->getIndex(this, field);
  if (idx == -1) return FALSE;
  name = fields->getFieldName(idx);
  return TRUE;
}


/*!
  This method sets whether notification will be propagated on changing the
  values of the contained fields.  The old value of the flag is returned.

  Notification is default \e on. Turning off automatic notification
  can be useful for optimizing performance. During series of updates
  you may want to avoid the propagation of notifications upon every
  field value change if you make a lot of them. This is how you should
  handle those cases:

  \code
  SbBool autonotify = node->enableNotify(FALSE);
  // ...
  // Make modifications to fields of "node" here.
  // ...
  node->enableNotify(autonotify);
  node->touch();
  \endcode

  \sa isNotifyEnabled()
 */
SbBool
SoFieldContainer::enableNotify(const SbBool enable)
{
  int & flags = this->donotify;
  const SbBool old = (flags & FLAG_DONOTIFY) != 0;
  flags &= ~FLAG_DONOTIFY;
  if (enable) flags |= FLAG_DONOTIFY;
  return old;
}

/*!
  This method returns whether notification of changes to the field values
  in the container is propagated to its auditors.

  \sa enableNotify()
*/
SbBool
SoFieldContainer::isNotifyEnabled(void) const
{
  const int flags = this->donotify;
  return (flags & FLAG_DONOTIFY) != 0;
}


/*!
  This method parses the values of one or more fields from the
  \a fielddata string.

  The fields must be in the same format as the Open Inventor file
  format.  \c TRUE is returned upon success, and \c FALSE otherwise.

  We use the reference dictionary provided by \a in.

  \sa get()
*/
SbBool
SoFieldContainer::set(const char * fielddata, SoInput * in)
{
  const SoFieldData * fields = this->getFieldData();
  if (!fields) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoFieldContainer::set",
                           "tried to set values of non-existent fields");
#endif // COIN_DEBUG
    // Return TRUE here might seem strange, but I think its correct to
    // do it like this -- we're just supposed to read field values until
    // we can't do that anymore. mortene.
    return TRUE;
  }

  SoInput * readbuf = in ? new SoInput(in) : new SoInput;
  readbuf->setBuffer(const_cast<char *>(fielddata), strlen(fielddata));

  SbBool dummy;
  SbBool ok = fields->read(readbuf, this, FALSE, dummy);
  delete readbuf;
  return ok;
}

// Used from get()'s SoOutput if the initial buffer is too small.
static void *
realloc_buffer(void * buffer, size_t newsize)
{
  return realloc(buffer, newsize);
}

/*!
  Put names and values of fields from this instance in the \a fielddata
  string. Fields will be separated in the returned string by end-of-line
  characters.

  We use the reference dictionary provided by \a out.

  \sa set()
 */
void
SoFieldContainer::get(SbString & fielddata, SoOutput * out)
{
  const SoFieldData * fields = this->getFieldData();
  if (!fields) {
    fielddata = "";
    return;
  }

  SoOutput * output = out ? new SoOutput(out) : new SoOutput();

  const size_t STARTSIZE = 256;
  void * buffer = malloc(STARTSIZE);

  output->setBuffer(buffer, STARTSIZE, realloc_buffer);

  output->setStage(SoOutput::COUNT_REFS);
  fields->write(output, this);
  output->setStage(SoOutput::WRITE);
  fields->write(output, this);

  size_t size;
  output->getBuffer(buffer, size);

  // Initialize fielddata to the default value if no fields are found.
  fielddata = "";

  // getBuffer() might return a size of zero. If it does, no need to
  // try to strip off the header.
  if (size > 0) {
    // Strip off header.
    char * start = strstr(static_cast<char *>(buffer), "\n\n");
    if (start != NULL) {
      start += 2;
      fielddata = start;
    }
  }

  free(buffer);

  delete output;
}

// Overridden from parent class.
void
SoFieldContainer::notify(SoNotList * l)
{
#if COIN_DEBUG && 0 // debug
  char c;
  SoDebugError::postInfo("SoFieldContainer::notify", "fc %p, list %p, stack %p", this, l, &c);
#endif // debug

  const int flags = this->donotify;
  if (flags & FLAG_DONOTIFY) {
    if (l->getLastRec()->getType() == SoNotRec::PARENT) {
      // we were notified from a child node. Create a new SoNotRec.
      SoNotRec rec(createNotRec());
      l->append(&rec);
      inherited::notify(l);
    }
    else {
      // we were notified from a field. No need to add a new
      // SoNotRec since the base pointer will point to us.
      assert(l->getLastRec()->getType() == SoNotRec::CONTAINER);
      assert(l->getLastRec()->getBase() == coin_assert_cast<SoBase *>(this));
      inherited::notify(l);
    }
  }
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoFieldContainer::notify", "DONE");
#endif // debug
}

/*!
  \COININTERNAL
  I can't make head or tails of this method, it seems to return TRUE
  no matter what the arguments are.
 */
SbBool
SoFieldContainer::validateNewFieldValue(SoField * COIN_UNUSED_ARG(field),
                                   void * COIN_UNUSED_ARG(newval))
{
  COIN_STUB();
  return TRUE;
}

// Documented in superclass. Overridden from SoBase to make sure field
// connections into other field containers are also accounted for.
void
SoFieldContainer::addWriteReference(SoOutput * out, SbBool isfromfield)
{
  // only count the fields if !isfromfield and this is the first time
  // that this node is counted (shouldWrite() is FALSE).
  SbBool countfields = !isfromfield && (SoWriterefCounter::instance(out)->shouldWrite(this) == FALSE);
  inherited::addWriteReference(out, isfromfield);

  // Avoid doing too many references to fields (and nodes, engines and
  // paths _within_ certain field types).
  if (countfields) {
    const SoFieldData * fd = this->getFieldData();
    if (fd) fd->write(out, this);
  }
}


// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Writes all the fields contained in this instance to the output stream
  within \a out.

  This method is solely called from the write methods of fields.

  Either from SoField if the write is done because of a field-to-field
  connection, or from one of the fields which may actually write
  SoFieldContainer instances, i.e. SoSFNode, SoMFNode, SoSFEngine,
  SoMFEngine, SoSFPath and SoMFPath.

  This method, in addition to the ordinary write() method of SoNode,
  needs to be present since the fields don't have a write action
  instance in their writeValue() method, and writeInstance() will
  create a new SoWriteAction and call continueToApply() on it.
*/
void
SoFieldContainer::writeInstance(SoOutput * out)
{
  const SoFieldData * fd = this->getFieldData();
  if (fd) fd->write(out, this);
}

/*!
  Returns \c TRUE if this object is instantiated from one of the native
  Coin classes, \c FALSE if the object's class is outside the standard
  Coin library.
 */
SbBool
SoFieldContainer::getIsBuiltIn(void) const
{
  return this->isBuiltIn;
}


/*!
  Returns a pointer to the class-wide field data storage object
  for this instance. If no fields are present, returns \c NULL.
 */
const SoFieldData *
SoFieldContainer::getFieldData(void) const
{
  return NULL;
}


// Note: the following documentation is used for all subclasses of
// SoFieldContainer (all the way down to nodekits, draggers and
// manipulators, for instance), so keep it general.
/*!
  Makes a deep copy of all data of \a from into this instance, \e
  except external scene graph references if \a copyconnections is \c
  FALSE.

  This is the method that should be overridden by extension node /
  engine / dragger / whatever subclasses which need to account for
  internal data that are not handled automatically.

  For copying nodes from application code, you should not invoke this
  function directly, but rather call the SoNode::copy() function:

  \code
  SoNode * mynewnode = templatenode->copy();
  \endcode

  The same also goes for engines.


  Make sure that when you override the copyContents() method in your
  extension class that you also make it call upwards to its parent
  superclass in the inheritance hierarchy, as copyContents() in for
  instance SoNode and SoFieldContainer does important work. It should
  go something like this:

  \code
  void
  MyCoinExtensionNode::copyContents(const SoFieldContainer * from,
                                    SbBool copyconnections)
  {
    // let parent superclasses do their thing (copy fields, copy
    // instance name, etc.)
    SoNode::copyContents(from, copyconnections);

    // [..then copy internal data..]
  }
  \endcode
*/
void
SoFieldContainer::copyContents(const SoFieldContainer * from,
                               SbBool copyconnections)
{
  this->setName(from->getName());
  this->donotify = from->donotify;
  this->copyFieldValues(from, copyconnections);
  if (from->getUserData()) this->setUserData(from->getUserData());
}


/*!
  Return copy of this instance.

  Note: default implementation just returns \c this pointer, SoNode
  and SoEngine overrides this method to return the pointer to the
  actual copy.
 */
SoFieldContainer *
SoFieldContainer::copyThroughConnection(void) const
{
  // Cast away const.
  return const_cast<SoFieldContainer *>(this);
}

/*!
  \COININTERNAL

  Initialize a dictionary hash storing pointers for original
  field container instances and their copies during scene graph copy
  operations.

  This method is called from the start of SoNode::copy().
 */
void
SoFieldContainer::initCopyDict(void)
{
  sofieldcontainer_copydict * copydict =
    sofieldcontainer_get_copydict();

  // Create two new dictionaries and _insert_ them in slot 0 in their
  // corresponding parallel lists, so they are stacked atop of the
  // already existing copy dictionaries (if any).

  // Push on stack.
  copydict->copiedinstancestack->insert(new SoFieldContainerCopyMap, 0);
  copydict->contentscopiedstack->insert(new ContentsCopiedMap, 0);
}


/*!
  Add a pair of pointers to an original field container and a copy of it
  to an internal pointer dictionary used during scene graph copy
  operations.
 */
void
SoFieldContainer::addCopy(const SoFieldContainer * orig,
                          const SoFieldContainer * copy)
{
  sofieldcontainer_copydict * copydict =
    sofieldcontainer_get_copydict();

  // ref() so that we can guarantee that the copy is not destructed
  // while we're copying. This fixes a nodekit/dragger copy problem
  // (for instance when copying SoDragPointDragger).  FIXME: figure
  // out exactly _why_ it is necessary to ref/unref the copied
  // instances. We need to track down exactly what happens in the
  // draggers that triggers an assert when copied. pederb, 2003-06-30
  copy->ref();

  SoFieldContainerCopyMap * copiedinstances = (*(copydict->copiedinstancestack))[0];
  ContentsCopiedMap * contentscopied  = (*(copydict->contentscopiedstack))[0];

  assert(copiedinstances);
  assert(contentscopied);

  SbBool s = copiedinstances->put(orig, copy);
  assert(s);
  s = contentscopied->put(orig, FALSE);
  assert(s);
}


/*!
  Returns the copy of \a orig, if any. If no copy exists, we return a
  \c NULL pointer.
 */
SoFieldContainer *
SoFieldContainer::checkCopy(const SoFieldContainer * orig)
{
  sofieldcontainer_copydict * copydict =
    sofieldcontainer_get_copydict();

  SoFieldContainerCopyMap * copiedinstances = (*(copydict->copiedinstancestack))[0];
  assert(copiedinstances);

  const SoFieldContainer * fccopy;
  // FIXME: ugly constness cast. 20050520 mortene.
  return const_cast<SoFieldContainer *>
    (
     copiedinstances->get(orig, fccopy) ? fccopy : NULL
     );
}


/*!
  Returns a copy of \a orig.

  If no copy has been made earlier when the function is called, a copy
  is made on-the-fly (which is the reason we need to pass along the
  \a copyconnections flag).

  This method will also run the copyContents() method on the copy, if
  it hasn't been run already. Note that if you call copyContents() on
  the copy outside of this method, this will go undetected and the
  guts of \a orig will be copied multiple times into its copy.

  If copyContents() is called directly (instead of using copy()), it is
  assumed that the user only wants to copy the field values, and we
  just return NULL here. This is done to match how it's done in SGI
  Inventor.

  \sa checkCopy()
*/
SoFieldContainer *
SoFieldContainer::findCopy(const SoFieldContainer * orig,
                           const SbBool copyconnections)
{
  sofieldcontainer_copydict * copydict =
    sofieldcontainer_get_copydict();

  // Sometimes copyContents() is called directly (initCopyDict() is
  // not called first) to just copy the fields of a single node. This
  // is often done in SGI example code. We should just return NULL
  // here in those cases to be compatible with SGI Inventor.
  if (copydict->copiedinstancestack->getLength() == 0) return NULL;

  SoFieldContainerCopyMap * copiedinstances = (*(copydict->copiedinstancestack))[0];
  ContentsCopiedMap * contentscopied  = (*(copydict->contentscopiedstack))[0];

  assert(copiedinstances);
  assert(contentscopied);

  const SoNode * protonode = coin_safe_cast<const SoNode *>(orig);
  SoProtoInstance * protoinst = protonode ?
    SoProtoInstance::findProtoInstance(protonode) : NULL;

  SoFieldContainer * cp = SoFieldContainer::checkCopy(orig);
  if (!cp) {
    if (protoinst) {
      // We need to do some extra work when copying nodes that are
      // ProtoInstance root nodes. We create a new ProtoInstance node,
      // and register its root node as the copy. pederb, 2002-06-17
      SoProto * proto = protoinst->getProtoDefinition();
      SoProtoInstance * newinst = proto->createProtoInstance();
      if (protoinst->getName().getLength()) {
        newinst->setName(protoinst->getName());
      }
      cp = newinst->getRootNode();
      // We have to call addCopy() before calling copyContents() since
      // the proto instance might have a field that has a pointer to
      // the root node. pederb, 2002-09-04
      newinst->copyContents(protoinst, FALSE);
    }
    else {
      const SoFieldContainer * ccp = NULL;
      if (orig->isOfType(SoProto::getClassTypeId())) {
        // just copy the pointer. A PROTO definition is
        // read-only. It's not possible to change it after it has been
        // created so this should be safe.
        ccp = orig;
      }
      else {
        ccp = static_cast<const SoFieldContainer *>(orig->getTypeId().createInstance());
      }
      assert(ccp);
      SoFieldContainer::addCopy(orig, ccp);
      cp = const_cast<SoFieldContainer *>(ccp);
    }
  }

  // Don't call copyContents for the proto instance root node, since
  // this is handled by the Proto node.
  if (!protoinst) {
    SbBool copied = FALSE;
    SbBool chk = contentscopied->get(orig, copied);
    assert(chk);

    if (!copied) {
      // we have to update the dictionary _before_ calling
      // copyContents in case we have an SoSFNode field in the node
      // that has a pointer to the node. Example scene graph:
      //
      // DEF mynode Script {
      //   field SFNode self USE mynode
      // }
      //
      // pederb, 2002-09-04
      chk = contentscopied->put(orig, TRUE);
      assert(!chk && "the key already exists");
      cp->copyContents(orig, copyconnections);
    }
  }
  return cp;
}

/*!
  \COININTERNAL

  Clean up the dictionary hash.

  This method is called from the end of SoNode::copy().
*/
void
SoFieldContainer::copyDone(void)
{
  sofieldcontainer_copydict * copydict =
    sofieldcontainer_get_copydict();

  SoFieldContainerCopyMap * copiedinstances = (*(copydict->copiedinstancestack))[0];
  ContentsCopiedMap * contentscopied  = (*(copydict->contentscopiedstack))[0];

  assert(copiedinstances);
  assert(contentscopied);

  // unref all copied instances. See comment in addCopy().
  for(
      SoFieldContainerCopyMap::const_iterator iter =
       copiedinstances->const_begin();
      iter!=copiedinstances->const_end();
      ++iter
      ) {
    iter->obj->unref();
  }


  delete copiedinstances;
  delete contentscopied;

  // Pop off stack.
  copydict->copiedinstancestack->remove(0);
  copydict->contentscopiedstack->remove(0);
}

// Documented in superclass.
SbBool
SoFieldContainer::readInstance(SoInput * in, unsigned short flags)
{
  const SoFieldData * fd = this->getFieldData();
  if (fd) {
    SbBool notbuiltin;
    return fd->read(in, this,
                    // The "error on unknown field" is FALSE when
                    // we're a group node, since fields can be
                    // followed by children node specifications.
                    (flags & SoBase::IS_GROUP) ? FALSE : TRUE,
                    notbuiltin);
  }
  return TRUE;
}

/*!
  Returns the data array sizes of the SoFieldData-registered multi-fields.
  The \a managed argument returns the size of the fields that Coin manages the
  memory for, and the \a unmanaged argument returns the size of the data in
  the multi-fields controlled by the application through
  SoMField::setValuesPointer() which Coin will not delete.

  Data that is kept in the object memory chunk (that is included when you
  do sizeof(object)) is not included in these values - only the memory that
  is managed in additional memory chunks from the object chunk is returned.

  Extension field types that are not known to the plain Coin library are
  not accounted for.  This function is therefore virtual, so such extension
  field types can be added to the numbers for extension nodes.

  This method is used for memory profiling purposes.

  \since Coin 3.0
*/
void
SoFieldContainer::getFieldsMemorySize(size_t & managed, size_t & unmanaged) const
{
  managed = 0;
  unmanaged = 0;

  SoFieldList fields;
  int numfields = this->getAllFields(fields);
  for (int c = 0; c < numfields; ++c) {
    const SoField * field = fields[c];

    if (field->getTypeId().isDerivedFrom(SoSField::getClassTypeId())) {
      // a couple of single-fields have "external"/allocated data as
      // well as the multi-fields

      const SoSField * sfield = static_cast<const SoSField *>(field);
      SoType sftype = sfield->getTypeId();

      if (sfield->getTypeId().isDerivedFrom(SoSFImage::getClassTypeId())) {
        const SoSFImage * imgfield = static_cast<const SoSFImage *>(sfield);
        SbVec2s size(0, 0);
        int nc = 0;
        imgfield->getValue(size, nc);
        managed += size_t(size[0]) * size_t(size[1]) * size_t(nc);
      } else if (sfield->getTypeId().isDerivedFrom(SoSFImage3::getClassTypeId())) {
        const SoSFImage3 * img3field = static_cast<const SoSFImage3 *>(sfield);
        SbVec3s size(0, 0, 0);
        int nc = 0;
        img3field->getValue(size, nc);
        managed += size_t(size[0]) * size_t(size[1]) * size_t(size[2]) * size_t(nc);
      } else {
        // memory size estimation probably not needed
      }
    }
    else if (field->getTypeId().isDerivedFrom(SoMField::getClassTypeId())) {
      const SoMField * mfield = static_cast<const SoMField *>(field);
      SoType mftype = mfield->getTypeId();
      int numelements = mfield->getNum();
      size_t elementsize = 0;

#define SBNAMESTRING(arg) \
  static const SbName SO__CONCAT(arg, _string)(SO__QUOTE(arg))

      SBNAMESTRING(MFBitMask);
      SBNAMESTRING(MFBool);
      SBNAMESTRING(MFColor);
      SBNAMESTRING(MFColorRGBA);
      SBNAMESTRING(MFDouble);
      SBNAMESTRING(MFEngine);
      SBNAMESTRING(MFEnum);
      SBNAMESTRING(MFFloat);
      SBNAMESTRING(MFInt32);
      SBNAMESTRING(MFMatrix);
      SBNAMESTRING(MFName);
      SBNAMESTRING(MFNode);
      SBNAMESTRING(MFPlane);
      SBNAMESTRING(MFRotation);
      SBNAMESTRING(MFShort);
      SBNAMESTRING(MFString);
      SBNAMESTRING(MFTime);
      SBNAMESTRING(MFUInt32);
      SBNAMESTRING(MFUShort);
      SBNAMESTRING(MFVec2b);
      SBNAMESTRING(MFVec2d);
      SBNAMESTRING(MFVec2f);
      SBNAMESTRING(MFVec2i32);
      SBNAMESTRING(MFVec2s);
      SBNAMESTRING(MFVec3b);
      SBNAMESTRING(MFVec3d);
      SBNAMESTRING(MFVec3f);
      SBNAMESTRING(MFVec3i32);
      SBNAMESTRING(MFVec3s);
      SBNAMESTRING(MFVec4b);
      SBNAMESTRING(MFVec4d);
      SBNAMESTRING(MFVec4f);
      SBNAMESTRING(MFVec4i32);
      SBNAMESTRING(MFVec4s);
      SBNAMESTRING(MFVec4ub);
      SBNAMESTRING(MFVec4ui32);
      SBNAMESTRING(MFVec4us);

      // TODO: stuff these values into an std::map<> instead of this ifelse...
      const char * mftypekey = mftype.getName().getString();
      if (mftypekey == MFBitMask_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFBitMask>::DataType[2]) / 2;
      } else if (mftypekey == MFBool_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFBool>::DataType[2]) / 2;
      } else if (mftypekey == MFColor_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFColor>::DataType[2]) / 2;
      } else if (mftypekey == MFColorRGBA_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFColorRGBA>::DataType[2]) / 2;
      } else if (mftypekey == MFDouble_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFDouble>::DataType[2]) / 2;
      } else if (mftypekey == MFEngine_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFEngine>::DataType[2]) / 2;
      } else if (mftypekey == MFEnum_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFEnum>::DataType[2]) / 2;
      } else if (mftypekey == MFFloat_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFFloat>::DataType[2]) / 2;
      } else if (mftypekey == MFInt32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFInt32>::DataType[2]) / 2;
      } else if (mftypekey == MFMatrix_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFMatrix>::DataType[2]) / 2;
      } else if (mftypekey == MFName_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFName>::DataType[2]) / 2;
      } else if (mftypekey == MFNode_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFNode>::DataType[2]) / 2;
      } else if (mftypekey == MFPlane_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFPlane>::DataType[2]) / 2;
      } else if (mftypekey == MFRotation_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFRotation>::DataType[2]) / 2;
      } else if (mftypekey == MFShort_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFShort>::DataType[2]) / 2;
      } else if (mftypekey == MFString_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFString>::DataType[2]) / 2;
        // FIXME: should we add strlen(str[i]) to memory size? would
        // potentially take some time to compute. 20080302 larsa
        //for (int c = 0; c < numelements; ++c) {
        //  add strlen(string[c]) + 1 to memory size?
        //}
      } else if (mftypekey == MFTime_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFTime>::DataType[2]) / 2;
      } else if (mftypekey == MFUInt32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFUInt32>::DataType[2]) / 2;
      } else if (mftypekey == MFUShort_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFUShort>::DataType[2]) / 2;
      } else if (mftypekey == MFVec2b_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec2b>::DataType[2]) / 2;
      } else if (mftypekey == MFVec2d_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec2d>::DataType[2]) / 2;
      } else if (mftypekey == MFVec2f_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec2f>::DataType[2]) / 2;
      } else if (mftypekey == MFVec2i32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec2i32>::DataType[2]) / 2;
      } else if (mftypekey == MFVec2s_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec2s>::DataType[2]) / 2;
      } else if (mftypekey == MFVec3b_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec3b>::DataType[2]) / 2;
      } else if (mftypekey == MFVec3d_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec3d>::DataType[2]) / 2;
      } else if (mftypekey == MFVec3f_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec3f>::DataType[2]) / 2;
      } else if (mftypekey == MFVec3i32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec3i32>::DataType[2]) / 2;
      } else if (mftypekey == MFVec3s_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec3s>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4b_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4b>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4d_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4d>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4f_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4f>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4i32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4i32>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4s_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4s>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4ub_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4ub>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4ui32_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4ui32>::DataType[2]) / 2;
      } else if (mftypekey == MFVec4us_string.getString()) {
        elementsize = sizeof(SbTypeInfo<SoMFVec4us>::DataType[2]) / 2;
      } else {
        // unsupported field type
        elementsize = -1;
      }

#undef SBNAMESTRING

      if (mfield->isDeleteValuesEnabled()) {
        // assume this is a self-managed multi-field
        managed += elementsize * numelements;
      } else {
        // assume setValuesPointer() has been used
        unmanaged += elementsize * numelements;
      }
    }
  }
} // getMFieldsDataSize()

/*!
  Set a generic user data pointer for this field container.

  This function can be used by the application programmer to, for
  instance, store a pointer to an application specific data structure
  that is in some way related to the field container. getUserData()
  can later be used to retrieve the pointer.

  \sa getUserData()
  \since Coin 2.0
*/
void
SoFieldContainer::setUserData(void * userdata) const
{
  (void)sofieldcontainer_userdata_dict->put(this, userdata);
}

/*!
  Return the generic user data pointer for this field container, or
  NULL if no user data have been set.

  \sa setUserData()
  \since Coin 2.0
*/
void *
SoFieldContainer::getUserData(void) const
{
  void * tmp = NULL;
  if (sofieldcontainer_userdata_dict->get(this, tmp)) {
    return tmp;
  }
  return NULL;
}

#undef FLAG_DONOTIFY
#undef FLAG_FIRSTINSTANCE
