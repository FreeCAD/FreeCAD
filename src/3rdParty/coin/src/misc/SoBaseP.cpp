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

#include <Inventor/misc/SoBase.h>
#include "misc/SoBaseP.h"

#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/SoInput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SbPList.h>
#include <Inventor/lists/SoAuditorList.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/SoDB.h>
#include <Inventor/fields/SoField.h>

#include "threads/threadsutilp.h"
#include "upgraders/SoUpgrader.h"
#include "nodes/SoUnknownNode.h"
#include "fields/SoGlobalField.h"
#include "io/SoInputP.h"

// *************************************************************************

// Strings and character tokens used in parsing.
const char SoBase::PImpl::OPEN_BRACE = '{';
const char SoBase::PImpl::CLOSE_BRACE = '}';

const char SoBase::PImpl::END_OF_LINE[] = "\n";
const char SoBase::PImpl::DEF_KEYWORD[] = "DEF";
const char SoBase::PImpl::USE_KEYWORD[] = "USE";
const char SoBase::PImpl::NULL_KEYWORD[] = "NULL";
const char SoBase::PImpl::ROUTE_KEYWORD[] = "ROUTE";

const char SoBase::PImpl::PROTO_KEYWORD[] = "PROTO";
const char SoBase::PImpl::EXTERNPROTO_KEYWORD[] = "EXTERNPROTO";

void * SoBase::PImpl::mutex = NULL;
void * SoBase::PImpl::name2obj_mutex = NULL;
void * SoBase::PImpl::obj2name_mutex = NULL;
void * SoBase::PImpl::auditor_mutex = NULL;
void * SoBase::PImpl::global_mutex = NULL;

SbHash<const SoBase *, SoAuditorList *> * SoBase::PImpl::auditordict = NULL;

// Only a small number of SoBase derived objects will under usual
// conditions have designated names, so we use a couple of static
// dictionary objects to keep track of them. Since we avoid storing a
// pointer for each and every object, we'll cut down on a decent
// amount of memory use this way (SoBase should be kept as slim as
// possible, as any dead weight is brought along in a lot of objects).
SbHash<const char *, SbPList *> * SoBase::PImpl::name2obj = NULL;
SbHash<const SoBase *, const char *> * SoBase::PImpl::obj2name = NULL;

// This is used for debugging purposes: it stores a pointer to all
// SoBase-derived objects that have been allocated and not
// deallocated.
SbBool SoBase::PImpl::trackbaseobjects = FALSE;
void * SoBase::PImpl::allbaseobj_mutex = NULL;
SoBaseSet * SoBase::PImpl::allbaseobj = NULL; // maps from SoBase * to NULL

SbString * SoBase::PImpl::refwriteprefix = NULL;

SbBool SoBase::PImpl::tracerefs = FALSE;
uint32_t SoBase::PImpl::writecounter = 0;

// *************************************************************************

// Create a new SoNode-derived instance from the input stream.
SoNode *
SoBase::PImpl::readNode(SoInput * in)
{
  SbName name;
  if (!in->read(name, TRUE)) return NULL;
  SoBase * node = NULL;
  if (!SoBase::PImpl::readBase(in, name, node)) return NULL;
  assert(node->isOfType(SoNode::getClassTypeId()));
  return (SoNode *) node;
}

// Remove reference from a name to the instance pointer.
void
SoBase::PImpl::removeName2Obj(SoBase * const base, const char * const name)
{
  CC_MUTEX_LOCK(SoBase::PImpl::name2obj_mutex);
  SbHash<const char*, SbPList*>::const_iterator iter = SoBase::PImpl::name2obj->find(name);
  SbBool found = (iter != SoBase::PImpl::name2obj->const_end());
  assert(found);
  
  SbPList * l = iter->obj;

  const int i = l->find(base);
  assert(i >= 0);
  l->remove(i);

  CC_MUTEX_UNLOCK(SoBase::PImpl::name2obj_mutex);
}

// Remove a reference from an instance pointer to its associated name.
void
SoBase::PImpl::removeObj2Name(SoBase * const base, const char * const COIN_UNUSED_ARG(name))
{
  CC_MUTEX_LOCK(SoBase::PImpl::obj2name_mutex);
  SoBase::PImpl::obj2name->erase(base);
  CC_MUTEX_UNLOCK(SoBase::PImpl::obj2name_mutex);
}

void
SoBase::PImpl::cleanup_auditordict(void)
{
  if (SoBase::PImpl::auditordict) {
    for(
       SbHash<const SoBase *, SoAuditorList *>::const_iterator iter =
         SoBase::PImpl::auditordict->const_begin();
       iter!=SoBase::PImpl::auditordict->const_end();
       ++iter
       ) {
      delete iter->obj;
    }

    delete SoBase::PImpl::auditordict;
    SoBase::PImpl::auditordict = NULL;
  }
}

void
SoBase::PImpl::check_for_leaks(void)
{
#if COIN_DEBUG
  if (SoBase::PImpl::trackbaseobjects) {
    SbList<const SoBase *> keys;
    SoBase::PImpl::allbaseobj->makeKeyList(keys);
    const unsigned int len = keys.getLength();
    if (len > 0) {
      // Use printf()s, in case SoDebugError has been made defunct by
      // previous coin_atexit() work.
      (void)printf("\nSoBase-derived instances not deallocated:\n");

      for (unsigned int i=0; i < len; i++) {
        const SoBase * base = keys[i];
        base->assertAlive();
        const SbName name = base->getName();
        const SoType t = base->getTypeId();
        SbString s;
        s.sprintf("\"%s\"", name.getString());
        (void)printf("\t%p type==(0x%04x, '%s') name=='%s' refs==%d\n",
                     base, t.getKey(), t.getName().getString(),
                     name == "" ? "no name" : s.getString(),
                     base->getRefCount());
      }
      (void)printf("\n");
    }
  }
#endif // COIN_DEBUG
}

//
// Callback from cc_rbptree_traverse().
//
void
SoBase::PImpl::rbptree_notify_cb(void * auditor, void * type, void * closure)
{
  NotifyData * data = static_cast<NotifyData *>(closure);
  data->cnt--;

  // MSVC7 on 64-bit Windows wants to go through this type when
  // casting from void*.
  const uintptr_t tmptype = (uintptr_t)type;

  if (data->cnt == 0) {
    data->thisp->doNotify(data->list, auditor, (SoNotRec::Type) tmptype);
  }
  else {
    assert(data->cnt > 0);
    // use a copy of 'l', since the notification list might change
    // when auditors are notified
    SoNotList listcopy(data->list);
    data->thisp->doNotify(&listcopy, auditor, (SoNotRec::Type) tmptype);
  }
}

// Reads the name of a reference after a "USE" keyword and finds the
// ptr to the object which is being referenced.
SbBool
SoBase::PImpl::readReference(SoInput * in, SoBase *& base)
{
  SbName refname;
  if (!in->read(refname, FALSE)) {
    SoReadError::post(in, "Premature end of file after \"%s\"", USE_KEYWORD);
    return FALSE;
  }

  // This code to handles cases where USE ref name is
  // immediately followed by a "." and a fieldname, as can occur
  // when reading field-to-field connections.
  if (!in->isBinary()) {
    SbString refstr = refname.getString();

    // NOTE:
    // If the name ends with a }. E.g.
    //
    // USE mesh+0}
    //
    // then we are in trouble, but so is Open Inventor.
    // This is due to the ability for "}" to be a character
    // in the name of a node.
    const size_t index = strcspn(refstr.getString(), ".");
    SbString startstr = refstr.getSubString(0, (int)(index - 1));
    SbString endstr = refstr.getSubString((int)index);
    in->putBack(endstr.getString());

    refname = startstr;
  }

  if ((base = in->findReference(refname)) == NULL) {
    SoReadError::post(in, "Unknown reference \"%s\"", refname.getString());
    return FALSE;
  }

  // when referencing an SoProtoInstance, we need to return the proto
  // instance's root node, not the actual proto instance node.
  if (base->isOfType(SoProtoInstance::getClassTypeId())) {
    base = ((SoProtoInstance*) base)->getRootNode();
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::readReference",
                         "USE: '%s'", refname.getString());
#endif // debug

  return TRUE;
}

// Read the SoBase instance.
SbBool
SoBase::PImpl::readBase(SoInput * in, SbName & classname, SoBase *& base)
{
  assert(classname != "");

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoBase::readBase", "classname: '%s'",
                         classname.getString());
#endif // debug

  SbBool ret = TRUE;
  base = NULL;

  SbName refname;

  if (in->isFileVRML2()) {
    if (classname == PROTO_KEYWORD ||
        classname == EXTERNPROTO_KEYWORD) { // special case to handle [EXTERN]PROTO definitions
      SoProto * proto = new SoProto(classname == EXTERNPROTO_KEYWORD);
      proto->ref();
      ret = proto->readInstance(in, 0);
      if (ret) {
        proto->unrefNoDelete();
        in->addProto(proto);
      }
      else {
        proto->unref();
        return FALSE;
      }
      base = proto;
      return TRUE;
    }
  }

  if (classname == DEF_KEYWORD) {
    if (!in->read(refname, FALSE) || !in->read(classname, TRUE)) {
      if (in->eof()) {
        SoReadError::post(in, "Premature end of file after %s", DEF_KEYWORD);
      }
      else {
        SoReadError::post(in, "Unable to read identifier after %s keyword", DEF_KEYWORD);
      }
      ret = FALSE;
    }

    if (!refname) {
      SoReadError::post(in, "No name given after %s", DEF_KEYWORD);
      ret = FALSE;
    }

    if (!classname) {
      SoReadError::post(in, "Invalid definition of %s", refname.getString());
      ret = FALSE;
    }
  }

  if (ret) {
    SbBool gotchar = FALSE; // Unnecessary, but kills a compiler warning.
    char c;
    if (!in->isBinary() && (!(gotchar = in->read(c)) || c != OPEN_BRACE)) {
      if (gotchar)
        SoReadError::post(in, "Expected '%c'; got '%c'", OPEN_BRACE, c);
      else
        SoReadError::post(in, "Expected '%c'; got EOF", OPEN_BRACE);
      ret = FALSE;
    }
    else {
      ret = SoBase::PImpl::readBaseInstance(in, classname, refname, base);

      if (ret && !in->isBinary()) {
        if (!(gotchar = in->read(c)) || c != CLOSE_BRACE) {
          if (gotchar)
            SoReadError::post(in, "Expected '%c'; got '%c' for %s", CLOSE_BRACE, c, classname.getString());
          else
            SoReadError::post(in, "Expected '%c'; got EOF for %s", CLOSE_BRACE, classname.getString());
          ret = FALSE;
        }
      }
    }
  }

  return ret;
}

// Read the SoBase instance.
SbBool
SoBase::PImpl::readBaseInstance(SoInput * in, const SbName & classname,
                                const SbName & refname, SoBase *& base)
{
  assert(classname != "");

  SbBool needupgrade = FALSE;

  // first, try creating an upgradable node, based on the version of
  // the input file.
  base = SoUpgrader::tryCreateNode(classname, in->getIVVersion());
  if (base) {
    // we need to upgrade the node after reading it
    needupgrade = TRUE;
  }
  else {
    // create normal Coin node
    base = SoBase::PImpl::createInstance(in, classname);
  }

  if (!base) { goto failed; }

  if (!(!refname)) {
    // Set up new entry in reference hash -- with full name.
    in->addReference(refname, base);

    // Remove reference counter suffix, if any (i.e. "goldsphere+2"
    // becomes "goldsphere").
    SbString instancename = refname.getString();
    const char * strp = instancename.getString();
    const char * occ = strstr(strp, SoBase::PImpl::refwriteprefix->getString());

    if (occ != strp) { // They will be equal if the name is only a refcount.
      const ptrdiff_t offset = occ - strp;
      if (occ) instancename = instancename.getSubString(0, (int)offset - 1);
      // Set name identifier for newly created SoBase instance.
      base->setName(instancename);
    }
  }

  // The "flags" argument to readInstance is only checked during
  // import from binary format files.
  {
    unsigned short flags = 0;
    if (in->isBinary() && (in->getIVVersion() > 2.0f)) {
      const SbBool ok = in->read(flags);
      if (!ok) { goto failed; }
    }

    const SbBool ok = base->readInstance(in, flags);
    if (!ok) { goto failed; }
  }

  // Make sure global fields are unique
  if (base->isOfType(SoGlobalField::getClassTypeId())) {
    SoGlobalField * globalfield = (SoGlobalField *)base;

    // The global field is removed from the global field list
    // because we have to check if there is already a global field
    // in the list with the same name.  This is because
    // SoGlobalField's constructor automatically adds itself to the
    // list of global fields without checking if the field already
    // exists.
    globalfield->ref(); // increase refcount to 1, so the next call will not destruct the node
    SoGlobalField::removeGlobalFieldContainer(globalfield);
    globalfield->unrefNoDelete(); // corrects ref count back to zero

    // A read-error sanity check should have been done in
    // SoGlobalField::readInstance().
    assert(globalfield->getFieldData()->getNumFields() == 1);

    // Now, see if the global field is in the database already.
    SoField * f = SoDB::getGlobalField(globalfield->getName());
    if (f) {
      SoField * basefield = globalfield->getFieldData()->getField(globalfield, 0);
      assert(basefield && "base (SoGlobalField) does not appear to have a field");

      if (!f->isOfType(basefield->getClassTypeId())) {
        SoReadError::post(in, "Types of equally named global fields do not match: existing: %s, new: %s",
                          f->getTypeId().getName().getString(), basefield->getTypeId().getName().getString());
        goto failed;
      }

      SoGlobalField * container = (SoGlobalField *)f->getContainer();

      // Copy new field values into the existing field. Open Inventor
      // apparently does not copy the new values into the old field,
      // but it seems logical to do so.
      SoFieldContainer::initCopyDict();
      container->copyFieldValues(globalfield, TRUE); // Assign new global field values to old global field
      SoFieldContainer::copyDone();

      // Make sure to update the mapping in SoInput if necessary
      if (!(!refname)) {
        // Set up new entry in reference hash -- with full name.
        in->removeReference(refname);
        in->addReference(refname, container);
      }

      // Remove newly made SoGlobalField, use the existing one instead.
      // Add it to the global field list before deleting it (we
      // manually removed it earlier to test it the field was already
      // in the database)
      SoGlobalField::addGlobalFieldContainer((SoGlobalField*) base);
      base->ref(); base->unref(); // this will delete the global field, and remove it from the database
      base = container;
      container->getFieldData()->getField(container, 0)->touch();
    }
    else {
      // The global field was first removed to check the existence
      // of an equal named item. If no such global field exists, the
      // removed global field has to be added again, which is done
      // by this code:
      SoGlobalField::addGlobalFieldContainer(globalfield);
    }
  }

  if (needupgrade) {
    SoBase * oldbase = base;
    oldbase->ref();
    base = SoUpgrader::createUpgrade(oldbase);
    assert(base && "should never happen (since needupgrade == TRUE)");
    oldbase->unref();
  }

  if (base->isOfType(SoProtoInstance::getClassTypeId())) {
    base = ((SoProtoInstance*) base)->getRootNode();
  }

  return TRUE;

failed:
  if (base) {
    if (!(!refname)) { in->removeReference(refname); }

    base->ref();
    base->unref();
    base = NULL;
  }

  return FALSE;
}

// Create a new instance of the "classname" type.
SoBase *
SoBase::PImpl::createInstance(SoInput * in, const SbName & classname)
{
  assert(classname != "");

  SoType type = SoType::badType();
  if (in->isFileVRML2()) {
    SbString newname;
    newname.sprintf("VRML%s", classname.getString());
    type = SoType::fromName(SbName(newname.getString()));
#if COIN_DEBUG && 0 // debug
    if (type != SoType::badType()) {
      SoDebugError::postInfo("SoBase::createInstance",
                             "Created VRML V2.0 type: %s",
                             type.getName().getString());
    }
#endif // debug
  }

  // search for PROTO in current SoInput instance
  SoProto * proto = in->findProto(classname);
  if (!proto) {
    // search in global PROTO list
    proto = SoProto::findProto(classname);
  }
  if (proto) return proto->createProtoInstance();

  if (type == SoType::badType())
    type = SoType::fromName(classname);

  SoBase * instance = NULL;

  if (type == SoType::badType() ||
      type == SoUnknownNode::getClassTypeId()) {
    // Default to SoUnknownNode for now.. FIXME: what if we're dealing
    // with an unknown engine? 20000105 mortene.
    SoUnknownNode * unknownnode = new SoUnknownNode;
    unknownnode->setNodeClassName(classname);
    instance = unknownnode;
#if COIN_DEBUG && 0 // debug
    if (SoInputP::debug()) {
      SoDebugError::postInfo("SoBase::createInstance",
                             "created SoUnknownNode for '%s'",
                             classname.getString());
    }
#endif // debug
  }
  else if (!type.canCreateInstance()) {
    SoReadError::post(in, "Class \"%s\" is abstract", classname.getString());
  }
  else {
    instance = (SoBase *)type.createInstance();
  }

  return instance;
}

// Hmm.
void
SoBase::PImpl::flushInput(SoInput * in)
{
#if 0 // FIXME: obsoleted, see comment at the end of SoBase::readBase(). 20020531 mortene.
  assert(FALSE);
#else // obsoleted
  assert(!in->isBinary());

  int nestlevel = 1;
  char c;

  while (nestlevel > 0 && in->read(c)) {
    if (c == CLOSE_BRACE) nestlevel--;
    else if (c == OPEN_BRACE) nestlevel++;
  }
#endif // obsoleted
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

#include <cstring>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFTime.h>

// Tests whether or not our mechanisms with the realTime field works
// correctly upon references to it in imported iv-files.
//
// (Added this test when it was suspected that a new realTime
// globalfield was made upon import due to bug(s) in
// SoBase::PImpl::readBaseInstance()).
//
// -mortene

BOOST_AUTO_TEST_CASE(realTime_globalfield_import)
{
  // SoDB::init() already called by test-suite init, and the realTime
  // global field will be set up there

  //Store away the state before we mess with the global realTime field
  SoSFTime * realtime = (SoSFTime *)SoDB::getGlobalField("realTime");
  assert(realtime);
  SbTime realTimeStorage = realtime->getValue();

  char scene[] =
    "#Inventor V2.1 ascii\n\n"
    "RotationXYZ {"
    "   angle = GlobalField {"
    "      type \"SFTime\""
    "      realTime 0"
    "   }"
    "   . realTime "
    "}";

  SoInput * in = new SoInput;
  in->setBuffer(scene, strlen(scene));
  SoNode * g = NULL;
  const SbBool readok = SoDB::read(in, g);
  assert(readok); // that import is ok is tested by a case in SoDB.cpp
  delete in;

  // check that the global field is still the same instance
  SoSFTime * realtimeafter = (SoSFTime *)SoDB::getGlobalField("realTime");
  BOOST_CHECK_MESSAGE(realtime == realtimeafter,
                      "internal realTime SoGlobalField value changed upon iv import");

  // clean up
  g->ref();
  g->unref();

  //Restore state
  realtime->setValue(realTimeStorage);

}

#endif // COIN_TEST_SUITE

// *************************************************************************
