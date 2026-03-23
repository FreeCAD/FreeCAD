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
  \class SoProtoInstance SoProtoInstance.h Inventor/misc/SoProtoInstance.h
  \brief The SoProtoInstance class handles PROTO instances.

  \sa SoProto
*/

// *************************************************************************

#include <Inventor/misc/SoProtoInstance.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/sensors/SoNodeSensor.h>

#include "tidbitsp.h"
#include "misc/SbHash.h"
#include "threads/threadsutilp.h"

// *************************************************************************

class SoProtoInstanceP {
public:
  SoProtoInstanceP() :
    fielddata(NULL),
    protodef(NULL),
    root(NULL)
  { }

  SoFieldData * fielddata;
  SoProto * protodef;
  SoNode * root;
};

// *************************************************************************

// The following code is used instead of SO_NODE_SOURCE() to let
// SoUnknownNodes have dynamic handling of SoFieldData objects.

PRIVATE_NODE_TYPESYSTEM_SOURCE(SoProtoInstance);

// *************************************************************************

typedef SbHash<const SoNode *, SoProtoInstance *> SoNode2SoProtoInstanceMap;

static SoNode2SoProtoInstanceMap * protoinstance_dict;
static void * protoinstance_mutex;

// *************************************************************************

// doc in parent
void
SoProtoInstance::initClass(void)
{
 /* Make sure we only initialize once. */
  assert(SoProtoInstance::classTypeId == SoType::badType());
  /* Make sure superclass gets initialized before subclass. */
  assert(inherited::getClassTypeId() != SoType::badType());

  /* Set up entry in the type system. */
  SoProtoInstance::classTypeId =
    SoType::createType(inherited::getClassTypeId(),
                       "ProtoInstance",
                       NULL,
                       SoNode::nextActionMethodIndex++);

  protoinstance_dict = new SoNode2SoProtoInstanceMap;
  CC_MUTEX_CONSTRUCT(protoinstance_mutex);
  coin_atexit((coin_atexit_f*) SoProtoInstance::cleanupClass, CC_ATEXIT_NORMAL);
}

/*!
  This static method cleans up static data of the SoProtoInstance class.
*/
void
SoProtoInstance::cleanupClass(void)
{
  delete protoinstance_dict;
  CC_MUTEX_DESTRUCT(protoinstance_mutex);
  SoProtoInstance::classTypeId STATIC_SOTYPE_INIT;
}

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor.
*/
SoProtoInstance::SoProtoInstance(SoProto * proto,
                                 const SoFieldData * deffielddata)
{
  PRIVATE(this) = new SoProtoInstanceP;
  PRIVATE(this)->fielddata = new SoFieldData;
  PRIVATE(this)->protodef = proto;
  if (proto) proto->ref();
  this->copyFieldData(deffielddata);
}

/*!
  Destructor.
*/
SoProtoInstance::~SoProtoInstance()
{
  this->setRootNode(NULL);
  const int n = PRIVATE(this)->fielddata->getNumFields();
  for (int i = 0; i < n; i++) {
    delete PRIVATE(this)->fielddata->getField(this, i);
  }
  delete PRIVATE(this)->fielddata;
  if (PRIVATE(this)->protodef) PRIVATE(this)->protodef->unref();
  delete PRIVATE(this);
  PRIVATE(this) = 0;
}

// doc in parent
const SoFieldData *
SoProtoInstance::getFieldData(void) const
{
  return PRIVATE(this)->fielddata;
}

/*!
  Returns the PROTO definition for this instance.
*/
SoProto *
SoProtoInstance::getProtoDefinition(void) const
{
  return PRIVATE(this)->protodef;
}

/*!
  Returns the PROTO definition name.
*/
SbName
SoProtoInstance::getProtoName(void) const
{
  if (PRIVATE(this)->protodef) return PRIVATE(this)->protodef->getProtoName();
  return SbName::empty();
}

// Doc in parent
SbBool
SoProtoInstance::readInstance(SoInput * in, unsigned short flags)
{
  return inherited::readInstance(in, flags);
  //  return FALSE;
}

/*!
  Sets the root node for this instance.
*/
void
SoProtoInstance::setRootNode(SoNode * root)
{
  CC_MUTEX_LOCK(protoinstance_mutex);
  if (PRIVATE(this)->root) {
    protoinstance_dict->erase(PRIVATE(this)->root);
  }
  PRIVATE(this)->root = root;
  if (root) {
    protoinstance_dict->put(root, this);
  }
  CC_MUTEX_UNLOCK(protoinstance_mutex);
}

/*!
  Returns the instance root node.
*/
SoNode *
SoProtoInstance::getRootNode(void)
{
  return PRIVATE(this)->root;
}

// Doc in parent
void
SoProtoInstance::write(SoWriteAction * action)
{
#if 0 // just testing, disabled pederb, 2002-06-18
  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out, FALSE);
  }
  else if (out->getStage() == SoOutput::WRITE) {
  }
  else assert(0 && "unknown stage");
#else
  inherited::write(action);
#endif
}

// Doc in parent
const char *
SoProtoInstance::getFileFormatName(void) const
{
  return PRIVATE(this)->protodef->getProtoName().getString();
}

/*!
  Given root node \a rootnode, return the PROTO instance, or NULL if
  \a rootnode is not a PROTO instance root node.
*/
SoProtoInstance *
SoProtoInstance::findProtoInstance(const SoNode * rootnode)
{
  SoProtoInstance * ret;
  CC_MUTEX_LOCK(protoinstance_mutex);
  if (!protoinstance_dict->get(rootnode, ret)) { ret = NULL; }
  CC_MUTEX_UNLOCK(protoinstance_mutex);
  return ret;
}

// Doc in parent
void
SoProtoInstance::copyFieldData(const SoFieldData * src)
{
  const int n = src->getNumFields();
  SoFieldContainer::initCopyDict();
  for (int i = 0; i < n; i++) {
    SoField * f = src->getField(PRIVATE(this)->protodef, i);
    SoField * cp = (SoField*) f->getTypeId().createInstance();
    cp->setContainer(this);
    PRIVATE(this)->fielddata->addField(this, src->getFieldName(i), cp);
    if (f->getFieldType() == SoField::NORMAL_FIELD ||
        f->getFieldType() == SoField::EXPOSED_FIELD) {
      cp->copyFrom(*f);
      cp->fixCopy(TRUE);
    }
    cp->setFieldType(f->getFieldType());
    cp->setDefault(f->isDefault());
  }
  SoFieldContainer::copyDone();
}

//
// Used to detect when the PROTO instance root node is destructed
//
void
SoProtoInstance::sensorCB(void * data, SoSensor *)
{
  // not used anymore. ProtoInstance is unref'ed from SoNode destructor
  SoProtoInstance * thisp = (SoProtoInstance*) data;
  thisp->setRootNode(NULL);
  thisp->unref();
}

#undef PRIVATE
