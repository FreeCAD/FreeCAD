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
  \class SoField SoField.h Inventor/fields/SoField.h
  \brief The SoField class is the top-level abstract base class for fields.

  \ingroup coin_fields

  Fields is the mechanism used throughout Coin for encapsulating basic
  data types to detect changes made to them, and to provide
  conversion, import and export facilities.

  Almost all public properties in nodes are stored in fields, and so
  are the inputs and outputs of engines. So fields can be viewed as
  the major mechanism for scene graph nodes and engines to expose their
  public API.

  Forcing data modification to go through a public function interface
  while hiding the data members makes it possible to automatically
  detect and react upon changes in the data structures set up by the
  application programmer.

  E.g. the default behavior when changing the value of a field in a
  scene graph node is that there'll automatically be a chain of
  notifications -- from the field to the owner node, from that node to
  its parent node, etc all the way through to the top-most root node,
  where the need for a rendering update will be signaled to the
  application.

  (This notification mechanism is the underlying feature that makes the
  Coin library classify as a so-called \e data-driven scene graph API.

  The practical consequences of this is that rendering and many other
  processing actions is default scheduled to \e only happen when
  something has changed in the retained data structures, making the
  Coin library under normal circumstances \e much less CPU intensive
  than so-called "application-driven" scene graph API, like for
  instance SGI IRIS Performer, which are continuously re-rendering
  even when nothing has changed in the data structures or with the
  camera viewport.)

  Storing data members as fields also provides other conveniences for
  the application programmer:

  \li Fields can be connected to other fields. This makes it for
      instance possible to have "self-updating" scenes, i.e. you can set
      up scenes where entities \e automatically react to changes in
      other entities. This also provides a necessary mechanism for
      having "auto-animating" scenes, as it is possible to connect any
      field to the global field named \c realTime, providing a
      wall-clock timer.

  \li When connecting fields to each other, Coin has built-in
      mechanisms for automatically converting between different field
      types.

  \li Fields provide persistence for scene graph import (and export)
      operations. This includes animating entities, so animations can
      be stored within ordinary Inventor format files.

  \li Fields provides features for introspection: they have a
      type-system, just like for nodes and actions, they are named,
      and it is also possible to find out which node, engine or other
      entity owns a field.

  \li Fields can hold multiple values. Multi-value fields comes with a
      much higher level interface abstraction than standard C/C++
      arrays.

  Note: there are some field classes which have been obsoleted from the
  Open Inventor API. They are: SoSFLong, SoSFULong, SoMFLong and
  SoMFULong. You should use these classes instead (respectively):
  SoSFInt32, SoSFUInt32, SoMFInt32 and SoMFUInt32.

  \TOOLMAKER_REF

  \sa SoFieldContainer, SoFieldData
*/

#include <Inventor/fields/SoField.h>

#include <cassert>
#include <cstring>

#include <Inventor/fields/SoFields.h>

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SoEngineList.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/sensors/SoDataSensor.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H
#include "SbBasicP.h"
#include "engines/SoConvertAll.h"
#include "fields/SoGlobalField.h"
#include "io/SoWriterefCounter.h"
#include "misc/SoConfigSettings.h"
#include "threads/threadsutilp.h"
#include "tidbitsp.h"
inline unsigned int SbHashFunc(const void * key);
#include "misc/SbHash.h"
inline unsigned int SbHashFunc(const void * key)
{
  return SbHashFunc(reinterpret_cast<size_t>(key));
}
#include "coindefs.h" // COIN_STUB(), COIN_CHECK_THREAD()

#ifdef COIN_THREADSAFE
#include "threads/recmutexp.h"
#define SOFIELD_RECLOCK (void) cc_recmutex_internal_field_lock()
#define SOFIELD_RECUNLOCK (void) cc_recmutex_internal_field_unlock()

#else // COIN_THREADSAFE

#define SOFIELD_RECLOCK
#define SOFIELD_RECUNLOCK

#endif // !COIN_THREADSAFE

static const int SOFIELD_GET_STACKBUFFER_SIZE = 1024;
// need one static mutex for field_buffer in SoField::get(SbString &)
static void * sofield_mutex = NULL;

// flags for this->statusbits

static const char IGNOREDCHAR = '~';
static const char CONNECTIONCHAR = '=';
/*
  This class is used to aid in "multiplexing" the pointer member of
  SoField. This is a way to achieve the goal of using minimum storage
  space for SoField classes in the default case (which is important,
  as fields are ubiquitous in Coin). The default case means no
  connections and only a field container given. If any connections are
  made (either "to" or "from"), we allocate an SoConnectStorage and
  move the field container pointer into it, while swapping in the
  SoConnectStorage pointer where the field container pointer used to
  be.
*/
class SoConnectStorage {
public:
  SoConnectStorage(SoFieldContainer * c, SoType t)
    : container(c),
    lastnotify(NULL),
    fieldtype(t),
    maptoconverter(13) // save about ~1kB vs default nr of buckets
    {
    }

#if COIN_DEBUG
  // Check that everything has been emptied.
  ~SoConnectStorage()
  {
    assert(this->maptoconverter.getNumElements() == 0);

    assert(masterfields.getLength() == 0);
    assert(masterengineouts.getLength() == 0);

    assert(slaves.getLength() == 0);
    assert(auditors.getLength() == 0);
  }
#endif // COIN_DEBUG

  // The container this field is part of.
  SoFieldContainer * container;

  // List of masters we're connected to as a slave. Use maptoconverter
  // dict to find SoFieldConverter engine in the connection (if any).
  SoFieldList masterfields;
  SoEngineOutputList masterengineouts;
  // Fields which are slaves to us. Use maptoconverter dict to find
  // SoFieldConverter engine in the connection (if any).
  SoFieldList slaves;
  // Direct auditors of us.
  SoAuditorList auditors;

  // used to track the last notification (for fanIn handling)
  void * lastnotify;

  // Convenience functions for adding, removing and finding mappings.

  void addConverter(const void * item, SoFieldConverter * converter)
  {
    // "item" can be SoField* or SoEngineOutput*.

    // FIXME: this probably hashes horribly bad, as the item value is
    // a pointer and is therefore address-aligned (lower 32 (?) bits
    // are all 0).  20010911 mortene.
    this->maptoconverter.put(item, converter);
  }

  void removeConverter(const void * item)
  {
    size_t ok = this->maptoconverter.erase(item);
    assert(ok);
  }

  SoFieldConverter * findConverter(const void * item)
  {
    SoFieldConverter * val;
    if (!this->maptoconverter.get(item, val)) { return NULL; }
    return val;
  }

  SbBool hasFanIn(void) {
    return (this->masterfields.getLength() + this->masterengineouts.getLength()) > 1;
  }
  int findFanInEngine(void) const {
    for (int i = 0; i < this->masterengineouts.getLength(); i++) {
      SoEngineOutput * o = this->masterengineouts[i];
      if (o->isNodeEngineOutput()) {
        if (static_cast<void *>(o->getNodeContainer()) == this->lastnotify) return i;
      }
      else {
        if (static_cast<void *>(o->getContainer()) == this->lastnotify) return i;
      }
    }
    return -1;
  }
  int findFanInField(void) const {
    for (int i = 0; i < this->masterfields.getLength(); i++) {
      if (static_cast<void *>(this->masterfields[i]->getContainer()) == this->lastnotify) return i;
    }
    return -1;
  }


  // Provides us with a hack to get at a master field's type in code
  // called from its constructor (SoField::getTypeId() is virtual and
  // can't be used).
  //
  // (Used in the master::~SoField() -> slave::disconnect(master)
  // chain.)
  SoType fieldtype;
  void add_vrml2_routes(SoOutput * out, const SoField * f);

private:
  // Dictionary of void* -> SoFieldConverter* mappings.
  SbHash<const void *, SoFieldConverter *> maptoconverter;

};

// helper function. Used to check if field is in a vrml2 node
static SbBool
is_vrml2_field(const SoField * f)
{
  assert(f);
  SoFieldContainer * fc = f->getContainer();
  // test fc to support fields with no container
  if (fc && fc->isOfType(SoNode::getClassTypeId())) {
    if (fc->isOfType(SoProtoInstance::getClassTypeId())) return TRUE;
    if (coin_assert_cast<SoNode *>(fc)->getNodeType() & SoNode::VRML2) return TRUE;
  }

  return FALSE;
}

//
// add all connections to this field as routes in SoOutput.  SoOutput
// will decide when to write the ROUTEs.
//
void
SoConnectStorage::add_vrml2_routes(SoOutput * out, const SoField * f)
{
  SoFieldContainer * tofc = f->getContainer();
  assert(tofc);
  SbName toname, fromname;
  (void) tofc->getFieldName(f, toname);

  int i;
  for (i = 0; i < this->masterfields.getLength(); i++) {
    SoField * master = this->masterfields[i];
    SoFieldContainer * fc = master->getContainer();
    assert(fc);
    (void) fc->getFieldName(master, fromname);

    if (out->getStage() == SoOutput::COUNT_REFS) {
      fc->addWriteReference(out, TRUE);
      tofc->addWriteReference(out, TRUE);
    }
    else {
      out->addRoute(fc, fromname, tofc, toname);
    }
  }
  for (i = 0; i < this->masterengineouts.getLength(); i++) {
    SoEngineOutput * engineout = this->masterengineouts[i];
    SoFieldContainer * fc = engineout->getFieldContainer();
    if (engineout->isNodeEngineOutput()) {
      SoNodeEngine * engine = engineout->getNodeContainer();
      assert(engine);
      (void) engine->getOutputName(engineout, fromname);
    }
    else {
      SoEngine * engine = engineout->getContainer();
      assert(engine);
      (void) engine->getOutputName(engineout, fromname);
    }
    if (out->getStage() == SoOutput::COUNT_REFS) {
      fc->addWriteReference(out, TRUE);
      tofc->addWriteReference(out, TRUE);
    }
    else {
      out->addRoute(fc, fromname, tofc, toname);
    }
  }
}

// Collects some private code for SoField.
//
// Note that there is no private implementation data pointer (aka
// "pimpl" or Cheshire Cat) for the SoField instances, as they should
// be as slim as possible. Therefore, this class only contains static
// functions.
class SoFieldP {
public:
  // Convenience method to extract a string that identifies the field
  // with as much relevant info as possible. Used from other debug
  // output code.
  static SbString getDebugIdString(const SoField * f)
  {
    SoFieldContainer * fcontainer = f->getContainer();
    SbName fname("<no-container>");
    if (fcontainer) {
      SbBool ok = fcontainer->getFieldName(f, fname);
      if (!ok) { fname = "<not-yet-added>"; }
    }
    SbString s;
    s.sprintf("field==%p/%s/'%s'",
              f,
              f->getTypeId().getName().getString(),
              fname.getString());
    return s;
  }

  static SbHash<char *, char **> * getReallocHash(void);
  static void * hashRealloc(void * bufptr, size_t size);

  static SbHash<char *, char **> * ptrhash;
};

SbHash<char *, char **> * SoFieldP::ptrhash = NULL;

extern "C" {
// atexit callbacks
static void SoField_cleanupClass(void);
static void hashExitCleanup(void);
static void field_mutex_cleanup(void);
}

SbHash<char *, char **> *
SoFieldP::getReallocHash(void)
{
  // FIXME: protect with mutex?
  if (SoFieldP::ptrhash == NULL) {
    SoFieldP::ptrhash = new SbHash<char *, char **>;
    coin_atexit(hashExitCleanup, CC_ATEXIT_NORMAL);
  }
  return SoFieldP::ptrhash;
}

void
hashExitCleanup(void)
{
  assert(SoFieldP::ptrhash->getNumElements() == 0);
  delete SoFieldP::ptrhash;
  SoFieldP::ptrhash = NULL;
}

void *
SoFieldP::hashRealloc(void * bufptr, size_t size)
{
  CC_MUTEX_LOCK(sofield_mutex);

  char ** bufptrptr = NULL;
  SbBool ok = SoFieldP::ptrhash->get(static_cast<char *>(bufptr), bufptrptr);
  assert(ok);

  // If *bufptrptr contains a NULL pointer, this is the first
  // invocation and the initial memory buffer was on the stack.
  char * newbuf;
  if (*bufptrptr == NULL) {
    // if initial buffer was on the stack, we need to manually copy
    // the data into the new buffer.
    newbuf = static_cast<char *>(malloc(size));
    memcpy(newbuf, bufptr, SOFIELD_GET_STACKBUFFER_SIZE);
  }
  else {
    newbuf = static_cast<char *>(realloc(bufptr, size));
  }
  if (newbuf != bufptr) {
    size_t isok = SoFieldP::ptrhash->erase(static_cast<char *>(bufptr));
    assert(isok);
    *bufptrptr = newbuf;
    SoFieldP::ptrhash->put(newbuf, bufptrptr);
  }

  CC_MUTEX_UNLOCK(sofield_mutex);

  return newbuf;
}

// *************************************************************************

// Documentation for abstract methods.

// FIXME: grab better version of getTypeId() doc from SoBase, SoAction
// and / or SoDetail. 20010913 mortene.
/*!
  \fn SoType SoField::getTypeId(void) const

  Returns the type identification instance which uniquely identifies
  the Coin field class the object belongs to.

  \sa getClassTypeId(), SoType
*/

/*!
  \fn SbBool SoField::isSame(const SoField & f) const
  Check for equal type and value(s).
*/

/*!
  \fn void SoField::copyFrom(const SoField & f)

  Copy value(s) from \a f into this field. \a f must be of the same
  type as this field.
*/

/*!
  \fn SbBool SoField::readValue(SoInput * in)
  Read field value(s).
*/

/*!
  \fn void SoField::writeValue(SoOutput * out) const
  Write field value(s).
*/


// *************************************************************************

SoType SoField::classTypeId STATIC_SOTYPE_INIT;

// *************************************************************************

// used to detect when a field that is already destructed is used
#define FLAG_ALIVE_PATTERN 0xbeef0000

// private methods. Inlined inside this file only.

// clear bits in statusbits
inline void
SoField::clearStatusBits(const unsigned int bits)
{
  this->statusbits &= ~bits;
}

// sets bits in statusbits
inline void
SoField::setStatusBits(const unsigned int bits)
{
  this->statusbits |= bits;
}

// return TRUE if any of bits is set
inline SbBool
SoField::getStatus(const unsigned int bits) const
{
  return (this->statusbits & bits) != 0;
}

// convenience method for clearing or setting based on boolean value
// returns TRUE if any bitflag changed value
inline SbBool
SoField::changeStatusBits(const unsigned int bits, const SbBool onoff)
{
  unsigned int oldval = this->statusbits;
  unsigned int newval = oldval;
  if (onoff) newval |= bits;
  else newval &= ~bits;
  if (oldval != newval) {
    this->statusbits = newval;
    return TRUE;
  }
  return FALSE;
}

// returns TRUE if this field has ext storage
inline SbBool
SoField::hasExtendedStorage(void) const
{
  return this->getStatus(FLAG_EXTSTORAGE);
}


/*!
  This is the base constructor for field classes. It takes care of
  doing the common parts of data initialization in fields.
*/
SoField::SoField(void)
  : container(NULL)
{
  this->statusbits = 0;
  this->setStatusBits(FLAG_DONOTIFY |
                      FLAG_ISDEFAULT |
                      FLAG_ENABLECONNECTS|
                      FLAG_ALIVE_PATTERN);
}

/*!
  Destructor. Disconnects ourself from any connected field or engine
  before we disconnect all auditors on the field.
*/
SoField::~SoField()
{
  // set status bit to avoid evaluating this field while
  // disconnecting connections.
  this->setStatusBits(FLAG_ISDESTRUCTING);

#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::~SoField", "destructing %p", this);
#endif //COIN_DEBUG_EXTRA

  // Disconnect ourself from all connections where this field is the
  // slave.
  this->disconnect();

  if (this->hasExtendedStorage()) {

    // Disconnect slave fields using us as a master.
    while (this->storage->slaves.getLength()) {
      this->storage->slaves[0]->disconnect(this);
    }

    // Disconnect other auditors.
    while (this->storage->auditors.getLength()) {
      SoNotRec::Type type = this->storage->auditors.getType(0);
      void * obj = this->storage->auditors.getObject(0);

      switch (type) {
      case SoNotRec::ENGINE:
        static_cast<SoEngineOutput *>(obj)->removeConnection(this);
        break;

      case SoNotRec::CONTAINER:
        assert(FALSE && "Container should not be in auditorlist");
        break;

      case SoNotRec::SENSOR:
        static_cast<SoDataSensor *>(obj)->dyingReference();
        break;

      case SoNotRec::FIELD:
        assert(FALSE); // should not happen, as slave fields are removed first.
        break;

      default:
        assert(FALSE); // no other allowed types.
        break;
      }
    }

    delete this->storage;
  }
#if COIN_DEBUG_EXTRA
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::~SoField", "%p done", this);
#endif //COIN_DEBUG_EXTRA

  this->clearStatusBits(FLAG_ALIVE_PATTERN);
}

// atexit
void
field_mutex_cleanup(void)
{
  CC_MUTEX_DESTRUCT(sofield_mutex);
}

/*!
  Internal method called upon initialization of the library (from
  SoDB::init()) to set up the type system.
*/
void
SoField::initClass(void)
{
  // Make sure we only initialize once.
  assert(SoField::classTypeId == SoType::badType());

  CC_MUTEX_CONSTRUCT(sofield_mutex);
  coin_atexit(field_mutex_cleanup, CC_ATEXIT_NORMAL);

  SoField::classTypeId = SoType::createType(SoType::badType(), "Field");
  SoField::initClasses();
  coin_atexit(SoField_cleanupClass, CC_ATEXIT_NORMAL);
}

void
SoField_cleanupClass(void)
{
  SoField::cleanupClass();
}

/*!
  This static method cleans up static data of the SoField class.
*/
void
SoField::cleanupClass(void)
{
  SoField::classTypeId STATIC_SOTYPE_INIT;
}

/*!
  Sets the flag which indicates whether or not the field should be
  ignored during certain operations.

  The effect of this flag depends on what type of field it is used on,
  and the type of the node which includes the field.

  This flag is represented in Inventor files by a ~ behind the field
  name.  The flag is in other words persistent.

  \sa isIgnored()
*/
void
SoField::setIgnored(SbBool ignore)
{
  if (this->changeStatusBits(FLAG_IGNORE, ignore)) {
    this->valueChanged(FALSE);
  }
}

/*!
  Returns the ignore flag.

  \sa setIgnored()
*/
SbBool
SoField::isIgnored(void) const
{
  return this->getStatus(FLAG_IGNORE);
}

/*!
  Set whether or not this field should be marked as containing a
  default value.

  \sa isDefault()
*/
void
SoField::setDefault(SbBool def)
{
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3) {
    SbString finfo = SoFieldP::getDebugIdString(this);
    SoDebugError::postInfo("SoField::setDefault", "%s, setDefault(%s)",
                           finfo.getString(), def ? "TRUE" : "FALSE");
  }
#endif //COIN_DEBUG_EXTRA

  (void) this->changeStatusBits(FLAG_ISDEFAULT, def);
}

/*!
  Check if the field contains its default value. Fields which have
  their default value intact will normally not be included in the
  output when writing scene graphs out to a file, for instance.

  \sa setDefault()
*/
SbBool
SoField::isDefault(void) const
{
  return this->getStatus(FLAG_ISDEFAULT);
}

/*!
  Returns a unique type identifier for this field class.

  \sa getTypeId(), SoType
*/
SoType
SoField::getClassTypeId(void)
{
  return SoField::classTypeId;
}

/*!
  Check if this instance is of a derived type or is the same type as
  the one given with the \a type parameter.
*/
SbBool
SoField::isOfType(const SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

/*!
  This sets a \a flag value which indicates whether or not the set up
  connection should be considered active. For as long as the "enable
  connection" flag is \c FALSE, no value propagation will be done from
  any connected source field, engine or interpolator into this field.

  If the connection is first disabled and then enabled again, the
  field will automatically be synchronized with any master field,
  engine or interpolator.

  \sa isConnectionEnabled()
*/
void
SoField::enableConnection(SbBool flag)
{
  SbBool oldval = this->getStatus(FLAG_ENABLECONNECTS);
  (void) this->changeStatusBits(FLAG_ENABLECONNECTS, flag);
  if (!oldval && flag) this->setDirty(TRUE);
}

/*!
  Return the current status of the connection enabled flag.

  \sa enableConnection()
*/
SbBool
SoField::isConnectionEnabled(void) const
{
  return this->getStatus(FLAG_ENABLECONNECTS);
}

/*!
  Connects this field as a slave to \a master. This means that the
  value of this field will be automatically updated when \a master is
  changed (as long as the connection also is enabled).

  If this field had any connections to master fields beforehand, these
  are all broken up if \a append is \c FALSE.

  Call with \a notnotify if you want to avoid the initial notification
  of connected auditors (a.k.a. \e slaves).

  Function will return \c TRUE unless:

  \li If the field connected \e from has a different type from the
      field connected \e to, a field converter is inserted. For some
      combinations of fields no such conversion is possible, and we'll
      return \c FALSE.

  \li If this field is already connected to the \a master, we will
      return \c FALSE.

  \sa enableConnection(), isConnectionEnabled(), isConnectedFromField()
  \sa getConnectedField(), appendConnection(SoField *)
*/
SbBool
SoField::connectFrom(SoField * master, SbBool notnotify, SbBool append)
{
  // detect and ref() global fields. This is done to automatically
  // detect when the last reference to a global field is deleted
  if (master->getContainer() && master->getContainer()->isOfType(SoGlobalField::getClassTypeId())) {
    master->getContainer()->ref();
  }
  // Initialize.  /////////////////////////////////////////////////

  this->extendStorageIfNecessary();
  master->extendStorageIfNecessary();

  SoType mastertype = master->getTypeId();
  SoType thistype = this->getTypeId();
  SbBool containerisconverter = this->getContainer() &&
    this->getContainer()->getTypeId().isDerivedFrom(SoFieldConverter::getClassTypeId());


  // Set up all links.  ///////////////////////////////////////////

  if (mastertype == thistype) { // Can do direct field-to-field link.
    if (!append) this->disconnect();
    else if (this->storage->masterfields.find(master) >= 0) {
      // detect and avoid multiple connections between the same fields
      // (a common bug in VRML files created by 3ds max).
#if COIN_DEBUG
      SoFieldContainer * fc = master->getContainer();
      SbName fcname = fc ? fc->getName() : SbName::empty();
      if (fcname != SbName::empty()) {
        SbName fieldname;
        (void) fc->getFieldName(master, fieldname);
        SoDebugError::postWarning("SoField::connectFrom",
                                  "connection from %p (%s.%s) already made",
                                  master,
                                  fcname.getString(),
                                  fieldname.getString());

      }
      else {
        SoDebugError::postWarning("SoField::connectFrom",
                                  "connection from %p already made", master);
      }
#endif // COIN_DEBUG
      return FALSE;
    }
    // Set up the auditor link from the master to the slave field.
    // (Note that the ``this'' slave field can also be an input field
    // of an SoFieldConverter instance.)
    master->addAuditor(this, SoNotRec::FIELD);
  }
  else { // Needs an SoFieldConverter between the fields.
    SoFieldConverter * conv = this->createConverter(mastertype);
    if (!conv) {
      // Just return FALSE and don't bother to warn, as that is done
      // by the createConverter() method.
      return FALSE;
    }

    if (!append) this->disconnect();

    SoField * converterinput = conv->getInput(mastertype);
    SoEngineOutput * converteroutput = conv->getOutput(thistype);

#if COIN_DEBUG
    if (converterinput == NULL) {
      SoDebugError::post("SoField::connectFrom",
                         "input field returned from field converter is NULL");
      return FALSE;
    } else if (converteroutput == NULL) {
      SoDebugError::post("SoField::connectFrom",
                         "output returned from field converter is NULL");
      return FALSE;
    }
#endif // COIN_DEBUG

    // Link up the input SoField of the SoFieldConverter to the master
    // field by recursively calling connectFrom().
    // the converter engine should always be notified upon connection
    // as it will never have a default value read in from a file.
    converterinput->connectFrom(master, FALSE);

    // Connect from the SoFieldConverter output to the slave field.
    converteroutput->addConnection(this);

    // Remember the connection from the slave field to the
    // SoFieldConverter by setting up a dict entry.
    this->storage->addConverter(master, conv);
  }

  // Common bookkeeping.
  this->storage->masterfields.append(master); // slave -> master link
  if (!containerisconverter)
    master->storage->slaves.append(this); // master -> slave link


  // Notification.  ///////////////////////////////////////////////

  if ((notnotify == FALSE) && this->isConnectionEnabled()) {
    this->setDirty(TRUE);
    this->setDefault(FALSE);
    this->startNotify();
  }

  return TRUE;
}

/*!
  Connects this field as a slave to \a master. This means that the value
  of this field will be automatically updated when \a master is changed (as
  long as the connection also is enabled).

  If this field had any master-relationships beforehand, these are all
  broken up if \a append is \c FALSE.

  Call with \a notnotify if you want to avoid the initial notification
  of connected auditors (a.k.a. \e slaves).

  Function will return \c TRUE unless:

  \li If the field output connected \e from is of a different type
      from the engine output field-type connected \e to, a field
      converter is inserted. For some combinations of fields no such
      conversion is possible, and we'll return \c FALSE.

  \li If this field is already connected to the \a master, we will
      return \c FALSE.

  \sa enableConnection(), isConnectionEnabled(), isConnectedFromField()
  \sa getConnectedField(), appendConnection(SoEngineOutput *)
*/
SbBool
SoField::connectFrom(SoEngineOutput * master, SbBool notnotify, SbBool append)
{
  // Initialize.  /////////////////////////////////////////////////

  this->extendStorageIfNecessary();

  SoType mastertype = master->getConnectionType();
  SoType thistype = this->getTypeId();

  // If we connectFrom() on the same engine as the field is already
  // connected to, we want to avoid the master container engine being
  // unref()'ed down to ref-count 0 upon the disconnect().
  SoFieldContainer * masterengine = master->getFieldContainer();

  if (masterengine) masterengine->ref();


  // Set up all links.  ///////////////////////////////////////////

  if (mastertype == thistype) { // Can do direct field-to-engineout link.
    if (!append) this->disconnect();
    else {
      // check if we're already connected
      if (this->storage->masterengineouts.find(master) >= 0) {
        // detect and avoid multiple connections between the same
        // field and engine output (a common bug in VRML files
        // created by 3ds max).
#if COIN_DEBUG
        SoDebugError::postWarning("SoField::connectFrom",
                                  "connection from %p already made", master);
#endif // COIN_DEBUG
        // Match the ref() invocation.
        if (masterengine) masterengine->unref();
        return FALSE;
      }
    }
    // Set up the auditor link from the master engineout to the slave
    // field.  (Note that the ``this'' slave field can also be an
    // input field of an SoFieldConverter instance.)

    // This is enough, the container SoEngine will automatically pick
    // up on it.
    master->addConnection(this);
  }
  else { // Needs an SoFieldConverter between this field and the SoEngineOutput
    SoFieldConverter * conv = this->createConverter(mastertype);
    if (!conv) { // Handle this exception.
      // Clean up the ref().
      if (masterengine) masterengine->unref();
      // Sorry, can't connect. Don't bother to spit out a warning, as
      // that is done in createConverter().
      return FALSE;
    }

    if (!append) this->disconnect();

    SoField * converterinput = conv->getInput(mastertype);
    SoEngineOutput * converteroutput = conv->getOutput(thistype);

#if COIN_DEBUG
    if (converterinput == NULL) {
      SoDebugError::post("SoField::connectFrom",
                         "input field returned from field converter is NULL");
      return FALSE;
    } else if (converteroutput == NULL) {
      SoDebugError::post("SoField::connectFrom",
                         "output returned from field converter is NULL");
      return FALSE;
    }
#endif // COIN_DEBUG

    // Link up the input SoField of the SoFieldConverter to the master
    // SoEngineOutput by recursively calling connectFrom().
    // the converter engine should always be notified upon connection
    // as it will never have a default value read in from a file
    converterinput->connectFrom(master, FALSE);

    // Connect from the SoFieldConverter output to the slave field.
    converteroutput->addConnection(this);

    // Remember the connection from the slave field to the
    // SoFieldConverter by setting up a dict entry.
    this->storage->addConverter(master, conv);
  }

  // Match the ref() invocation.
  if (masterengine) masterengine->unref();

  // Common bookkeeping.
  this->storage->masterengineouts.append(master); // slave -> master link

  // Notification.  ///////////////////////////////////////////////

  if ((notnotify == FALSE) && this->isConnectionEnabled()) {
    this->setDirty(TRUE);
    this->setDefault(FALSE);
    this->startNotify();
  }

  return TRUE;
}


/*!
  Disconnect this field as a slave from \a master.
*/
void
SoField::disconnect(SoField * master)
{
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::disconnect",
                         "removing slave field %p from master field %p",
                         this, master);
#endif //COIN_DEBUG_EXTRA

  const int idx = this->storage->masterfields.find(master);
  if (idx == -1) {
    SoDebugError::post("SoField::disconnect",
                       "can't disconnect from a field which we're not connected to!");
    return;
  }

  this->evaluate();

  SbBool containerisconverter = this->getContainer() &&
    this->getContainer()->getTypeId().isDerivedFrom(SoFieldConverter::getClassTypeId());


  // Decouple links. ///////////////////////////////////////////////////

  // Remove bookkeeping material.
  if (!containerisconverter) master->storage->slaves.removeItem(this);

  this->storage->masterfields.remove(idx);

  SoFieldConverter * converter = this->storage->findConverter(master);
  if (converter) { // There's a converter engine between the fields.

    SoField * converterinput =
      converter->getInput(SoType::badType()); // dummy type
    converterinput->disconnect(master);

    SoEngineOutput * converteroutput =
      converter->getOutput(SoType::badType()); // dummy type
    converteroutput->removeConnection(this);

    this->storage->removeConverter(master);
    converter->unref();
  }
  else { // No converter, just a direct link.
    master->removeAuditor(this, SoNotRec::FIELD);
  }

  // detect and unref() global fields. This is done to detect when the
  // last reference to a global fields is deleted.
  if (master->getContainer() && master->getContainer()->isOfType(SoGlobalField::getClassTypeId())) {
    master->getContainer()->unref();
  }
}

/*!
  Disconnect this field as a slave from \a master.
*/
void
SoField::disconnect(SoEngineOutput * master)
{
  // First check to see we're the input field of an
  // SoFieldConverter. If so, recursively call disconnect() with the
  // field on "the other side" of the converter.

  SoType fieldconvtype = SoFieldConverter::getClassTypeId();
  SbBool containerisconverter =
    this->getContainer() &&
    this->getContainer()->getTypeId().isDerivedFrom(fieldconvtype);
  if (containerisconverter) {
    SoFieldConverter * converter =
      coin_assert_cast<SoFieldConverter *>(this->getContainer());
    SoEngineOutput * converterout =
      converter->getOutput(SoType::badType()); // dummy type
    SoFieldList fl;
    converterout->getForwardConnections(fl);
    fl[0]->disconnect(master);
    return;
  }


#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::disconnect",
                           "removing slave field %p (%s.%s) from master "
                           "engineout %p",
                           this,
                           this->storage->container->getTypeId().getName().getString(),
                           this->storage->fieldtype.getName().getString(),
                           master);
#endif //COIN_DEBUG_EXTRA


  // Check the enabled flag to avoid evaluating from engines which are
  // being destructed. This is a bit of a hack, but I don't think it
  // matters.   -- mortene.
  if (master->isEnabled()) this->evaluate();

  // Decouple links. ///////////////////////////////////////////////////

  // Remove bookkeeping material.
  this->storage->masterengineouts.removeItem(master);

  SoFieldConverter * converter = this->storage->findConverter(master);
  if (converter) { // There's a converter engine between the fields.
    SoField * converterinput =
      converter->getInput(SoType::badType()); // dummy type
    converterinput->storage->masterengineouts.removeItem(master);
    master->removeConnection(converterinput);

    SoEngineOutput * converteroutput =
      converter->getOutput(SoType::badType()); // dummy type
    converteroutput->removeConnection(this);

    this->storage->removeConverter(master);
    converter->unref();
  }
  else { // No converter, just a direct link.
    master->removeConnection(this);
  }
}

/*!
  Returns number of fields this field is a slave of.

  \sa getConnections()
*/
int
SoField::getNumConnections(void) const
{
  return this->hasExtendedStorage() ?
    this->storage->masterfields.getLength() : 0;
}

/*!
  Returns number of masters this field is connected to, and places
  pointers to all of them into \a masterlist.

  Note that we replace the contents of \a masterlist, i.e. we're \e
  not appending new data.

  \sa getNumConnections()
*/
int
SoField::getConnections(SoFieldList & masterlist) const
{
  if (!this->hasExtendedStorage()) return 0;

  masterlist = this->storage->masterfields;
  return masterlist.getLength();
}

/*!
  Disconnect all connections from this field as a slave to master
  fields or engine outputs.
*/
void
SoField::disconnect(void)
{
  // Disconnect us from all master fields.
  while (this->isConnectedFromField())
    this->disconnect(this->storage->masterfields[0]);

  // Disconnect us from all master engine outputs.
  while (this->isConnectedFromEngine())
    this->disconnect(this->storage->masterengineouts[0]);

  assert(this->isConnected() == FALSE);
}

/*!
  Returns \c TRUE if we're connected from another field, engine or
  interpolator.

  \sa isConnectedFromField(), isConnectedFromEngine()
  \sa connectFrom()
*/
SbBool
SoField::isConnected(void) const
{
  return (this->isConnectedFromField() ||
          this->isConnectedFromEngine());
}

/*!
  Returns \c TRUE if we're a slave of at least one field.

  \sa isConnected(), isConnectedFromEngine()
  \sa connectFrom(SoField *)
*/
SbBool
SoField::isConnectedFromField(void) const
{
  return (this->hasExtendedStorage() &&
          this->storage->masterfields.getLength() > 0);
}

/*!
  Returns \c TRUE if we're connected from an engine.

  \sa isConnected(), isConnectedFromField()
  \sa connectFrom(SoEngineOutput *)
*/
SbBool
SoField::isConnectedFromEngine(void) const
{
  return (this->hasExtendedStorage() &&
          this->storage->masterengineouts.getLength() > 0);
}

// Simplify by collecting common code for SoField::getConnected*() methods.
#define SOFIELD_GETCONNECTED(_masterlist_) \
  if (!this->hasExtendedStorage()) return FALSE; \
  int nrmasters = this->storage->_masterlist_.getLength(); \
  if (nrmasters < 1) return FALSE; \
  master = this->storage->_masterlist_[nrmasters - 1]; \
  return TRUE

/*!
  Returns \c TRUE if we are connected as a slave to at least one other
  field.  \a master will be set to the source field in the last field
  connection made.

  \sa isConnectedFromField(), connectFrom(SoField *),
  \sa appendConnection(SoField *)
*/
SbBool
SoField::getConnectedField(SoField *& master) const
{
  SOFIELD_GETCONNECTED(masterfields);
}

/*!
  Returns \c TRUE if we are connected as a slave to at least one
  engine. \a master will be set to the source of the last engine
  connection made.

  \sa isConnectedFromEngine(), connectFrom(SoEngineOutput *)
  \sa appendConnection(SoEngineOutput *)
*/
SbBool
SoField::getConnectedEngine(SoEngineOutput *& master) const
{
  SOFIELD_GETCONNECTED(masterengineouts);
}

#undef SOFIELD_GETCONNECTED

/*!
  Appends all the fields which are auditing this field in \a
  slavelist, and returns the number of fields which are our slaves.
*/
int
SoField::getForwardConnections(SoFieldList & slavelist) const
{
  if (!this->hasExtendedStorage()) return 0;

  int nr = 0;

  for (int i=0; i < this->storage->slaves.getLength(); i++) {
    slavelist.append(this->storage->slaves[i]);
    nr++;
  }

  return nr;
}

/*!
  Let the field know to which container it belongs.

  \sa getContainer(), SoFieldContainer
*/
void
SoField::setContainer(SoFieldContainer * cont)
{
  if (!this->hasExtendedStorage()) this->container = cont;
  else this->storage->container = cont;

  // The field should have been set to its default value before it is
  // added to the container.
  //
  // This might seem strange, but it looks like it is necessary to do
  // it this way to be compatible with Open Inventor.
  this->setDefault(TRUE);
}

/*!
  Returns the SoFieldContainer object "owning" this field.

  \sa SoFieldContainer, setContainer()
*/
SoFieldContainer *
SoField::getContainer(void) const
{
  if (!this->hasExtendedStorage()) return this->container;
  else return this->storage->container;
}

/*!
  Set the field's value through the given \a valuestring. The format
  of the string must adhere to the ASCII format used in Coin data
  format files.

  Only the value should be specified - \e not the name of the field.

  \c FALSE is returned if the field value is invalid for the field
  type and can't be parsed in any sensible way.

  \sa get()
*/
SbBool
SoField::set(const char * valuestring)
{
  // Note that it is not necessary to set a header identification line
  // for this to work.
  SoInput in;
  in.setBuffer(const_cast<char *>(valuestring), strlen(valuestring));
  if (!this->readValue(&in)) return FALSE;

  this->valueChanged();
  return TRUE;
}

/*!
  Returns the field's value as an ASCII string in the export data
  format for Inventor files.

  \sa set()
*/
void
SoField::get(SbString & valuestring)
{
  // FIXME: this function should be const! 20050607 mortene.

  // NOTE: this code has an almost verbatim copy in SoMField::get1(),
  // so remember to update both places if any fixes are done.

  // Initial buffer setup.
  SoOutput out;
  char initbuffer[SOFIELD_GET_STACKBUFFER_SIZE];
  char * bufferptr = NULL; // indicates that initial buffer is on the stack

  CC_MUTEX_LOCK(sofield_mutex);
  SbBool ok = SoFieldP::getReallocHash()->put(initbuffer, &bufferptr);
  assert(ok);
  CC_MUTEX_UNLOCK(sofield_mutex);

  out.setBuffer(initbuffer, sizeof(initbuffer), SoFieldP::hashRealloc);

  // Record offset to skip header.
  out.write("");
  size_t offset;
  void * buffer;
  out.getBuffer(buffer, offset);

  // Write field..
  out.setStage(SoOutput::COUNT_REFS);
  this->countWriteRefs(&out);
  out.setStage(SoOutput::WRITE);
  this->writeValue(&out);

  // ..then read it back into the SbString.
  size_t size;
  out.getBuffer(buffer, size);
  valuestring = static_cast<char *>(buffer) + offset;

  // dealloc tmp memory buffer
  free(bufferptr);

  CC_MUTEX_LOCK(sofield_mutex);
  size_t isok = SoFieldP::getReallocHash()->erase(bufferptr ? bufferptr : initbuffer);
  assert(isok);
  CC_MUTEX_UNLOCK(sofield_mutex);
}

/*!
  Notify the field as well as the field's owner / container that it
  has been changed.

  Touching a field which is part of any component (engine or node) in
  a scene graph will lead to a forced redraw. This is useful if you
  have been doing several updates to the field wrapped in a pair of
  enableNotify() calls to notify the field's auditors that its value
  has changed.

  \sa setValue(), enableNotify()
*/
void
SoField::touch(void)
{
  if (this->container) this->startNotify();
}

/*!
  Trigger a notification sequence.

  At the end of a notification sequence, all "immediate" sensors
  (i.e. sensors set up with a zero priority) are triggered.
*/
void
SoField::startNotify(void)
{
  SoNotList l;
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::startNotify", "field %p (%s), list %p",
                         this, this->getTypeId().getName().getString(), &l);
#endif //COIN_DEBUG_EXTRA

  SoDB::startNotify();
  this->notify(&l);
  SoDB::endNotify();

#if COIN_DEBUG_EXTRA
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::startNotify", "DONE\n\n");
#endif //COIN_DEBUG_EXTRA
}

/*!
  Notify auditors that this field has changed.
*/
void
SoField::notify(SoNotList * nlist)
{
  assert((this->statusbits & FLAG_ALIVE_PATTERN) == FLAG_ALIVE_PATTERN);
#if COIN_DEBUG
  if (this->getContainer()) {
    this->getContainer()->assertAlive();
  }
#endif // COIN_DEBUG

  // check NotRec type to find if the notification was from a
  // connection. If someone changes the field directly we should
  // just continue.

  SoNotRec * rec = nlist->getLastRec();
  if (rec) {
    SoNotRec::Type t = nlist->getLastRec()->getType();
    if (t == SoNotRec::ENGINE || t == SoNotRec::FIELD) {
      if (this->hasExtendedStorage()) {
        this->storage->lastnotify = static_cast<void *>(rec->getBase());
      }
      // don't process the notification if we're notified from a
      // connection, and connection is disabled (through
      // enableConnection())
      if (!this->isConnectionEnabled()) return;
    }
  }

  // In Inventor it is legal to have circular field connections. This
  // test stops the notification from entering into an infinite
  // recursion because of such connections. The flag is set/cleared
  // before/after propagating the notification.
  if (this->getStatus(FLAG_ISNOTIFIED)) return;

  // needed because of the So[SF|MF]Node fields. When a node inside
  // such a field is changed we must mark the field as not default so
  // that SoWriteAction will export it. We can safely call
  // setDefault(FALSE) for other field types as well, since the only
  // other reason for entering here is if the field is connected from
  // an engine output or from another field.
  this->setDefault(FALSE);

#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    if (this != SoDB::getGlobalField("realTime")) {
      SoDebugError::postInfo("SoField::notify", "%p (%s (%s '%s')) -- start",
                             this,
                             this->getTypeId().getName().getString(),
                             this->getContainer() ? this->getContainer()->getTypeId().getName().getString() : "*none*",
                             this->getContainer() ? this->getContainer()->getName().getString() : "*none*");
    }
#endif //COIN_DEBUG_EXTRA

  // If we're not the originator of the notification process, we need
  // to be marked dirty, as it means something we're connected to as a
  // slave has changed and our value needs to be updated.
  //
  // Note: don't try to "optimize" code here by moving the setDirty()
  // call down into the isNotifyEnabled() check, as setDirty()
  // _should_ happen if we're not the originator -- no matter what the
  // status of the notification enable flag is.
  if (nlist->getFirstRec()) this->setDirty(TRUE);

  if (this->isNotifyEnabled()) {
    SoFieldContainer * cont = this->getContainer();
    this->setStatusBits(FLAG_ISNOTIFIED);
    SoNotRec rec(createNotRec(cont));
    nlist->append(&rec, this);
    nlist->setLastType(SoNotRec::CONTAINER); // FIXME: Not sure about this. 20000304 mortene.

#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::notify",
                           "field %p, list %p", this, nlist);
#endif //COIN_DEBUG_EXTRA

    if (this->hasExtendedStorage() && this->storage->auditors.getLength()) {
      // need to copy list first if we're going to notify the auditors
      SoNotList listcopy(*nlist);
      if (cont) cont->notify(nlist);
      this->notifyAuditors(&listcopy);
    }
    else {
      if (cont) cont->notify(nlist);
    }
    this->clearStatusBits(FLAG_ISNOTIFIED);
  }

#if COIN_DEBUG_EXTRA
  if (wLevel>=3)
    if (this != SoDB::getGlobalField("realTime")) {
      SoDebugError::postInfo("SoField::notify", "%p (%s (%s '%s')) -- done",
                             this,
                             this->getTypeId().getName().getString(),
                             this->getContainer() ? this->getContainer()->getTypeId().getName().getString() : "*none*",
                             this->getContainer() ? this->getContainer()->getName().getString() : "*none*");
    }
#endif //COIN_DEBUG_EXTRA
}

/*!
  This method sets whether notification will be propagated on changing
  the value of the field.  The old value of the setting is returned.

  \sa isNotifyEnabled()
*/
SbBool
SoField::enableNotify(SbBool on)
{
  const SbBool old = this->getStatus(FLAG_DONOTIFY);
  (void) this->changeStatusBits(FLAG_DONOTIFY, on);
  return old;
}

/*!
  This method returns whether notification of changes to the field
  value are propagated to the auditors.

  \sa enableNotify()
*/
SbBool
SoField::isNotifyEnabled(void) const
{
  return this->getStatus(FLAG_DONOTIFY);
}

// Makes an extended storage block on first connection.
void
SoField::extendStorageIfNecessary(void)
{
  if (!this->hasExtendedStorage()) {
    this->storage = new SoConnectStorage(this->container, this->getTypeId());
    this->setStatusBits(FLAG_EXTSTORAGE);
  }
}

/*!
  Add an auditor to the list. All auditors will be notified whenever
  this field changes its value(s).
*/
void
SoField::addAuditor(void * f, SoNotRec::Type type)
{
  this->extendStorageIfNecessary();
  this->storage->auditors.append(f, type);
  this->connectionStatusChanged(+1);
}

/*!
  Remove an auditor from the list.
*/
void
SoField::removeAuditor(void * f, SoNotRec::Type type)
{
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::removeAuditor",
                           "%p removing %p", this, f);
#endif //COIN_DEBUG_EXTRA

  assert(this->hasExtendedStorage());
  this->storage->auditors.remove(f, type);
  this->connectionStatusChanged(-1);
}

/*!
  Checks for equality. Returns \c 0 if the fields are of different
  type or the field's value(s) are not equal.
*/
int
SoField::operator ==(const SoField & f) const
{
  return this->isSame(f);
}

/*!
  Returns \c TRUE if the fields are of different type or have different
  value.
*/
int
SoField::operator !=(const SoField & f) const
{
  return !this->isSame(f);
}

/*!
  Returns \c TRUE if it is necessary to write the field when dumping a
  scene graph. This needs to be done if the field is not default (it
  has been changed from its default value), if it is ignored, or if
  it is connected from another field or engine.
*/
SbBool
SoField::shouldWrite(void) const
{
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3) {
    SbString finfo = SoFieldP::getDebugIdString(this);
    SoDebugError::postInfo("SoField::shouldWrite",
                           "%s: isDefault==%d, isIgnored==%d, isConnected==%d",
                           finfo.getString(), this->isDefault(),
                           this->isIgnored(), this->isConnected());
  }
#endif //COIN_DEBUG_EXTRA

  if (!this->isDefault()) return TRUE;
  if (this->isIgnored()) return TRUE;

  if (this->isConnected()) {
#if 0 // disabled (was only needed for bidirectional connections in PROTOs)
    // I suspect this code was here only to make the bidirectional
    // connection hack in SoProto work. Connected PROTO instance
    // fields should be written even if they have the default value
    // (just like any other field). pederb, 2005-12-20
    SoFieldContainer * thecontainer = this->getContainer();
    if ( thecontainer != NULL &&
         thecontainer->isOfType(SoProtoInstance::getClassTypeId()) ) {
      // PROTO instance fields are usually connected, but we don't want to
      // write out PROTO instance fields that contain default values - they
      // will be hooked up and get the default value from the PROTO interface
      // when they are read in again later anyways. -- 20040115 larsa
      return FALSE;
    }
#endif // disabled PROTO hack
    return TRUE;
  }

  // SGI Inventor seems to test forward connections here also. We
  // consider this is bug, since this field should not write just
  // because some field is connected from this field.  pederb.

  return FALSE;
}

/*!
  Called whenever another slave attaches or detaches itself to us.  \a
  numconnections is the difference in number of connections made
  (i.e. if stuff is \e disconnected, \a numconnections will be a
  negative number).

  The default method is empty. Override in subclasses if you want do
  something special on connections/disconnections.
*/
void
SoField::connectionStatusChanged(int COIN_UNUSED_ARG(numconnections))
{
}

/*!
  Returns \c TRUE if this field should not be written into at the
  moment the method is called.

  This method is used internally in Coin during notification and
  evaluation processes, and should normally not be of interest to the
  application programmer.
*/
SbBool
SoField::isReadOnly(void) const
{
  return this->getStatus(FLAG_READONLY);
}

/*!
  This method is internally called after SoField::copyFrom() during
  scene graph copies, and should do the operations necessary for
  fixing up the field instance after it has gotten a new value.

  The default method in the SoField superclass does nothing.

  The application programmer should normally not need to consider this
  method, unless he constructs a complex field type which contains new
  references to container instances (i.e. nodes or
  engines). Overriding this method is then necessary to update the
  reference pointers, as they could have been duplicated during the
  copy operation.
*/
void
SoField::fixCopy(SbBool COIN_UNUSED_ARG(copyconnections))
{
}

/*!
  Returns \c TRUE if this field has references to any containers in
  the scene graph which are also duplicated during the copy operation.

  Note that this method \e only is valid to call during copy
  operations.

  See also the note about the relevance of the fixCopy() method for
  application programmers, as it is applicable on this method as well.
*/
SbBool
SoField::referencesCopy(void) const
{
  int i, n;
  if (!this->hasExtendedStorage()) return FALSE;

  const SoFieldList & masterfields = this->storage->masterfields;
  n = masterfields.getLength();
  for (i = 0; i < n; i++) {
    SoFieldContainer * fc = masterfields[i]->getContainer();
    if (SoFieldContainer::checkCopy(fc)) return TRUE;
  }

  const SoEngineOutputList & masterengineouts =
    this->storage->masterengineouts;
  n = masterengineouts.getLength();
  for (i = 0; i < n; i++) {
    SoEngineOutput * eout = masterengineouts[i];
    SbBool isengine = ! eout->isNodeEngineOutput();
    SoFieldContainer * fc = isengine ?
      coin_assert_cast<SoFieldContainer *>(eout->getContainer()) :
      coin_assert_cast<SoFieldContainer *>(eout->getNodeContainer());
    if (SoFieldContainer::checkCopy(fc)) return TRUE;
    if (isengine || (fc->isOfType(SoEngine::getClassTypeId()) &&
                     coin_assert_cast<SoEngine *>(fc)->shouldCopy())) return TRUE;
  }
  return FALSE;
}

/*!
  If \a fromfield contains a connection to another field, make this
  field also use the same connection.
*/
void
SoField::copyConnection(const SoField * fromfield)
{
  // Consider most common case first.
  if (!fromfield->isConnected()) return;

  // first, disconnect all existing connections (engines often
  // connect to the realTime global field in the constructor).
  this->disconnect();

  assert(fromfield->hasExtendedStorage());
  int i;

  for (i = 0; i < fromfield->storage->masterfields.getLength(); i++) {
    SoField * master = fromfield->storage->masterfields[i];
    SoFieldContainer * masterfc = master->getContainer();
    SbName fieldname;
    (void) masterfc->getFieldName(master, fieldname);
    SoFieldContainer * copyfc = masterfc->copyThroughConnection();
    SoField * copyfield = copyfc->getField(fieldname);

    SbBool notnotify = FALSE;
    switch (master->getFieldType()) {
    case EVENTIN_FIELD:
    case EVENTOUT_FIELD:
      notnotify = TRUE;
      break;
    default:
      break;
    }
    (void) this->connectFrom(copyfield, notnotify, TRUE);
  }
  for (i = 0; i < fromfield->storage->masterengineouts.getLength(); i++) {
    SoEngineOutput * master = fromfield->storage->masterengineouts[i];
    SoEngineOutput * copyeo = NULL;

    if (master->isNodeEngineOutput()) {
      SbName name;
      SoNodeEngine * masterengine = master->getNodeContainer();
      (void) masterengine->getOutputName(master, name);
      SoNodeEngine * copyengine =
       coin_assert_cast<SoNodeEngine *>(masterengine->copyThroughConnection());
      copyeo = copyengine->getOutput(name);
    }
    else {
      SbName name;
      SoEngine * masterengine = master->getContainer();
      (void) masterengine->getOutputName(master, name);
      SoEngine * copyengine =
       coin_assert_cast<SoEngine *>(masterengine->copyThroughConnection());
      copyeo = copyengine->getOutput(name);
    }
    assert(copyeo);
    (void) this->connectFrom(copyeo, FALSE, TRUE);
  }
}

// This templatized inline is just a convenience function for reading
// with error detection.
template <class Type>
static inline SbBool
READ_VAL(SoInput * in, Type & val)
{
  if (!in->read(val)) {
    SoReadError::post(in, "Premature end of file");
    return FALSE;
  }
  return TRUE;
}


/*!
  Reads and sets the value of this field from the given SoInput
  instance.  Returns \c FALSE if the field value cannot be parsed
  from the input.

  The second argument is the field's context-specific \a name, which
  is typically its unique identifier in its field container.

  \sa set(), write()
*/
SbBool
SoField::read(SoInput * in, const SbName & name)
{
  SbBool readok = TRUE;
  SbBool oldnotify = this->enableNotify(FALSE);
  SbBool didreadvalue = FALSE;

  if (in->checkISReference(this->getContainer(), name, readok) || readok == FALSE) {
    if (!readok) {
      SoFieldContainer * fc = this->getContainer();
      SbString s("");
      if (fc) { s.sprintf(" of %s", fc->getTypeId().getName().getString()); }
      SoReadError::post(in, "Couldn't read value for field \"%s\"%s",
                        name.getString(), s.getString());
    }
    goto sofield_read_return;
  }

  this->setDefault(FALSE);
  this->setDirty(FALSE);

  if (!in->isBinary()) { // ASCII file format.
    char c;
    // Check for the ignored flag first, as it is valid to let the
    // field data be just the ignored flag and nothing else.
    if (!READ_VAL(in, c)) { readok = FALSE; goto sofield_read_return; }

    if (c == IGNOREDCHAR) this->setIgnored(TRUE);
    else {
      // First check if there's a field-to-field connection here as
      // the default value following the field can be omitted.
      if (c == CONNECTIONCHAR) {
        // There's potential for an obscure bug to happen here: if the
        // field is an SoSFString where the string is unquoted and
        // starts with a CONNECTIONCHAR (i.e. '='), it will lead to a
        // false positive for the if()-check below, which again causes a
        // rather obtuse error message:
        //
        // Coin read error: Expected '{'; got '}'
        //     Occurred at line   3 in hepp.iv
        //
        // For the following test file:
        //
        // ----8<---- [snip] -------8<----
        // #Inventor V2.1 ascii
        //
        // Info { string =moo }
        // ----8<---- [snip] -------8<----
        //
        // Tamer Fahmy investigated and found this behavior to also
        // happen for SGI Inventor. Since that is the case, we won't
        // try to handle this as a valid file construct.
        //
        // FIXME: it would be nice if we could improve the error
        // message, to let the app programmer actually stand a chance
        // of debugging this when it happens. 20030811 mortene.
        if (!this->readConnection(in)) { readok = FALSE; goto sofield_read_return; }
        goto sofield_read_return;
      }
      else in->putBack(c);

      // Read field value(s).
      if (!this->readValue(in)) {
        SoFieldContainer * fc = this->getContainer();
        SbString s("");
        if (fc) { s.sprintf(" of %s", fc->getTypeId().getName().getString()); }
        SoReadError::post(in, "Couldn't read value for field \"%s\"%s",
                          name.getString(), s.getString());
        readok = FALSE;
        goto sofield_read_return;
      }
      else didreadvalue = TRUE;

      // Check again for the ignored flag indicator after the field
      // value.
      if (in->read(c)) { // if-check in case EOF on an SoField::set() invocation
        if (c == IGNOREDCHAR) this->setIgnored(TRUE);
        else in->putBack(c);
      }
    }

    // Check field-to-field connection indicator again /after/ the
    // field (start-)value.
    if (in->read(c)) { // if-check in case EOF on an SoField::set() invocation
      if (c == CONNECTIONCHAR) { if (!this->readConnection(in)) { readok = FALSE; goto sofield_read_return; } }
      else { in->putBack(c); }
    }
  }
  else { // Binary file format.
    // Read field value(s).
    if (!this->readValue(in)) {
      SoFieldContainer * fc = this->getContainer();
      SbString s("");
      if (fc) { s.sprintf(" of %s", fc->getTypeId().getName().getString()); }
      SoReadError::post(in, "Couldn't read value for field \"%s\"%s",
                        name.getString(), s.getString());
      readok = FALSE;
      goto sofield_read_return;
    }
    else didreadvalue = TRUE;

    // Check for the "ignored", "connection" and "default" flags.
    unsigned int flags;
    if (!READ_VAL(in, flags)) { readok = FALSE; goto sofield_read_return; }

    if (flags & SoField::IGNORED) this->setIgnored(TRUE);
    if (flags & SoField::CONNECTED) { if (!this->readConnection(in)) { readok = FALSE; goto sofield_read_return; }}
    if (flags & SoField::DEFAULT) this->setDefault(TRUE);

#if COIN_DEBUG
    if (flags & ~SoField::ALLFILEFLAGS) {
      SoDebugError::postWarning("SoField::read",
                                "unknown field flags (0x%x) -- "
                                "please report to <coin-support@coin3d.org>",
                                flags);
    }
#endif // COIN_DEBUG
  }

 sofield_read_return:
  (void) this->enableNotify(oldnotify);

  if (readok) {
    if (didreadvalue) this->valueChanged(FALSE);
    else {
      // we called setDirty(FALSE) in the beginning of the function.
      // Since this field is read without a value (just connected to
      // some other field/engine), we need to mark the field as dirty
      // so that it is evaluated the next time the field is read
      this->setDirty(TRUE);
      this->startNotify();
    }
  }
  return readok;
}

/*!
  Write the value of the field to the given SoOutput instance (which
  can be either a memory buffer or a file, in ASCII or in binary
  format).

  \sa get(), read(), SoOutput
*/
void
SoField::write(SoOutput * out, const SbName & name) const
{
  if (out->getStage() == SoOutput::COUNT_REFS) {
    // Handle first stage of write operations.
    this->countWriteRefs(out);
    return;
  }

  // Ok, we've passed the first write stage and is _really_ writing.

  // Check connection (this is common code for ASCII and binary
  // write).
  SbBool writeconnection = FALSE;
  SbName dummy;
  SoFieldContainer * fc = this->resolveWriteConnection(dummy);

  if (fc && (SoWriterefCounter::instance(out)->shouldWrite(fc) || fc->isOfType(SoEngine::getClassTypeId())))
    writeconnection = TRUE;

  // check VRML2 connections. Since VRML2 fields can have multiple
  // master fields/engines, the field can still be default even though
  // it is connected, and we should _not_ write the field. The ROUTEs
  // should be added though. pederb, 2002-06-13

  if (is_vrml2_field(this)) {
    if (writeconnection) {
      writeconnection = FALSE;
      this->storage->add_vrml2_routes(out, this);
      // if no value has been set, don't write field even if it is
      // connected
      if (this->isDefault()) return;
    }
    // never write eventIn or eventOut fields
    if ((this->getFieldType() == SoField::EVENTIN_FIELD) ||
        (this->getFieldType() == SoField::EVENTOUT_FIELD)) return;
  }

  // ASCII write.
  if (!out->isBinary()) {
    out->indent();
    // Cast to avoid "'s.
    out->write(static_cast<const char *>(name));
    if (!this->isDefault()) {
      out->write(' ');
      this->writeValue(out);
    }
    if (this->isIgnored()) {
      out->write(' ');
      out->write(IGNOREDCHAR);
    }

    if (writeconnection) this->writeConnection(out);
    out->write('\n');
  }
  // Binary write.
  else {
    // Cast to avoid "'s.
    out->write(static_cast<const char *>(name));
    this->writeValue(out);

    unsigned int flags = 0;
    if (this->isIgnored()) flags |= SoField::IGNORED;
    if (writeconnection) flags |= SoField::CONNECTED;
    if (this->isDefault()) flags |= SoField::DEFAULT;

    out->write(flags);

    if (writeconnection) this->writeConnection(out);
  }
}

#include <cstdio>

/*!
  This method is called during the first pass of write operations, to
  count the number of write references to this field and to "forward"
  the reference counting operation to the field containers we're
  connected to.
*/
void
SoField::countWriteRefs(SoOutput * out) const
{
  // Count all connected fields/engines. Inventor only allows one
  // master field/engine, but VRML2 can have multiple. This code
  // should work for both Inventor and VRML2 scene graphs
  // though. pederb, 2002-06-13
  if (this->isConnected()) {
    if (is_vrml2_field(this)) {
      this->storage->add_vrml2_routes(out, this);
    }
    else {
      int i;
      for (i = 0; i < this->storage->masterfields.getLength(); i++) {
        SoField * master = this->storage->masterfields[i];
        SoFieldContainer * fc = master->getContainer();
        assert(fc);
        // TRUE = reference is from field connection. This is needed
        // so that the fields inside 'fc' is counted only once
        fc->addWriteReference(out, TRUE);
      }
      for (i = 0; i < this->storage->masterengineouts.getLength(); i++) {
        SoEngineOutput * engineout = this->storage->masterengineouts[i];
        SoFieldContainer * fc = engineout->getFieldContainer();
        assert(fc);
        // since engines are always connected directly to the field
        // (they're not nodes), engines are always counted with
        // isfromfield = FALSE
        fc->addWriteReference(out, FALSE);
      }
    }
  }
}

/*!
  \fn void SoField::evaluate(void) const

  Re-evaluates the value of this field any time a getValue() call is
  made and the field is marked dirty. This is done in this way to gain
  the advantages of having lazy evaluation.
*/

//
// private method called from SoField::evaluate() when the field is
// connected and dirty
//
void
SoField::evaluateField(void) const
{
  // if we're destructing, don't continue as this would cause
  // a call to the virtual evaluateConnection()
  if (this->getStatus(FLAG_ISDESTRUCTING)) {
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3) {
    SoDebugError::postInfo("SoField::evaluate",
                           "Stopped evaluate while destructing.");
  }
#endif //COIN_DEBUG_EXTRA
    return;
  }

  if (!this->isConnected()) return;

  assert(this->storage != NULL);

  // lock _before_ testing FLAG_ISEVALUATING to be thread safe
  SOFIELD_RECLOCK;
  // Recursive calls to SoField::evaluate() should _absolutely_ not
  // happen, as the state of the field variables might not be
  // consistent while evaluating.
  //
  // This is an error which is not too hard to bump into, and the
  // _immediate_ repercussions are non-fatal, so we just spit out this
  // error message and carry on -- to not cause any more application
  // programmer frustrations than necessary.

  if (this->getStatus(FLAG_ISEVALUATING)) {
#if COIN_DEBUG
    SoDebugError::post("SoField::evaluate",
                       "Detected indirectly recursive call to "
                       "SoField::evaluate() -- which is a *bad* thing."
                       "This indicates a non-trivial programming error "
                       "somewhere either in the application (most likely) "
                       "or the library code itself (less likely). "
                       "We strongly advise you to investigate and resolve "
                       "this issue before moving on.");
#endif // COIN_DEBUG
    SOFIELD_RECUNLOCK;
    return;
  }

  // Cast away the const. (evaluate() must be const, since we're using
  // evaluate() from getValue().)
  SoField * that = const_cast<SoField *>(this);

  // Check the NEEDEVALUATION flag in case some other thread has just
  // evaluated the field. The flag is checked in SoField::evaluate(),
  // but it is possible that two (or more) threads might enter
  // evaluateField() simultaneously, so this test is necessary.
  // pederb, 2002-10-04
  if (this->getStatus(FLAG_NEEDEVALUATION) && this->getStatus(FLAG_ENABLECONNECTS)) {
    that->setStatusBits(FLAG_ISEVALUATING);
    this->evaluateConnection();
    that->clearStatusBits(FLAG_ISEVALUATING);
    // this will clear the NEEDEVALUATION flag
    that->setDirty(FALSE);
  }
  SOFIELD_RECUNLOCK;
}

/*!
  Do we need re-evaluation?
*/
SbBool
SoField::getDirty(void) const
{
  return this->getStatus(FLAG_NEEDEVALUATION);
}

/*!
  Mark field for re-evaluation (upon next read operation), but do not
  trigger a notification.
*/
void
SoField::setDirty(SbBool dirty)
{
  COIN_CHECK_THREAD();
  (void) this->changeStatusBits(FLAG_NEEDEVALUATION, dirty);
}

/*!
  Connect ourself as slave to another object, while still keeping the
  other connections currently in place.

  \sa connectFrom()
*/
SbBool
SoField::appendConnection(SoEngineOutput * master, SbBool notnotify)
{
  return this->connectFrom(master, notnotify, TRUE);
}

/*!
  Connect ourself as slave to another object, while still keeping the
  other connections currently in place.

  \sa connectFrom()
*/
SbBool
SoField::appendConnection(SoField * master, SbBool notnotify)
{
  return this->connectFrom(master, notnotify, TRUE);
}

// Make a converter from value(s) of the given field type and the
// value(s) of this type. Returns NULL if no value conversion between
// types is possible.
SoFieldConverter *
SoField::createConverter(SoType from) const
{
  SoType thistype = this->getTypeId();
  assert(from != thistype);
  SoType convtype = SoDB::getConverter(from, thistype);
  if (convtype == SoType::badType()) {
#if COIN_DEBUG // COIN_DEBUG
    SoDebugError::postWarning("SoField::createConverter",
                              "no converter for %s to %s",
                              from.getName().getString(),
                              thistype.getName().getString());
#endif // COIN_DEBUG
    return NULL;
  }

  SoFieldConverter * fc;

  if (convtype == SoConvertAll::getClassTypeId())
    fc = new SoConvertAll(from, this->getTypeId());
  else
    fc = static_cast<SoFieldConverter *>(convtype.createInstance());

  fc->ref();
  return fc;
}


/*!
  Read the master field of a field-to-field connection (and its field
  container).

  If input parsing is successful, this field will be connected as a
  slave to the master field.

  Note that this slave field will \e not be marked as "dirty" upon
  connection, i.e. it will retain its value until the first update of
  the master field is made \e after the connection was set up. This to
  be in conformance with how the Inventor Mentor specifies how field
  connections should be imported (see page 270).
*/
SbBool
SoField::readConnection(SoInput * in)
{
  // For debugging purposes, here's a handy test case for checking
  // that a field-field connection, where an initial value for the
  // slave is given, will behave according to the Mentor, as mentioned
  // above in the function API documentation:
  //
  // -----8<------- [snip] -----------------8<------- [snip] -----------
  // #Inventor V2.1 ascii
  //
  // DEF SCENE_ROOT Separator {
  //    ## on startup this should give a green cube
  //    Switch {
  //       whichChild 0 = SelectOne { type SoMFInt32 index 1 input [ 0,1 ] }.output
  //       Material { diffuseColor 0.1 1.0 0.1 }
  //       Material { diffuseColor 1.0 0.1 0.1 }
  //    }
  //    Cone {}
  // } # SCENE_ROOT
  // -----8<------- [snip] -----------------8<------- [snip] -----------
  //
  // (Provided by Gerhard Reitmayr.)

  // ***********************************************************************

  // Read the fieldcontainer instance containing the master field
  // we're connected to.
  SoBase * bp;
  if (!SoBase::read(in, bp, SoFieldContainer::getClassTypeId())) return FALSE;
  if (!bp) {
    SoReadError::post(in, "couldn't read field-to-field connection");
    return FALSE;
  }

  SoFieldContainer * fc = coin_assert_cast<SoFieldContainer *>(bp);

  // Scan past the '.' character for ASCII format.
  if (!in->isBinary()) {
    char c;
    if (!in->read(c)) {
      SoReadError::post(in, "premature EOF");
      return FALSE;
    }
    if (c != '.') {
      SoReadError::post(in, "expected field connection token '.', "
                        "but got '%c'", c);
      return FALSE;
    }
  }

  // Read name of master field.
  SbName mastername;
  if (!in->read(mastername, TRUE)) {
    SoReadError::post(in, "premature EOF");
    return FALSE;
  }

  // Get pointer to master field or engine output and connect.

  SoEngineOutput * masteroutput = NULL;
  SoField * masterfield = fc->getField(mastername);

  if (!masterfield) {
    masteroutput =
      fc->isOfType(SoEngine::getClassTypeId()) ?
      coin_safe_cast<SoEngine*>(fc)->getOutput(mastername) : NULL;

    if (!masteroutput) {
      masteroutput =
        fc->isOfType(SoNodeEngine::getClassTypeId()) ?
        coin_safe_cast<SoNodeEngine *>(fc)->getOutput(mastername) : NULL;
    }
  }

  if (!masterfield && !masteroutput) {
    SoReadError::post(in, "no field or output \"%s\" in \"%s\"",
                      mastername.getString(),
                      fc->getTypeId().getName().getString());
    return FALSE;
  }

  SbBool ok = FALSE;

  // Make connection, with "do not notify" flag set to TRUE, to avoid
  // making ourselves "dirty" (i.e.: we will continue using our
  // current value until the master is updated).
  if (masterfield) { ok = this->connectFrom(masterfield, TRUE); }
  else if (masteroutput) { ok = this->connectFrom(masteroutput, TRUE); }

  if (!ok) {
    SoReadError::post(in, "couldn't connect \"%s\" field to \"%s\", "
                      "connection will be ignored",
                      this->getTypeId().getName().getString(),
                      mastername.getString());
  }

  return TRUE;
}

/*!
  Write out information about this field's connection.
*/
void
SoField::writeConnection(SoOutput * out) const
{
  SbName mastername;
  SoFieldContainer * fc = this->resolveWriteConnection(mastername);
  assert(fc);

  if (!out->isBinary()) {
    out->write(' ');
    out->write(CONNECTIONCHAR);
  }

  if (fc->isOfType(SoNode::getClassTypeId())) {
    SoWriteAction wa(out);
    wa.continueToApply(coin_assert_cast<SoNode *>(fc));
  }
  else {
    // Note: for this to work, classes inheriting SoFieldContainer
    // which are _not_ also inheriting from SoNode must call
    // SoBase::writeHeader() and SoBase::writeFooter().
    fc->writeInstance(out);
    // FIXME: does this work for engines? 20000131 mortene.
  }

  if (!out->isBinary()) {
    out->indent();
    out->write(". ");
  }

  out->write(mastername.getString());
}

// Check if this field should write a connection upon export. Returns
// a pointer to the field container with the master field we're
// connected to (or NULL if none, or if the master field's container
// is not within the scene graph). If the return value is non-NULL, the
// name of the master field is copied to the mastername argument.
SoFieldContainer *
SoField::resolveWriteConnection(SbName & mastername) const
{
  if (!this->isConnected()) return NULL;

  SoFieldContainer * fc = NULL;
  SoField * fieldmaster;
  SoEngineOutput * enginemaster;

  if (this->getConnectedField(fieldmaster)) {
    fc = fieldmaster->getContainer();
    assert(fc);
    SbBool ok = fc->getFieldName(fieldmaster, mastername);
    assert(ok);
  }
  else if (this->getConnectedEngine(enginemaster)) {
    fc = enginemaster->getFieldContainer();
    assert(fc);
    // FIXME: couldn't we use getFieldName()? 20000129 mortene.
    SbBool ok =
      enginemaster->isNodeEngineOutput() ?
      coin_assert_cast<SoNodeEngine *>(fc)->getOutputName(enginemaster, mastername) :
      coin_assert_cast<SoEngine *>(fc)->getOutputName(enginemaster, mastername);
    assert(ok);
  }
  else assert(FALSE);

  return fc;
}


/*!
  If we're connected to a field/engine/interpolator, copy the value
  from the master source.
*/
void
SoField::evaluateConnection(void) const
{
  SbBool fanin = this->storage->hasFanIn();
  SbBool didevaluate = FALSE;

  // FIXME: should we evaluate from all masters in turn? 19990623 mortene.
  if (this->isConnectedFromField()) {
    int idx = fanin ? this->storage->findFanInField() : this->storage->masterfields.getLength() - 1;
    if (idx >= 0) {
      didevaluate = TRUE;
      SoField * master = this->storage->masterfields[idx];
      // don't copy if master is destructing, or if master is currently
      // evaluating. The master might be evaluating if we have circular
      // field connections. If this is the case, the field will already
      // contain the correct value, and we should not copy again.
      if (!master->isDestructing() && !master->getStatus(FLAG_ISEVALUATING)) {
        SoFieldConverter * converter = this->storage->findConverter(master);
        if (converter) converter->evaluateWrapper();
        else {
          SoField * that = const_cast<SoField *>(this); // cast away const
          // Copy data. Disable notification first since notification
          // has already been sent from the master.
          SbBool oldnotify = that->enableNotify(FALSE);
          that->copyFrom(*master);
          (void) that->enableNotify(oldnotify);
        }
      }
    }
  }
  if (this->isConnectedFromEngine() && !didevaluate) {
    int idx = fanin ? this->storage->findFanInEngine() : this->storage->masterengineouts.getLength() - 1;
    if (idx >= 0) {
      SoEngineOutput * master = this->storage->masterengineouts[idx];
      SoFieldConverter * converter = this->storage->findConverter(master);
      if (converter) converter->evaluateWrapper();
      else if (master->isNodeEngineOutput()) {
        master->getNodeContainer()->evaluateWrapper();
      }
      else {
        master->getContainer()->evaluateWrapper();
      }
    }
  }
}

/*!
  This method is always called whenever the field's value has been
  changed by direct invocation of setValue() or some such. You should
  \e never call this method from anywhere in the code where the field
  value is being set through an evaluation of its connections.

  If \a resetdefault is \c TRUE, the flag marking whether or not the
  field has its default value will be set to \c FALSE.

  The method will also notify any auditors that the field's value has
  changed.
*/
void
SoField::valueChanged(SbBool resetdefault)
{
  if (this->changeStatusBits(FLAG_READONLY, TRUE)) {
    this->setDirty(FALSE);
    if (resetdefault) this->setDefault(FALSE);
    if (this->container) this->startNotify();
    this->clearStatusBits(FLAG_READONLY);
  }
}

// Notify any auditors by marking them dirty - i.e. ready for
// re-evaluation.  Auditors include connected fields, sensors,
// containers (nodes/engines), ...
void
SoField::notifyAuditors(SoNotList * l)
{
#if COIN_DEBUG_EXTRA
  int wLevel =
    SoConfigSettings::getInstance()->settingAsInt("COIN_WARNING_LEVEL");
  if (wLevel>=3)
    SoDebugError::postInfo("SoField::notifyAuditors",
                           "field %p, list %p", this, l);
#endif //COIN_DEBUG_EXTRA
  if (this->hasExtendedStorage() && this->storage->auditors.getLength())
    this->storage->auditors.notify(l);
}

/*!
  Set type of this field.

  The possible values for \a type is: 0 for ordinary fields, 1 for
  eventIn fields, 2 for eventOut fields, 3 for internal fields, 4 for
  VRML2 exposedField fields. There are also enum values in SoField.h.
*/
void
SoField::setFieldType(int type)
{
  this->clearStatusBits(FLAG_TYPEMASK);
  assert(type >=0 && type <= FLAG_TYPEMASK);
  this->setStatusBits(static_cast<unsigned int>(type));
}

/*!
  Return the type of this field.

  \sa setFieldType()
*/
int
SoField::getFieldType(void) const
{
  return this->statusbits & FLAG_TYPEMASK;
}

/*!
  Can be used to check if a field is being destructed.
*/
SbBool
SoField::isDestructing(void) const
{
  return this->getStatus(FLAG_ISDESTRUCTING);
}

/*!
  \internal
*/
SoNotRec
SoField::createNotRec(SoBase * cont)
{
  SoNotRec rec(cont);
  rec.setOperationType(SoNotRec::FIELD_UPDATE);
  return rec;
}

/*!
  Initialize all the field classes.
*/
void
SoField::initClasses(void)
{
  SoSField::initClass();
  SoSFBox2s::initClass();
  SoSFBox2i32::initClass();
  SoSFBox2f::initClass();
  SoSFBox2d::initClass();
  SoSFBox3s::initClass();
  SoSFBox3i32::initClass();
  SoSFBox3f::initClass();
  SoSFBox3d::initClass();
  SoSFBool::initClass();
  SoSFColor::initClass();
  SoSFColorRGBA::initClass();
  SoSFDouble::initClass();
  SoSFEngine::initClass();
  SoSFFloat::initClass();
  SoSFShort::initClass();
  SoSFUShort::initClass();
  SoSFInt32::initClass();
  SoSFUInt32::initClass();
  SoSFVec2b::initClass();
  SoSFVec2s::initClass();
  SoSFVec2i32::initClass();
  SoSFVec2f::initClass();
  SoSFVec2d::initClass();
  SoSFVec3b::initClass();
  SoSFVec3s::initClass();
  SoSFVec3i32::initClass();
  SoSFVec3f::initClass();
  SoSFVec3d::initClass();
  SoSFVec4b::initClass();
  SoSFVec4ub::initClass();
  SoSFVec4s::initClass();
  SoSFVec4us::initClass();
  SoSFVec4i32::initClass();
  SoSFVec4ui32::initClass();
  SoSFVec4f::initClass();
  SoSFVec4d::initClass();
  SoSFMatrix::initClass();
  SoSFEnum::initClass();
  SoSFBitMask::initClass();
  SoSFImage::initClass();
  SoSFImage3::initClass();
  SoSFName::initClass();
  SoSFNode::initClass();
  SoSFPath::initClass();
  SoSFPlane::initClass();
  SoSFRotation::initClass();
  SoSFString::initClass();
  SoSFTime::initClass();
  SoSFTrigger::initClass();

  SoMField::initClass();
  SoMFBool::initClass();
  SoMFColor::initClass();
  SoMFColorRGBA::initClass();
  SoMFDouble::initClass();
  SoMFEngine::initClass();
  SoMFEnum::initClass();
  SoMFBitMask::initClass();
  SoMFFloat::initClass();
  SoMFInt32::initClass();
  SoMFMatrix::initClass();
  SoMFName::initClass();
  SoMFNode::initClass();
  SoMFPath::initClass();
  SoMFPlane::initClass();
  SoMFRotation::initClass();
  SoMFShort::initClass();
  SoMFString::initClass();
  SoMFTime::initClass();
  SoMFUInt32::initClass();
  SoMFUShort::initClass();
  SoMFVec2b::initClass();
  SoMFVec2s::initClass();
  SoMFVec2i32::initClass();
  SoMFVec2f::initClass();
  SoMFVec2d::initClass();
  SoMFVec3b::initClass();
  SoMFVec3s::initClass();
  SoMFVec3i32::initClass();
  SoMFVec3f::initClass();
  SoMFVec3d::initClass();
  SoMFVec4b::initClass();
  SoMFVec4ub::initClass();
  SoMFVec4s::initClass();
  SoMFVec4us::initClass();
  SoMFVec4i32::initClass();
  SoMFVec4ui32::initClass();
  SoMFVec4f::initClass();
  SoMFVec4d::initClass();

  // Create these obsoleted types for backwards compatibility. They
  // are typedef'ed to the types which obsoleted them, but this is
  // needed so it will also be possible to use SoType::fromName() with
  // the old names and create instances in that manner.
  //
  // FIXME: SoType::fromName("oldname") == SoType::fromName("newname")
  // will fail, but this can be solved with a hack in
  // SoType::operator==(). Do we _want_ to implement this hack,
  // though? It'd be ugly as hell.  19991109 mortene.
  // Does it need to be so ugly?  == could compare createInstance
  // pointers if both have is set?  But would it be correct, and would
  // any code depend on or benefit from such behaviour?  20070518 larsa
  SoType::createType(SoSField::getClassTypeId(), "SFLong",
                     &SoSFInt32::createInstance);
  SoType::createType(SoSField::getClassTypeId(), "SFULong",
                     &SoSFUInt32::createInstance);
  SoType::createType(SoMField::getClassTypeId(), "MFLong",
                     &SoMFInt32::createInstance);
  SoType::createType(SoMField::getClassTypeId(), "MFULong",
                     &SoMFUInt32::createInstance);
}

#undef FLAG_ALIVE_PATTERN
#undef SOFIELD_RECLOCK
#undef SOFIELD_RECUNLOCK
