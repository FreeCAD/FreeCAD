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
  \class SoBase SoBase.h Inventor/misc/SoBase.h
  \brief The SoBase class is the top-level superclass for a number
  of class-hierarchies.

  \ingroup coin_general

  SoBase provides the basic interfaces and methods for doing reference
  counting, type identification and import/export. All classes in Coin
  which uses these mechanisms are descendent from this class.

  One important issue with SoBase-derived classes is that they should
  \e not be statically allocated, neither in static module memory nor
  on function's stack frames. SoBase-derived classes must \e always be
  allocated dynamically from the memory heap with the \c new operator.

  This is so because SoBase-derived instances are reference counted,
  and will self destruct on the appropriate time. For this to work,
  they must be explicitly allocated in heap memory. See the class
  documentation of SoNode for more information.
*/

// *************************************************************************

// FIXME: There's a lot of methods in SoBase used to implement VRML
// support which are missing.
//
// UPDATE 20020217 mortene: is this FIXME still correct?

// FIXME: One more thing missing: detect cases where we should
// instantiate SoUnknownEngine instead of SoUnknownNode.

// *************************************************************************

#include <Inventor/misc/SoBase.h>

#include <cassert>
#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/lists/SoBaseList.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/sensors/SoDataSensor.h>

#include "misc/SoBaseP.h"
#include "nodes/SoUnknownNode.h"
#include "fields/SoGlobalField.h"
#include "misc/SbHash.h"
#include "upgraders/SoUpgrader.h"
#include "threads/threadsutilp.h"
#include "tidbitsp.h"
#include "io/SoInputP.h"
#include "io/SoWriterefCounter.h"
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// *************************************************************************

// Note: the following documentation for getTypeId() will also be
// visible for subclasses, so keep it general.
/*!
  \fn SoType SoBase::getTypeId(void) const

  \brief Returns the type identification of an object derived from a
  class inheriting SoBase. This is used for runtime type checking and
  "downward" casting.

  Usage example:

  \code
  void foo(SoNode * node)
  {
    if (node->getTypeId() == SoFile::getClassTypeId()) {
      SoFile * filenode = (SoFile *)node;  // safe downward cast, knows the type
      /// [then something] ///
    }
  }
  \endcode


  For application programmers wanting to extend the library with new
  nodes, engines, nodekits, draggers or others: this method needs to
  be overridden in \e all subclasses. This is typically done as part
  of setting up the full type system for extension classes, which is
  usually accomplished by using the predefined macros available
  through for instance Inventor/nodes/SoSubNode.h (SO_NODE_INIT_CLASS
  and SO_NODE_CONSTRUCTOR for node classes),
  Inventor/engines/SoSubEngine.h (for engine classes) and so on.

  For more information on writing Coin extensions, see the class
  documentation of the top level superclasses for the various class
  groups.
*/

// Note: the following documentation for readInstance() will also be
// visible for subclasses, so keep it general.
/*!
  \fn SbBool SoBase::readInstance(SoInput * in, unsigned short flags)

  This method is mainly intended for internal use during file import
  operations.

  It reads a definition of an instance from the input stream \a in.
  The input stream state points to the start of a serialized /
  persistent representation of an instance of this class type.

  \c TRUE or \c FALSE is returned, depending on if the instantiation
  and configuration of the new object of this class type went OK or
  not.  The import process should be robust and handle corrupted input
  streams by returning \c FALSE.

  \a flags is used internally during binary import when reading user
  extension nodes, group nodes or engines.
*/

/*!
  \enum SoBase::BaseFlags
  \COININTERNAL
*/

// *************************************************************************

SoType SoBase::classTypeId STATIC_SOTYPE_INIT;

/**********************************************************************/

// This can be any "magic" bitpattern of 4 bits which seems unlikely
// to be randomly assigned to a memory byte upon destruction. I chose
// "1101".
//
// The 4 bits allocated for the "alive" bitpattern are used in
// SoBase::ref() to try to detect when the instance has been
// prematurely destructed.
//
// <mortene@sim.no>
#define ALIVE_PATTERN 0xd

unsigned int SbHashFunc(const SoBase * key) {
  return SbHashFunc(reinterpret_cast<size_t>(key));
}

/*!
  Constructor. The initial reference count will be set to zero.
 */
SoBase::SoBase(void)
{
  COIN_CHECK_THREAD();

  // It is a common mistake to place e.g. nodes as static member
  // variables, or on the main()-function's stack-frame. This catches
  // some (but not all) of those cases.
  //
  // FIXME: we could probably add in an MSWin-specific extra check
  // here for instances placed dynamically on a stack, using Win32 API
  // functions that can classify a memory pointer as e.g. heap or
  // stack. 20031018 mortene.
  assert((SoBase::classTypeId != SoType::badType()) &&
         "An SoBase-derived class was attempted instantiated *before* Coin initialization. (Have you perhaps placed an SoBase-derived instance (e.g. a scene graph node) in non-heap memory?) See SoBase class documentation for more info.");

  cc_rbptree_init(&this->auditortree);

  this->objdata.referencecount = 0;

  // For debugging -- we try to catch dangling references after
  // premature destruction. See the SoBase::assertAlive() method for
  // further doc.
  this->objdata.alive = ALIVE_PATTERN;

  // For debugging, store a pointer to all SoBase-instances.
#if COIN_DEBUG
  if (SoBase::PImpl::trackbaseobjects) {
    CC_MUTEX_LOCK(SoBase::PImpl::allbaseobj_mutex);
    //void * dummy;
    assert(SoBase::PImpl::allbaseobj->find(this)==SoBase::PImpl::allbaseobj->const_end());
    (*SoBase::PImpl::allbaseobj)[this]=NULL;
    CC_MUTEX_UNLOCK(SoBase::PImpl::allbaseobj_mutex);
  }
#endif // COIN_DEBUG
}

/*!
  Destructor. There should not be any normal circumstance where you need
  to explicitly delete an object derived from the SoBase class, as the
  reference counting should take care of deallocating unused objects.

  \sa unref(), unrefNoDelete()
 */
SoBase::~SoBase()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::~SoBase", "%p", this);
#endif // debug

  // Set the 4 bits of bitpattern to anything but the "magic" pattern
  // used to check that we are still alive.
  this->objdata.alive = (~ALIVE_PATTERN) & 0xf;

  if (SoBase::PImpl::auditordict) {
    //SoAuditorList * l;
    if (SoBase::PImpl::auditordict->find(this)!=SoBase::PImpl::auditordict->const_end()) {
      SoBase::PImpl::auditordict->erase(this);
      //delete l;
    }
  }
  cc_rbptree_clean(&this->auditortree);

#if COIN_DEBUG
  if (SoBase::PImpl::trackbaseobjects) {
    CC_MUTEX_LOCK(SoBase::PImpl::allbaseobj_mutex);
    const size_t ok = SoBase::PImpl::allbaseobj->erase(this);
    assert(ok && "something fishy going on in debug object tracking");
    CC_MUTEX_UNLOCK(SoBase::PImpl::allbaseobj_mutex);
  }
#endif // COIN_DEBUG
}

//
// callback from auditortree that is used to add sensor
// auditors to the list (closure).
//
static void
sobase_sensor_add_cb(void * auditor, void * type, void * closure)
{
  SbList<SoDataSensor *> * auditingsensors =
    (SbList<SoDataSensor*> *) closure;

  // MSVC7 on 64-bit Windows wants to go through this type when
  // casting from void*.
  const uintptr_t tmp = (uintptr_t)type;
  switch ((SoNotRec::Type) tmp) {
  case SoNotRec::SENSOR:
    auditingsensors->append((SoDataSensor *)auditor);
    break;

  case SoNotRec::FIELD:
  case SoNotRec::ENGINE:
  case SoNotRec::CONTAINER:
  case SoNotRec::PARENT:
    // FIXME: should any of these get special treatment? 20000402 mortene.
    break;

  default:
    assert(0 && "Unknown auditor type");
  }
}

/*!
  Cleans up all hanging references to and from this instance, and then
  commits suicide.

  Called automatically when the reference count goes to zero.
*/
void
SoBase::destroy(void)
{
  SbName name = this->getName();
#if COIN_DEBUG && 0 // debug
  SoType t = this->getTypeId();
  SoDebugError::postInfo("SoBase::destroy", "start -- %p '%s' ('%s')",
                         this, t.getName().getString(), name.getString());
#endif // debug


#if COIN_DEBUG
  if (SoBase::PImpl::tracerefs) {
    SoDebugError::postInfo("SoBase::destroy",
                           "%p ('%s')",
                           this, this->getTypeId().getName().getString());
  }
#endif // COIN_DEBUG

  // Find all auditors that they need to cut off their link to this
  // object. I believe this is necessary only for sensors.
  SbList<SoDataSensor *> auditingsensors;
  cc_rbptree_traverse(&this->auditortree, (cc_rbptree_traversecb *)sobase_sensor_add_cb, &auditingsensors);

  // Notify sensors that we're dying.
  for (int j = 0; j < auditingsensors.getLength(); j++)
    auditingsensors[j]->dyingReference();

  // Link out instance name from the list of all SoBase instances.
  if (name != SbName::empty()) SoBase::PImpl::removeName2Obj(this, name.getString());

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::destroy", "delete this %p", this);
#endif // debug

  // Harakiri!
  delete this;

  // Link out obj-pointer to name reference now that object is dead.
  if (name != SbName::empty()) SoBase::PImpl::removeObj2Name(this, name.getString());

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::destroy", "done -- %p '%s' ('%s')",
                         this, t.getName().getString(), name.getString());
#endif // debug
}

/*!
  \brief Sets up initialization for data common to all instances of
  this class, like submitting necessary information to the Coin type
  system.
 */
void
SoBase::initClass(void)
{
  // check_for_leaks() goes through the allocation list, and checks if
  // all allocated SoBase-derived instances was deallocated before the
  // atexit-routines were run.
  //
  // Set up to run after most other atexit-code, since we depend on
  // Coin cleaning up internal nodes etc (like the static
  // sub-scene graphs in draggers).
  //
  // -mortene.
  coin_atexit((coin_atexit_f *)SoBase::PImpl::check_for_leaks, CC_ATEXIT_TRACK_SOBASE_INSTANCES);

  coin_atexit((coin_atexit_f *)SoBase::cleanClass, CC_ATEXIT_SOBASE);

  // Avoid multiple attempts at initialization.
  assert(SoBase::classTypeId == SoType::badType());

  SoBase::classTypeId = SoType::createType(SoType::badType(), "Base");

  SoBase::PImpl::name2obj = new SbHash<const char *, SbPList *>;
  SoBase::PImpl::obj2name = new SbHash<const SoBase *, const char *>();
  SoBase::PImpl::refwriteprefix = new SbString("+");
  SoBase::PImpl::allbaseobj = new SoBaseSet;

  CC_MUTEX_CONSTRUCT(SoBase::PImpl::mutex);
  CC_MUTEX_CONSTRUCT(SoBase::PImpl::obj2name_mutex);
  CC_MUTEX_CONSTRUCT(SoBase::PImpl::name2obj_mutex);
  CC_MUTEX_CONSTRUCT(SoBase::PImpl::allbaseobj_mutex);
  CC_MUTEX_CONSTRUCT(SoBase::PImpl::auditor_mutex);
  CC_MUTEX_CONSTRUCT(SoBase::PImpl::global_mutex);

  // debug
  const char * str = coin_getenv("COIN_DEBUG_TRACK_SOBASE_INSTANCES");
  SoBase::PImpl::trackbaseobjects = str && atoi(str) > 0;

  SoWriterefCounter::initClass();
}

// Clean up all commonly allocated resources before application
// exit. Only for debugging purposes.
void
SoBase::cleanClass(void)
{
  assert(SoBase::PImpl::name2obj);
  assert(SoBase::PImpl::obj2name);

  // Delete the SbPLists in the dictionaries.
  for(
      SbHash<const char *, SbPList *>::const_iterator iter =
       SoBase::PImpl::name2obj->const_begin();
      iter!=SoBase::PImpl::name2obj->const_end();
      ++iter
      ) {
    delete iter->obj;
  }

  delete SoBase::PImpl::allbaseobj; SoBase::PImpl::allbaseobj = NULL;

  delete SoBase::PImpl::name2obj; SoBase::PImpl::name2obj = NULL;
  delete SoBase::PImpl::obj2name; SoBase::PImpl::obj2name = NULL;

  delete SoBase::PImpl::refwriteprefix; SoBase::PImpl::refwriteprefix = NULL;

  SoBase::classTypeId STATIC_SOTYPE_INIT;

  CC_MUTEX_DESTRUCT(SoBase::PImpl::mutex);
  CC_MUTEX_DESTRUCT(SoBase::PImpl::obj2name_mutex);
  CC_MUTEX_DESTRUCT(SoBase::PImpl::allbaseobj_mutex);
  CC_MUTEX_DESTRUCT(SoBase::PImpl::name2obj_mutex);
  CC_MUTEX_DESTRUCT(SoBase::PImpl::auditor_mutex);
  CC_MUTEX_DESTRUCT(SoBase::PImpl::global_mutex);

  SoBase::PImpl::tracerefs = FALSE;
  SoBase::PImpl::writecounter = 0;
}

/*!
  \COININTERNAL

  There are 4 bits allocated for each SoBase-object for a bitpattern
  that indicates the object is still "alive". The pattern is changed
  when the object is destructed. If this method is then called after
  destruction, an assert will hit.

  This is used internally in Coin (in for instance SoBase::ref()) to
  try to detect when the instance has been prematurely
  destructed. This is a very common mistake made by application
  programmers (letting the refcount drop to zero before it should, that
  is), so the extra piece of assistance through the accompanying
  assert() in this method to detect dangling references to the object,
  with subsequent memory corruption and mysterious crashes, should be
  a Good Thing.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoBase::assertAlive(void) const
{
  if (this->objdata.alive != ALIVE_PATTERN) {
    SoDebugError::post("SoBase::assertAlive",
                       "Detected an attempt to access an instance (%p) of an "
                       "SoBase-derived class after it was destructed!  "
                       "This is most likely to be the result of some grave "
                       "programming error in the application / client "
                       "code (or less likely: internal library code), "
                       "causing premature destruction of a reference "
                       "counted object instance. This check was called "
                       "from a dangling reference to it.", this);
    assert(FALSE && "SoBase-object no longer alive!");
  }
}

/*!
  Increase the reference count of the object. This might be necessary
  to do explicitly from user code for certain situations (mainly to
  avoid premature deletion), but is usually called from other instances
  within the Coin library when objects are made dependent on each other.

  See the class documentation of SoNode for more extensive information
  about reference counting.

  \sa unref(), unrefNoDelete()
 */
void
SoBase::ref(void) const
{
  COIN_CHECK_THREAD();

  if (COIN_DEBUG) this->assertAlive();

  CC_MUTEX_LOCK(SoBase::PImpl::mutex);
#if COIN_DEBUG
  int32_t currentrefcount = this->objdata.referencecount;
#endif // COIN_DEBUG
  this->objdata.referencecount++;
  CC_MUTEX_UNLOCK(SoBase::PImpl::mutex);

#if COIN_DEBUG
  if (this->objdata.referencecount < currentrefcount) {
    SoDebugError::post("SoBase::ref",
                       "%p ('%s') - referencecount overflow!: %d -> %d",
                       this, this->getTypeId().getName().getString(),
                       currentrefcount, this->objdata.referencecount);

    // The reference counter is contained within 27 bits of signed
    // integer, which means it can go up to about ~67 million
    // references. It's hard to imagine that this should be too small,
    // so we don't bother to try to handle overflows any better than
    // this.
    //
    // If we should ever revert this decision, look in Coin-1 for how
    // to handle overflows graciously.
    assert(FALSE && "reference count overflow");
  }
#endif // COIN_DEBUG

#if COIN_DEBUG
  if (SoBase::PImpl::tracerefs) {
    SoDebugError::postInfo("SoBase::ref",
                           "%p ('%s') - referencecount: %d",
                           this, this->getTypeId().getName().getString(),
                           this->objdata.referencecount);
  }
#endif // COIN_DEBUG
}

/*!
  Decrease the reference count of an object. If the reference count
  reaches zero, the object will delete itself. Be careful when
  explicitly calling this method, beware that you usually need to
  match user level calls to ref() with calls to either unref() or
  unrefNoDelete() to avoid memory leaks.

  \sa ref(), unrefNoDelete()
*/
void
SoBase::unref(void) const
{
  COIN_CHECK_THREAD();

  if (COIN_DEBUG) this->assertAlive();

  CC_MUTEX_LOCK(SoBase::PImpl::mutex);
  this->objdata.referencecount--;
  int refcount = this->objdata.referencecount;

  CC_MUTEX_UNLOCK(SoBase::PImpl::mutex);

#if COIN_DEBUG
  if (SoBase::PImpl::tracerefs) {
    SoDebugError::postInfo("SoBase::unref",
                           "%p ('%s') - referencecount: %d",
                           this, this->getTypeId().getName().getString(),
                           this->objdata.referencecount);
  }
  if (refcount < 0) {
    // Do the debug output in two calls, since the getTypeId() might
    // cause a crash, and then there'd be no output at all with a
    // single SoDebugError::postWarning() call.
    SoDebugError::postWarning("SoBase::unref", "ref count less than zero");
    SoDebugError::postWarning("SoBase::unref", "...for %s instance at %p",
                              this->getTypeId().getName().getString(), this);
  }
#endif // COIN_DEBUG
  if (refcount == 0) {
    SoBase * base = const_cast<SoBase *>(this);
    base->destroy();
  }
}

/*!
  Decrease reference count, but do \e not delete ourself if the count
  reaches zero.

  \sa unref()
 */
void
SoBase::unrefNoDelete(void) const
{
  COIN_CHECK_THREAD();

  if (COIN_DEBUG) this->assertAlive();

  this->objdata.referencecount--;
#if COIN_DEBUG
  if (SoBase::PImpl::tracerefs) {
    SoDebugError::postInfo("SoBase::unrefNoDelete",
                           "%p ('%s') - referencecount: %d",
                           this, this->getTypeId().getName().getString(),
                           this->objdata.referencecount);
  }
#endif // COIN_DEBUG
}

/*!
  Returns number of objects referring to this object.
*/
int32_t
SoBase::getRefCount(void) const
{
  return this->objdata.referencecount;
}

/*!
  Force an update, in the sense that all objects connected to this
  object as an auditor will have to re-check the values of their
  inter-dependent data.

  This is often used as an effective way of manually triggering a
  redraw by application programmers.
*/
void
SoBase::touch(void)
{
  this->startNotify();
}

/*!
  \brief Returns \c TRUE if the type of this object is either of the
  same type or inherited from \a type.

  This is used for runtime type checking and "downward" casting.

  Usage example:

  \code
  void foo(SoNode * node)
  {
    if (node->isOfType(SoGroup::getClassTypeId())) {
      SoGroup * group = (SoGroup *)node;  // safe downward cast, knows the type
      /// [then something ] ///
    }
  }
  \endcode

 */
SbBool
SoBase::isOfType(SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

/*!
  \brief This static method returns the SoType object associated with
  objects of this class.
 */
SoType
SoBase::getClassTypeId(void)
{
  return SoBase::classTypeId;
}

/*!
  Returns name of object. If no name has been given to this object,
  the returned SbName will contain the empty string.
 */
SbName
SoBase::getName(void) const
{
  // If this assert fails, SoBase::initClass() has probably not been
  // called, or you have objects on the stack that is destroyed after
  // you have invoked SoDB::cleanup().
  assert(SoBase::PImpl::obj2name);

  //const char * value = NULL;
  CC_MUTEX_LOCK(SoBase::PImpl::obj2name_mutex);
  SbHash<const SoBase *, const char *>::const_iterator tmp = SoBase::PImpl::obj2name->find(this);
  SbBool found = (tmp != SoBase::PImpl::obj2name->const_end());
  CC_MUTEX_UNLOCK(SoBase::PImpl::obj2name_mutex);
  return SbName(found ? tmp->obj : "");
}

/*!
  Set the name of this object.

  Some characters are invalid to use as parts of names for SoBase
  derived objects, as object names needs to be consistent with the
  syntax of Inventor and VRML files upon file export / import
  operations (so one must for instance avoid using special token
  characters).

  Invalid characters will be automatically replaced by underscore
  characters. If the name \e starts with an invalid character, the new
  name will be \e preceded by an underscore character.

  For the exact definitions of what constitutes legal and illegal
  characters for SoBase names, see the SbName functions listed below.

  \sa getName(), SbName::isBaseNameStartChar(), SbName::isBaseNameChar()
*/
void
SoBase::setName(const SbName & newname)
{

  // This may look peculiar, but it is useful in combination with the
  // COIN_DEBUG_TRACK_SOBASE_INSTANCES envvar to track down where
  // un-deallocated SoBase-instances were allocated from. (I.e., run it
  // in a debugger and check the backtrace.)  -mortene.
#if 0 // debug
  static SbBool checked = FALSE;
  static const char * tracename = NULL;
  if (!checked) {
    tracename = coin_getenv("COIN_DEBUG_ASSERT_SOBASE_SETNAME");
    checked = TRUE;
  }
  if (tracename) { assert(newname != tracename); }
#endif // debug


  // remove old name first
  SbName oldName = this->getName();
  if (oldName != SbName::empty()) SoBase::removeName(this, oldName.getString());

  // semantics in the original SGI Inventor is to not build a separate
  // name list for unnamed SoBase instances
  if (newname == SbName::empty()) { return; }

  // check for bad characters
  const char * str = newname.getString();
  SbBool isbad = !SbName::isBaseNameStartChar(str[0]);

  int i;
  const int newnamelen = newname.getLength();
  for (i = 1; i < newnamelen && !isbad; i++) {
    isbad = !SbName::isBaseNameChar(str[i]);
  }

  if (isbad) {
    // replace bad characters
    SbString goodstring;

    if (!SbName::isBaseNameStartChar(str[0])) goodstring += '_';

    for (i = 0; i < newnamelen; i++) {
      goodstring += SbName::isBaseNameChar(str[i]) ? str[i] : '_';
    }

#if COIN_DEBUG
    SoDebugError::postWarning("SoBase::setName", "Bad characters in "
                              "name '%s'. Replacing with name '%s'",
                              str, goodstring.getString());
#endif // COIN_DEBUG

    SoBase::addName(this, SbName(goodstring.getString()));
  }
  else {
    SoBase::addName(this, newname.getString());
  }
}

/*!
  Adds a name<->object mapping to the global dictionary.
 */
void
SoBase::addName(SoBase * const b, const char * const name)
{
  assert(name);

  SbPList * l;
  CC_MUTEX_LOCK(SoBase::PImpl::name2obj_mutex);
  SbHash<const char*, SbPList*>::const_iterator tmp = SoBase::PImpl::name2obj->find(name);
  if (tmp==SoBase::PImpl::name2obj->const_end()) {
    // name not used before, create new list
    l = new SbPList;
    (*SoBase::PImpl::name2obj)[name] = l;
  }
  else {
    l = tmp->obj;
  }

  // append this to the list
  l->append(b);
  CC_MUTEX_UNLOCK(SoBase::PImpl::name2obj_mutex);

  CC_MUTEX_LOCK(SoBase::PImpl::obj2name_mutex);
  // set name of object. SbHash::put() will overwrite old name
  (*SoBase::PImpl::obj2name)[b] = name;
  CC_MUTEX_UNLOCK(SoBase::PImpl::obj2name_mutex);
}

/*!
  Removes a name<->object mapping from the global dictionary.
*/
void
SoBase::removeName(SoBase * const base, const char * const name)
{
  SoBase::PImpl::removeObj2Name(base, name);
  SoBase::PImpl::removeName2Obj(base, name);
}

/*!
  This is the method which starts the notification sequence
  after changes.

  At the end of a notification sequence, all "immediate" sensors
  (i.e. sensors set up with a zero priority) are triggered.
*/
void
SoBase::startNotify(void)
{
  SoNotList l;
  SoNotRec rec(createNotRec());
  l.append(&rec);
  l.setLastType(SoNotRec::CONTAINER);

  SoDB::startNotify();
  this->notify(&l);
  SoDB::endNotify();
}

/*!
  Notifies all auditors for this instance when changes are made.
*/
void
SoBase::notify(SoNotList * l)
{
  if (COIN_DEBUG) this->assertAlive();

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::notify", "base %p, list %p", this, l);
#endif // debug

  SoBase::PImpl::NotifyData notdata;
  notdata.cnt = cc_rbptree_size(&this->auditortree);
  notdata.list = l;
  notdata.thisp = this;

  cc_rbptree_traverse(&this->auditortree, (cc_rbptree_traversecb *)SoBase::PImpl::rbptree_notify_cb, &notdata);
  assert(notdata.cnt == 0);
}

/*!
  Add an auditor to notify whenever the object changes in any significant
  way.

  \sa removeAuditor()
 */
void
SoBase::addAuditor(void * const auditor, const SoNotRec::Type type)
{
  // MSVC7 on 64-bit Windows wants to go through this type before
  // casting to void*.
  const uintptr_t val = (uintptr_t)type;
  cc_rbptree_insert(&this->auditortree, auditor, (void *)val);
}

/*!
  Remove an auditor from the list. \a auditor will then no longer be
  notified whenever any updates are made to this object.

  \sa addAuditor()
*/
void
SoBase::removeAuditor(void * const auditor, const SoNotRec::Type COIN_UNUSED_ARG(type))
{
  cc_rbptree_remove(&this->auditortree, auditor);
}


static void
sobase_audlist_add(void * pointer, void * type, void * closure)
{
  SoAuditorList * l = (SoAuditorList*) closure;
  // MSVC7 on 64-bit Windows wants to go through this type before
  // casting to void*.
  const uintptr_t tmp = (uintptr_t)type;
  l->append(pointer, (SoNotRec::Type)tmp);
}

/*!
  Returns list of objects auditing this object.

  \sa addAuditor(), removeAuditor()
*/
const SoAuditorList &
SoBase::getAuditors(void) const
{
  CC_MUTEX_LOCK(SoBase::PImpl::auditor_mutex);

  if (SoBase::PImpl::auditordict == NULL) {
    SoBase::PImpl::auditordict = new SbHash<const SoBase *, SoAuditorList *>();
    coin_atexit((coin_atexit_f*)SoBase::PImpl::cleanup_auditordict, CC_ATEXIT_NORMAL);
  }

  SoAuditorList * l = NULL;
  SbHash<const SoBase *, SoAuditorList *>::const_iterator iter
    = 
    SoBase::PImpl::auditordict->find(this);
  if (iter!=SoBase::PImpl::auditordict->const_end()) {
    l = iter->obj;
    // empty list before copying in new values
    for (int i = 0; i < l->getLength(); i++) {
      l->remove(i);
    }
  }
  else {
    (*SoBase::PImpl::auditordict)[this] = new SoAuditorList;
  }
  cc_rbptree_traverse(&this->auditortree, (cc_rbptree_traversecb*)sobase_audlist_add, l);

  CC_MUTEX_UNLOCK(SoBase::PImpl::auditor_mutex);

  return *l;
}

/*!
  This method is used during the first write pass of a write action to
  count the number of references to this object in the scene graph.
*/
void
SoBase::addWriteReference(SoOutput * out, SbBool isfromfield)
{
  assert(out->getStage() == SoOutput::COUNT_REFS);

  int refcount = SoWriterefCounter::instance(out)->getWriteref(this);

#if COIN_DEBUG
  if (SoWriterefCounter::debugWriterefs()) {
    SoDebugError::postInfo("SoBase::addWriteReference",
                           "%p/%s/'%s': %d -> %d",
                           this, this->getTypeId().getName().getString(),
                           this->getName().getString(),
                           refcount, refcount + 1);
  }
#endif // COIN_DEBUG

  refcount++;

  if (!isfromfield) {
    SoWriterefCounter::instance(out)->setInGraph(this, TRUE);
  }
  SoWriterefCounter::instance(out)->setWriteref(this, refcount);
}

/*!
  Returns \c TRUE if this object should be written out during a write action.
  Will return \c FALSE if no references to this object have been made in the
  scene graph.

  Note that connections from the fields of field container objects is not
  alone a valid reason for writing out the object -- there must also be at
  least one reference directly from another SoBase (like a parent/child
  relationship, for instance).

  This method will return a valid result only during the second pass of
  write actions.
*/
SbBool
SoBase::shouldWrite(void)
{
  return SoWriterefCounter::instance(NULL)->shouldWrite(this);
}

/*!
  \COININTERNAL
*/
void
SoBase::incrementCurrentWriteCounter(void)
{
  ++SoBase::PImpl::writecounter;
}

/*!
  \COININTERNAL
*/
void
SoBase::decrementCurrentWriteCounter(void)
{
  --SoBase::PImpl::writecounter;
}

/*!
  Returns the object of \a type, or derived from \a type, registered
  under \a name. If several objects have been registered under the same name
  with the same type, returns the \e last one which was registered.

  If no object of a valid type or subtype has been registered with the
  given name, returns \c NULL.
*/
SoBase *
SoBase::getNamedBase(const SbName & name, SoType type)
{
  CC_MUTEX_LOCK(SoBase::PImpl::name2obj_mutex);
  SbHash<const char*, SbPList*>::const_iterator iter = 
    SoBase::PImpl::name2obj->find((const char *)name);
  if (iter!=SoBase::PImpl::name2obj->const_end()) {
    SbPList * l = iter->obj;
    if (l->getLength()) {
      SoBase * b = (SoBase *)((*l)[l->getLength() - 1]);
      if (b->isOfType(type)) {
        CC_MUTEX_UNLOCK(SoBase::PImpl::name2obj_mutex);
        return b;
      }
    }
  }
  CC_MUTEX_UNLOCK(SoBase::PImpl::name2obj_mutex);
  return NULL;
}

/*!
  Returns the number of objects of type \a type, or derived from \a type,
  registered under \a name.

  All matches will also be appended to \a baselist.
 */
int
SoBase::getNamedBases(const SbName & name, SoBaseList & baselist, SoType type)
{
  CC_MUTEX_LOCK(SoBase::PImpl::name2obj_mutex);

  int matches = 0;

  SbHash<const char*, SbPList*>::const_iterator iter = 
    SoBase::PImpl::name2obj->find((const char *)name);
  if (iter!=SoBase::PImpl::name2obj->const_end()) {
    SbPList * l = iter->obj;
    
    for (int i=0; i < l->getLength(); i++) {
      SoBase * b = (SoBase *)((*l)[i]);
      if (b->isOfType(type)) {
        baselist.append(b);
        matches++;
      }
    }
  }
  CC_MUTEX_UNLOCK(SoBase::PImpl::name2obj_mutex);

  return matches;
}

/*!
  Read next SoBase derived instance from the \a in stream, check that
  it is derived from \a expectedtype and place a pointer to the newly
  allocated instance in \a base.

  \c FALSE is returned on read errors, mismatch with the \a
  expectedtype, or if there are attempts at referencing (through the
  \c USE keyword) unknown instances.

  If we return \c TRUE with \a base equal to \c NULL, three things
  might have happened:

  1. End-of-file. Use SoInput::eof() after calling this method to
  detect end-of-file conditions.

  2. \a in didn't have a valid identifier name at the stream for us to
  read. This happens either in the case of errors, or when all child
  nodes of a group have been read. Check if the next character in the
  stream is a '}' to detect the latter case.

  3. A child was given as the \c NULL keyword. This can happen when
  reading the contents of SoSFNode fields (note that NULL is not
  allowed for SoMFNode)

  If \c TRUE is returned and \a base is not \c NULL upon return, the
  instance was allocated and initialized according to what was read
  from the \a in stream.
*/
SbBool
SoBase::read(SoInput * in, SoBase *& base, SoType expectedtype)
{
  // FIXME: the interface design for this function is goddamn _awful_!
  // We need to keep it like this, though, to be compatible with SGI
  // Inventor. What we however /could/ do about it is:
  //
  // First, split out the SoBase::PImpl class definition to a separate
  // interface, which can be accessed internally from Coin library
  // code.
  //
  // Then, in this, write up /sensibly designed/ read()-function(s)
  // which implements the actually needed functionality of this
  // method.
  //
  // Third, make this function use those new functions in SoBase::PImpl (to
  // avoid code duplication) -- and mangle the results so that this
  // function still conforms to the SGI Inventor behavior.
  //
  // Finally, start using the SoBase::PImpl::read()-function(s) from
  // internal Coin code instead, to clean up the messy interaction
  // with this function from everywhere else.
  //
  // 20060202 mortene.

  assert(expectedtype != SoType::badType());
  base = NULL;

  SbName name;
  SbBool result = in->read(name, TRUE);

#if COIN_DEBUG
  if (SoInputP::debug()) {
    // This output is extremely useful when debugging the import code.
    SoDebugError::postInfo("SoBase::read",
                           "SoInput::read(&name, TRUE) => returns %s, name=='%s'",
                           result ? "TRUE" : "FALSE", name.getString());
  }
#endif // COIN_DEBUG

  // read all (vrml97) routes. Do this also for non-vrml97 files,
  // since in Coin we can have a mix of Inventor and VRML97 nodes in
  // the same file.
  while (result && name == PImpl::ROUTE_KEYWORD) {
    result = SoBase::readRoute(in);
    // read next ROUTE keyword
    if (result) result = in->read(name, TRUE);
    else return FALSE; // error while reading ROUTE
  }

  // The SoInput stream does not start with a valid base name. Return
  // TRUE with base==NULL.
  if (!result) return TRUE;

  // If no valid name / identifier string is found, the return value
  // from SbInput::read(SbName&,TRUE) _should_ also be FALSE.
  assert(name != "");

  if (name == PImpl::USE_KEYWORD) result = SoBase::PImpl::readReference(in, base);
  else if (name == PImpl::NULL_KEYWORD) return TRUE;
  else result = SoBase::PImpl::readBase(in, name, base);

  // Check type correctness.
  if (result) {
    assert(base);

    SoType type = base->getTypeId();
    assert(type != SoType::badType());

    if (!type.isDerivedFrom(expectedtype)) {
      SoReadError::post(in, "Type '%s' is not derived from '%s'",
                        type.getName().getString(),
                        expectedtype.getName().getString());
      result = FALSE;
    }
  }

  // Make sure we don't leak memory.
  if (!result && base && (name != PImpl::USE_KEYWORD)) {
    base->ref();
    base->unref();
  }

#if COIN_DEBUG
  if (SoInputP::debug()) {
    SoDebugError::postInfo("SoBase::read", "done, name=='%s' baseptr==%p, result==%s",
                           name.getString(), base, result ? "TRUE" : "FALSE");
  }
#endif // COIN_DEBUG

  return result;
}

/*!
  Referenced instances of SoBase are written as "DEF NamePrefixNumber" when
  exported. "Name" is the name of the base instance from setName(), "Prefix"
  is common for all objects and can be set by this method (default is "+"),
  and "Number" is a unique id which is necessary if multiple objects have
  the same name.

  If you want the prefix to be something else than "+", use this method.
 */
void
SoBase::setInstancePrefix(const SbString & c)
{
  SoWriterefCounter::setInstancePrefix(c);
  (*SoBase::PImpl::refwriteprefix) = c;
}

/*!
  Set to \c TRUE to activate debugging of reference counting, which
  could aid in finding hard to track down problems with accesses to
  freed memory or memory leaks. Note: this will produce lots of
  debug information in any "normal" running system, so use sensibly.

  The reference tracing functionality will be disabled in "release
  versions" of the Coin library.
 */
void
SoBase::setTraceRefs(SbBool trace)
{
  SoBase::PImpl::tracerefs = trace;
}

/*!
  Return the status of the reference tracing flag.

  \sa setTraceRefs()
 */
SbBool
SoBase::getTraceRefs(void)
{
  return SoBase::PImpl::tracerefs;
}

/*!
  Returns \c TRUE if this object will be written more than once upon
  export. Note that the result from this method is only valid during the
  second pass of a write action (and partially during the COUNT_REFS pass).
 */
SbBool
SoBase::hasMultipleWriteRefs(void) const
{
  return SoWriterefCounter::instance(NULL)->getWriteref(this) > 1;
}

// FIXME: temporary bug-workaround needed to test if we are exporting
// a VRML97 or an Inventor file. Implementation in SoOutput.cpp.
// pederb, 2003-03-18
extern SbString SoOutput_getHeaderString(const SoOutputP * out);

/*!
  Write out the header of any SoBase derived object. The header consists
  of the \c DEF keyword and the object name (if the object has a name,
  otherwise these will be skipped), the class name and a left bracket.

  Alternatively, the object representation may be made up of just the
  \c USE keyword plus the object name, if this is the second or subsequent
  reference written to the file.

  If the object has been completed just by writing the header (which will
  be the case if we're writing multiple references of an object),
  we return \c TRUE, otherwise \c FALSE.

  If we return \c FALSE (i.e. there's more to write), we will
  increment the indentation level.

  \sa writeFooter(), SoOutput::indent()
 */
SbBool
SoBase::writeHeader(SoOutput * out, SbBool isgroup, SbBool isengine) const
{
  if (!out->isBinary()) {
    out->write(PImpl::END_OF_LINE);
    out->indent();
  }

  SbName name = this->getName();
  int refid = out->findReference(this);
  SbBool firstwrite = (refid == SoWriterefCounter::FIRSTWRITE);
  SbBool multiref = SoWriterefCounter::instance(out)->hasMultipleWriteRefs(this);
  SbName writename = SoWriterefCounter::instance(out)->getWriteName(this);

  // Write the node
  if (!firstwrite) {
    out->write(PImpl::USE_KEYWORD);
    if (!out->isBinary()) out->write(' ');
    out->write(writename.getString());

    if (SoWriterefCounter::debugWriterefs()) {
      SbString tmp;
      tmp.sprintf(" # writeref: %d\n",
                  SoWriterefCounter::instance(out)->getWriteref(this));
      out->write(tmp.getString());
    }
  }
  else {
    if (name != SbName::empty() || multiref) {
      out->write(PImpl::DEF_KEYWORD);
      if (!out->isBinary()) out->write(' ');

      out->write(writename.getString());
      if (!out->isBinary()) out->write(' ');
    }

    if (this->isOfType(SoNode::getClassTypeId()) &&
        ((SoNode*)this)->getNodeType() == SoNode::VRML2) {
      SbString nodename(this->getFileFormatName());
      if (nodename.getLength() > 4) {
        SbString vrml = nodename.getSubString(0, 3);
        const char vrml2headerprefix[] = "#VRML V2.0 utf8";
        const size_t len = sizeof(vrml2headerprefix) - 1;
        const SbString fullheader = SoOutput_getHeaderString(out->pimpl);
        const SbString fileid = ((size_t)fullheader.getLength() < len) ?
          fullheader : fullheader.getSubString(0, len - 1);
        // FIXME: using a temporary workaround to test if we're
        // exporting a VRML97 file. pederb, 2003-03-18
        //
        // UPDATE 20060207 mortene: a better solution would be to
        // carry along the format information in the SoOutput (or an
        // internal private SoOutputP class?) as an enum or something,
        // methinks.
        if ((vrml == "VRML") && (fileid == vrml2headerprefix)) {
          SbString substring = nodename.getSubString(4);
          out->write(substring.getString());
        }
        else {
          out->write(nodename.getString());
        }
      }
      else {
        out->write(nodename.getString());
      }
    }
    else {
      out->write(this->getFileFormatName());
    }
    if (out->isBinary()) {
      unsigned int flags = 0x0;
      if (isgroup) flags |= SoBase::IS_GROUP;
      if (isengine) flags |= SoBase::IS_ENGINE;
      out->write(flags);
    }
    else {
      out->write(" {");
      if (SoWriterefCounter::debugWriterefs()) {
        SbString tmp;
        tmp.sprintf(" # writeref: %d\n",
                    SoWriterefCounter::instance(out)->getWriteref(this));
        out->write(tmp.getString());
      }
      out->write(PImpl::END_OF_LINE);
      out->incrementIndent();
    }
  }

  int writerefcount = SoWriterefCounter::instance(out)->getWriteref(this);

#if COIN_DEBUG
  if (SoWriterefCounter::debugWriterefs()) {
    SoDebugError::postInfo("SoBase::writeHeader",
                           "%p/%s/'%s': %d -> %d",
                           this,
                           this->getTypeId().getName().getString(),
                           this->getName().getString(),
                           writerefcount, writerefcount - 1);
  }
#endif // COIN_DEBUG

  writerefcount--;
  SoWriterefCounter::instance(out)->setWriteref(this, writerefcount);

  // Don't need to write out the rest if we are writing anything but
  // the first instance.
  return (firstwrite == FALSE);
}

/*!
  This method will terminate the block representing an SoBase derived
  object.
 */
void
SoBase::writeFooter(SoOutput * out) const
{
  if (!out->isBinary()) {

    // Keep the old ugly-bugly formatting style around, in case
    // someone, for some obscure reason, needs it.
    static int oldstyle = -1;
    if (oldstyle == -1) {
      oldstyle = coin_getenv("COIN_OLDSTYLE_FORMATTING") ? 1 : 0;
    }

    // FIXME: if we last wrote a field, this EOF is superfluous -- so
    // we are getting a lot of empty lines in the files. Should
    // improve output formatting further. 20031223 mortene.
    if (!oldstyle) { out->write(PImpl::END_OF_LINE); }

    out->decrementIndent();
    out->indent();
    out->write(PImpl::CLOSE_BRACE);
  }
}

/*!
  Returns the class name this object should be written under.  Default
  string returned is the name of the class from the type system.

  User extensions nodes and engines override this method to return the
  name of the extension (instead of "UnknownNode" or "UnknownEngine").
 */
const char *
SoBase::getFileFormatName(void) const
{
  return this->getTypeId().getName().getString();
}

/*!
  \COININTERNAL
*/
uint32_t
SoBase::getCurrentWriteCounter(void)
{
  return SoBase::PImpl::writecounter;
}

/*!
  Connect a route from the node named \a fromnodename's field \a
  fromfieldname to the node named \a tonodename's field \a
  tofieldname. This method will consider the fields types (event in,
  event out, etc) when connecting.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SbBool
SoBase::connectRoute(SoInput * COIN_UNUSED_ARG(in),
                     const SbName & fromnodename, const SbName & fromfieldname,
                     const SbName & tonodename, const SbName & tofieldname)
{
  SoNode * fromnode = SoNode::getByName(fromnodename);
  SoNode * tonode = SoNode::getByName(tonodename);
  if (fromnode && tonode) {
    SoDB::createRoute(fromnode, fromfieldname.getString(),
                      tonode, tofieldname.getString());
    return TRUE;
  }
  return FALSE;
}

/*!
  \COININTERNAL

  Reads a (VRML97) ROUTE. We decided to also add support for routes in
  Coin, as a generic feature, since we think it is nicer than setting
  up field connections inside the nodes.

*/
SbBool
SoBase::readRoute(SoInput * in)
{
  SbString fromstring, tostring;

  SbName fromnodename;
  SbName fromfieldname;
  SbName toname;
  SbName tonodename;
  SbName tofieldname;
  SbBool ok;

  ok =
    in->read(fromstring) &&
    in->read(toname) &&
    in->read(tostring);

  if (ok) ok = (toname == SbName("TO"));

  if (ok) {
    ok = FALSE;

    // parse from-string
    char * str1 = (char*) fromstring.getString();
    char * str2 = str1 ? (char*) strchr(str1, '.') : NULL;
    if (str1 && str2) {
      *str2++ = 0;

      // now parse to-string
      fromnodename = str1;
      fromfieldname = str2;
      str1 = (char*) tostring.getString();
      str2 = str1 ? strchr(str1, '.') : NULL;
      if (str1 && str2) {
        *str2++ = 0;
        tonodename = str1;
        tofieldname = str2;

        ok = TRUE;
      }
    }
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::readRoute",
                         "%s.%s %s %s.%s",
                         fromnodename.getString(),
                         fromfieldname.getString(),
                         toname.getString(),
                         tonodename.getString(),
                         tofieldname.getString());
#endif // debug

  if (!ok) SoReadError::post(in, "Error parsing ROUTE keyword");
  else {
    SoProto * proto = in->getCurrentProto();
    if (proto) {
      proto->addRoute(fromnodename, fromfieldname, tonodename, tofieldname);
    }
    else {
      SoNode * fromnode = SoNode::getByName(fromnodename);
      SoNode * tonode = SoNode::getByName(tonodename);

      if (!fromnode || !tonode) {
        SoReadError::post(in,
                          "Unable to create ROUTE from %s.%s to %s.%s. "
                          "Delaying.",
                          fromnodename.getString(), fromfieldname.getString(),
                          tonodename.getString(), tofieldname.getString());
        in->addRoute(fromnodename, fromfieldname, tonodename, tofieldname);
      }

      (void)SoBase::connectRoute(in, fromnodename, fromfieldname,
                                 tonodename, tofieldname);
    }
  }
  return ok;
}

//
// private method that sends a notify to auditor based on type
//
void
SoBase::doNotify(SoNotList * l, const void * auditor, const SoNotRec::Type type)
{
  l->setLastType(type);
  switch (type) {
  case SoNotRec::CONTAINER:
  case SoNotRec::PARENT:
    {
      SoFieldContainer * obj = (SoFieldContainer *)auditor;
      obj->notify(l);
    }
    break;

  case SoNotRec::SENSOR:
    {
      SoDataSensor * obj = (SoDataSensor *)auditor;
#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoAuditorList::notify",
                             "notify and schedule sensor: %p", obj);
#endif // debug
      // don't schedule the sensor here. The sensor instance will do
      // that in notify() (it might also choose _not_ to schedule),
      obj->notify(l);
    }
    break;

  case SoNotRec::FIELD:
  case SoNotRec::ENGINE:
    {
      // We used to check whether or not the fields was already
      // dirty before we transmitted the notification
      // message. This is _not_ correct (the dirty flag is
      // conceptually only relevant for whether or not to do
      // re-evaluation), so don't try to "optimize" the
      // notification mechanism by re-introducing that "feature".
      // :^/
      ((SoField *)auditor)->notify(l);
    }
    break;

  default:
    assert(0 && "Unknown auditor type");
  }
}

/*!
  Lock access to static data.
  \internal
  \since Coin 2.3
*/
void
SoBase::staticDataLock(void)
{
  CC_MUTEX_LOCK(SoBase::PImpl::global_mutex);
}

/*!
  Unlock access to static data.
  \internal
  \since Coin 2.3
*/
void
SoBase::staticDataUnlock(void)
{
  CC_MUTEX_UNLOCK(SoBase::PImpl::global_mutex);
}

/*!
  Create a record entry for notification lists.
  \internal
*/
SoNotRec
SoBase::createNotRec(void)
{
  return SoNotRec(this);
}

#undef ALIVE_PATTERN

/* *********************************************************************** */

#ifdef COIN_TEST_SUITE

/*This test targets the implementation of SoWriterefCounter::getWriteName
  in the source file SoWriterefCounter.cpp. Test is put here because the
  SoWriterefCounter class is not part of the public API. It is being called
  from here 
  */

#include <Inventor/SoDB.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/actions/SoToVRML2Action.h>
#include <Inventor/VRMLnodes/SoVRMLGroup.h>

 static char * buffer;
  static size_t buffer_size = 0;

//
// If function is copied from SoWriterefCounter.cpp as pricate classes cannot be called from the test suite
//
  static SbBool
dont_mangle_output_names(const SoBase *base)
{
  static int COIN_DONT_MANGLE_OUTPUT_NAMES = -1;

  // Always unmangle node names in VRML1 and VRML2
  if (base->isOfType(SoNode::getClassTypeId()) &&
      (((SoNode *)base)->getNodeType()==SoNode::VRML1 ||
     ((SoNode *)base)->getNodeType()==SoNode::VRML2)) return TRUE;

  if (COIN_DONT_MANGLE_OUTPUT_NAMES < 0) {
    COIN_DONT_MANGLE_OUTPUT_NAMES = 0;
    const char * env = coin_getenv("COIN_DONT_MANGLE_OUTPUT_NAMES");
    if (env) COIN_DONT_MANGLE_OUTPUT_NAMES = atoi(env);
  }
  return COIN_DONT_MANGLE_OUTPUT_NAMES ? TRUE : FALSE;
}

  static void *
  buffer_realloc(void * bufptr, size_t size)
  {
    buffer = (char *)realloc(bufptr, size);
    buffer_size = size;
    return buffer;
  }


BOOST_AUTO_TEST_CASE(checkWriteWithMultiref)
{
	SoDB::init();
	   SoNode* scenegraph;
       SoSeparator *root = new SoSeparator;
       root->ref();
       root->setName("root");
		scenegraph = root;
       SoSeparator *n0 = new SoSeparator;
       SoSeparator *a0 = new SoSeparator;
       SoSeparator *a1 = new SoSeparator;
       SoSeparator *a2 = new SoSeparator;
       SoSeparator *a3 = new SoSeparator;
       SoSeparator *b0 = new SoSeparator;
       SoSeparator *b1 = new SoSeparator;
       SoSeparator *b2 = new SoSeparator;
       SoSeparator *b3 = new SoSeparator;
       SoSeparator *b4 = new SoSeparator;
       SoSeparator *c0 = new SoSeparator;

       a2->setName(SbName("MyName"));
       b0->setName(SbName("MyName"));
       b1->setName(SbName("MyName"));
       b2->setName(SbName("MyName"));
       b4->setName(SbName("MyName"));
       c0->setName(SbName("MyName"));

       root->addChild(n0);
       root->addChild(n0);
       root->addChild(a0);
       a0->addChild(b0);
       a0->addChild(b1);
       root->addChild(b0);
       root->addChild(a1);
       a1->addChild(b2);
       a1->addChild(b1);
       root->addChild(b1);
       root->addChild(a2);
       root->addChild(a2);
       root->addChild(a3);
       a3->addChild(b3);
       b3->addChild(c0);
       b3->addChild(c0);
       a3->addChild(b4);
       a3->addChild(a2);



/*
       Correct output file if dont_mangle_names:

#VRML V1.0 ascii

VRMLGroup {
  children 
    DEF root VRMLGroup {
      children [ 
        DEF _+0 VRMLGroup {
        },
        USE _+0,
        DEF _+1 VRMLGroup {
          children [ 
            DEF MyName VRMLGroup {
            },
            DEF MyName+2 VRMLGroup {
            } ]
        },
        USE MyName,
        DEF _+3 VRMLGroup {
          children [ 
            DEF MyName+4 VRMLGroup {
            },
            USE MyName+2 ]
        },
        USE MyName+2,
        DEF MyName+5 VRMLGroup {
        },
        USE MyName+5,
        DEF _+6 VRMLGroup {
          children [ 
            DEF _+7 VRMLGroup {
              children [ 
                DEF MyName+8 VRMLGroup {
                },
                USE MyName+8 ]
            },
            DEF MyName+9 VRMLGroup {
            },
            USE MyName+5 ]
        } ]
    }
}
    */

	   /* correct outputfile for default OIV behavior:
#VRML V1.0 ascii

DEF root Separator {
  DEF _+0 Separator {
  }
  USE _+0
  Separator {
    DEF MyName Separator {
    }
    DEF MyName+1 Separator {
    }
  }
  USE MyName
  Separator {
    DEF MyName+2 Separator {
    }
    USE MyName+1
  }
  USE MyName+1
  DEF MyName+3 Separator {
  }
  USE MyName+3
  Separator {
    Separator {
      DEF MyName+4 Separator {
      }
      USE MyName+4
    }
    DEF MyName+5 Separator {
    }
    USE MyName+3
  }
}
*/

#ifdef HAVE_VRML97
	    SoVRMLGroup *newroot;
	   for(int j=0;j<2;j++) {
		   if(j==1) {
	SoToVRML2Action tovrml2;
      tovrml2.apply(root);
      newroot = tovrml2.getVRML2SceneGraph();
	  newroot->ref();
	  scenegraph = newroot;
		   }


	   buffer = (char*)malloc(1024);
       SoOutput out;
       // out.openFile("out.wrl"); 
	   out.setBuffer(buffer, 1024,buffer_realloc);
       out.setHeaderString(SbString("#VRML V1.0 ascii"));
       SoWriteAction wra(&out);
       wra.apply(scenegraph);
	   SbString s(buffer);
	   free(buffer);

	   int pos;
	   SbString ss;

	   SbList<SbString> node_names(15);
	   if(j==1)
		BOOST_CHECK_MESSAGE(dont_mangle_output_names(scenegraph)==TRUE,"don't mangle should be TRUE");
	   else
		BOOST_CHECK_MESSAGE(dont_mangle_output_names(scenegraph)==FALSE,"don't mangle should be FALSE");

	   if(dont_mangle_output_names(scenegraph)) {
		   node_names.append("_+0");
		   node_names.append("_+0");
		   node_names.append("_+1");
		   node_names.append("MyName");
		   node_names.append("MyName+2");
		   node_names.append("MyName");
		   node_names.append("_+3");
		   node_names.append("MyName+4");
		   node_names.append("MyName+2");
		   node_names.append("MyName+2");
		   node_names.append("MyName+5");
		   node_names.append("MyName+5");
		   node_names.append("_+6");
		   node_names.append("_+7");
		   node_names.append("MyName+8");
		   node_names.append("MyName+8");
		   node_names.append("MyName+9");
		   node_names.append("MyName+5");
	   }
	   else {
		   node_names.append("_+0");
		   node_names.append("_+0");
		   node_names.append("MyName");
		   node_names.append("MyName+1");
		   node_names.append("MyName");
		   node_names.append("MyName+2");
		   node_names.append("MyName+1");
		   node_names.append("MyName+1");
		   node_names.append("MyName+3");
		   node_names.append("MyName+3");
		   node_names.append("MyName+4");
		   node_names.append("MyName+4");
		   node_names.append("MyName+5");
		   node_names.append("MyName+3");
	   }
   
	   int list_index = 0;
	   SbBool fail = false;
	   ss = s;
	   for(int i=0;i<node_names.getLength();i++) {
		   pos = ss.find(node_names[i]);
		   if(pos==-1) {
			   fail = true;
			   break;
		   }
		   ss = ss.getSubString(pos+node_names[i].getLength(),ss.getLength());
	   }


	   BOOST_CHECK_MESSAGE(!fail,"Check failed, written node names should match test template");
	   
	   }
	   
	
       root->unref();
	   newroot->unref();
#endif
 }

#endif // COIN_TEST_SUITE

/* *********************************************************************** */
