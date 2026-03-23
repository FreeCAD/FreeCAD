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

#include "io/SoWriterefCounter.h"

#include <cassert>

#include <Inventor/C/tidbits.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoBase.h>
#include <Inventor/nodes/SoNode.h>

#include "tidbitsp.h"
#include "threads/threadsutilp.h"
#include "misc/SbHash.h"

// *************************************************************************

class SoWriterefCounterBaseData {
public:
  SoWriterefCounterBaseData() {
    writeref = 0;
    ingraph = FALSE;
  }

  int32_t writeref;
  SbBool ingraph;
};

// *************************************************************************

typedef SbHash<const SoBase *, SoWriterefCounterBaseData *> SoBase2SoWriterefCounterBaseDataMap;

class SoWriterefCounterOutputData {
public:
  SoBase2SoWriterefCounterBaseDataMap writerefdict;

  SoWriterefCounterOutputData()
    : writerefdict(1051), refcount(0) {
  }

  // need refcounter since dict can be shared among several SoOutputs
  void ref(void) {
    this->refcount++;
  }
  void unref(void) {
    if (--this->refcount == 0) {
      this->cleanup();
      delete this;
    }
  }
  void debugCleanup(void) {
#if COIN_DEBUG
    for(
       SoBase2SoWriterefCounterBaseDataMap::const_iterator iter =
         writerefdict.const_begin();
       iter!=writerefdict.const_end();
       ++iter
       ) {
      const SoBase * base = iter->key;

      SbName name = base->getName();
      if (name == "") name = "<noname>";

      SoDebugError::postWarning("SoWriterefCounter::<cleanup>",
                                "Not removed from writerefdict: %p, %s:%s",
                                base, base->getTypeId().getName().getString(), name.getString());

    }
#endif // COIN_DEBUG
    this->cleanup();
  }

protected:
  ~SoWriterefCounterOutputData() {
  }

private:
  int refcount;

  void cleanup(void) {
    for(
       SoBase2SoWriterefCounterBaseDataMap::const_iterator iter =
         writerefdict.const_begin();
       iter!=writerefdict.const_end();
       ++iter
       ) {
      delete iter->obj;
    }

    this->writerefdict.clear();
  }

};

// *************************************************************************

typedef SbHash<SoOutput *, SoWriterefCounter *> SoOutput2SoWriterefCounterMap;
typedef SbHash<const SoBase *, int> SoBase2Id;

class SoWriterefCounterP {
public:
  SoWriterefCounterP(SoWriterefCounter * master, SoOutput * out, SoWriterefCounterP * dataCopy)
    : master(master), out(out)
  {
    if (dataCopy) {
      this->outputdata = dataCopy->outputdata;
      this->sobase2id = new SoBase2Id(*dataCopy->sobase2id);
    }
    else {
      this->outputdata = new SoWriterefCounterOutputData;
      this->sobase2id = new SoBase2Id;
    }
    this->outputdata->ref();
    this->nextreferenceid = 0;
  }
  ~SoWriterefCounterP() {
    this->outputdata->unref();
    delete this->sobase2id;
  }
  void appendPostfix(const SoBase *base, SbString &name, int refid)
  {
    // Fix to avoid writing DEFs starting with an illegal
    // character (e.g. '+') 
    if (name.getLength() == 0 &&
        base->isOfType(SoNode::getClassTypeId()) &&
        !SbName::isBaseNameStartChar((*refwriteprefix)[0])) {
      name += "_";
    }

    name += SoWriterefCounterP::refwriteprefix->getString();
    name.addIntString(refid);
  }

  SoWriterefCounter * master;
  SoOutput * out;
  SoWriterefCounterOutputData * outputdata;
  SoBase2Id * sobase2id;
  int nextreferenceid;

  static void * mutex;
  static SoOutput2SoWriterefCounterMap * outputdict;
  static SoWriterefCounter * current; // used to be backwards compatible
  static SbString * refwriteprefix;

  static void atexit_cleanup(void) {
    current = NULL;
    delete refwriteprefix;
    refwriteprefix = NULL;
    delete outputdict;
    outputdict = NULL;
    CC_MUTEX_DESTRUCT(mutex);
  }

};

void * SoWriterefCounterP::mutex;
SoOutput2SoWriterefCounterMap *  SoWriterefCounterP::outputdict;
SoWriterefCounter *  SoWriterefCounterP::current = NULL; // used to be backwards compatible
SbString *  SoWriterefCounterP::refwriteprefix;

#define PRIVATE(obj) obj->pimpl

// *************************************************************************

SoWriterefCounter::SoWriterefCounter(SoOutput * out, SoOutput * copyfrom)
{
  SoWriterefCounterP * datafrom = NULL;
  if (copyfrom) {
    SoWriterefCounter * frominst = SoWriterefCounter::instance(copyfrom);
    datafrom = frominst->pimpl;
  }
  PRIVATE(this) = new SoWriterefCounterP(this, out, datafrom);
}

SoWriterefCounter::~SoWriterefCounter()
{
  delete PRIVATE(this);
}

void
SoWriterefCounter::create(SoOutput * out, SoOutput * copyfrom)
{
  SoWriterefCounter * inst = new SoWriterefCounter(out, copyfrom);
  CC_MUTEX_LOCK(SoWriterefCounterP::mutex);
  SbBool ret = SoWriterefCounterP::outputdict->put(out, inst);
  assert(ret && "writeref instance already exists!");
  CC_MUTEX_UNLOCK(SoWriterefCounterP::mutex);
}

void
SoWriterefCounter::destruct(SoOutput * out)
{
  SoWriterefCounter * inst = SoWriterefCounter::instance(out);
  assert(inst && "instance not found!");

  CC_MUTEX_LOCK(SoWriterefCounterP::mutex);
  (void) SoWriterefCounterP::outputdict->erase(out);
  delete inst;
  CC_MUTEX_UNLOCK(SoWriterefCounterP::mutex);
}

void
SoWriterefCounter::initClass(void)
{
  CC_MUTEX_CONSTRUCT(SoWriterefCounterP::mutex);
  SoWriterefCounterP::outputdict = new SoOutput2SoWriterefCounterMap;
  SoWriterefCounterP::refwriteprefix = new SbString("+");
  coin_atexit((coin_atexit_f*) SoWriterefCounterP::atexit_cleanup, CC_ATEXIT_NORMAL);
}

void
SoWriterefCounter::debugCleanup(void)
{
  PRIVATE(this)->sobase2id->clear();
  PRIVATE(this)->outputdata->debugCleanup();
}

void
SoWriterefCounter::setInstancePrefix(const SbString & s)
{
  (*SoWriterefCounterP::refwriteprefix) = s;
}

SoWriterefCounter *
SoWriterefCounter::instance(SoOutput * out)
{
  if (out == NULL) {
    // to be backwards compatible with old code
    return SoWriterefCounterP::current;
  }

  CC_MUTEX_LOCK(SoWriterefCounterP::mutex);

  SoWriterefCounter * inst = NULL;

  const SbBool ok = SoWriterefCounterP::outputdict->get(out, inst);
  assert(ok && "no instance");

  SoWriterefCounterP::current = inst;
  CC_MUTEX_UNLOCK(SoWriterefCounterP::mutex);
  return inst;
}

SbBool
SoWriterefCounter::shouldWrite(const SoBase * base) const
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    return data->ingraph;
  }
  return FALSE;
}

SbBool
SoWriterefCounter::hasMultipleWriteRefs(const SoBase * base) const
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    return data->writeref > 1;
  }
  return FALSE;
}

int
SoWriterefCounter::getWriteref(const SoBase * base) const
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    return data->writeref;
  }
  return 0;
}

void
SoWriterefCounter::setWriteref(const SoBase * base, const int ref)
{
  // for debugging
  //  SbName name = base->getName();
  //  fprintf(stderr,"writeref: %s, %d, ingraph: %d\n", name.getString(), ref,
  //          isInGraph(base));
  //   }

  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    data->writeref = ref;
  }
  else {
    data = new SoWriterefCounterBaseData;
    data->writeref = ref;
    (void) PRIVATE(this)->outputdata->writerefdict.put(base, data);
  }


  if (ref == 0) {
    SoOutput * out = PRIVATE(this)->out;
    // for better debugging
    this->removeWriteref(base);
    // Ouch. Does this to avoid having two subsequent write actions on
    // the same SoOutput to write "USE ..." when it should write a
    // full node/subgraph specification on the second run.  -mortene.
    //
    // FIXME: accessing out->removeSoBase2IdRef() directly takes a
    // "friend SoBase" in the SoOutput class definition. Should fix
    // with proper design for next major Coin release. 20020426 mortene.
    if (out->findReference(base) != FIRSTWRITE) {
      out->removeSoBase2IdRef(base);
    }
  }
  if (ref < 0) {
    SbName name = base->getName();
    if (name == "") name = "<noname>";
    SoDebugError::postWarning("SoWriterefCounter::setWriteref",
                              "writeref < 0 for %s <%p>", name.getString(), base);
  }
}

void
SoWriterefCounter::decrementWriteref(const SoBase * base)
{
  this->setWriteref(base, this->getWriteref(base) - 1);
}


SbBool
SoWriterefCounter::isInGraph(const SoBase * base) const
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    return data->ingraph;
  }
  return FALSE;
}

void
SoWriterefCounter::setInGraph(const SoBase * base, const SbBool ingraph)
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    data->ingraph = ingraph;
  }
  else {
    data = new SoWriterefCounterBaseData;
    data->ingraph = ingraph;
    (void)PRIVATE(this)->outputdata->writerefdict.put(base, data);
  }
}

void
SoWriterefCounter::removeWriteref(const SoBase * base)
{
  SoWriterefCounterBaseData * data;
  if (PRIVATE(this)->outputdata->writerefdict.get(base, data)) {
    delete data;
    (void) PRIVATE(this)->outputdata->writerefdict.erase(base);
  }
  else {
    assert(0 && "writedata not found");
  }
}

//
// If this environment variable is set to 1, we try to preserve
// the original node names as far as possible instead of appending
// a "+<refid>" suffix.
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

SbName
SoWriterefCounter::getWriteName(const SoBase * base) const
{
  SoOutput * out = PRIVATE(this)->out;
  SbName name = base->getName();
  int refid = out->findReference(base);
  SbBool firstwrite = (refid == (int) FIRSTWRITE);
  SbBool multiref = this->hasMultipleWriteRefs(base);

  // Find what node name to write
  SbString writename;
  


 


    if (!firstwrite) {
      writename = name.getString();
      // We have used a suffix when DEF'ing the node
      if (refid != (int) NOSUFFIX) {
        PRIVATE(this)->appendPostfix(base, writename, refid);
      }
      // Detects last USE of a node, enables reuse of DEF's
	 // if dont_mangle_output_names(base) is true;
    // Try to keep the original node names as far as possible.
    // Weaknesses (FIXME kintel 20020429):
    //  o We should try to reuse refid's as well.
    //  o We should try to let "important" (=toplevel?) nodes
    //    keep their original node names before some subnode "captures" it.
    //
      if (!multiref && dont_mangle_output_names(base)) out->removeDEFNode(SbName(writename));
    }
    else {
      SbBool found = out->lookupDEFNode(name);
      writename = name.getString();
      if (!found && (!multiref || name.getLength() > 0)) {
        // We can use the node without a suffix
        if (multiref) out->addDEFNode(name);
        out->setReference(base, (int) NOSUFFIX);
      }
      if ((found && name.getLength()>0) || (multiref && name.getLength()==0)) {
        // Node name is already DEF'ed or an unnamed multiref => use a suffix.
        PRIVATE(this)->appendPostfix(base, writename, out->addReference(base));
        out->addDEFNode(SbName(writename));
      }
  }
 
  return writename;
}

/*!
  Makes a unique id for \a base and adds a mapping into our dictionary.
*/
int
SoWriterefCounter::addReference(const SoBase * base)
{
  if (!PRIVATE(this)->sobase2id) PRIVATE(this)->sobase2id = new SoBase2Id;
  const int id = PRIVATE(this)->nextreferenceid++;
  PRIVATE(this)->sobase2id->put(base, id);
  return id;
}

/*!
  Returns the unique identifier for \a base or -1 if not found.
*/
int
SoWriterefCounter::findReference(const SoBase * base) const
{
  int id;
  const SbBool ok =
    PRIVATE(this)->sobase2id &&
    PRIVATE(this)->sobase2id->get(base, id);
  return ok ? id : -1;
}

/*!
  Sets the reference for \a base manually.
*/
void
SoWriterefCounter::setReference(const SoBase * base, int refid)
{
  if (!PRIVATE(this)->sobase2id) PRIVATE(this)->sobase2id = new SoBase2Id;
  PRIVATE(this)->sobase2id->put(base, refid);
}

void
SoWriterefCounter::removeSoBase2IdRef(const SoBase * base)
{
  PRIVATE(this)->sobase2id->erase(base);
}

/*!
  Returns TRUE if the user wants extra debugging information regarding
  writerefs (COIN_DEBUG_WRITEREFS=1)
*/
SbBool
SoWriterefCounter::debugWriterefs(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_DEBUG_WRITEREFS");
    dbg = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return dbg;
}

/**********************************************************************/


#undef PRIVATE

