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

// This is an internal class for holding user extension nodes from .iv
// files. The header file for this node is not exported at ``make
// install'', and the class is not documented for application
// programmers.

// FIXME: copy operations doesn't work as expected on this node. I
// think both the copyContents() and addToCopyDict() methods needs to
// be overridden.  20000404 mortene.

#include "nodes/SoUnknownNode.h"

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/C/tidbits.h>

#include "io/SoWriterefCounter.h"
#include "io/SoInputP.h"
#include "tidbitsp.h"

// *************************************************************************

class SoUnknownNodeP {
public:
  SbName classname;
  SoChildList * privatechildren, * alternate;
};

// *************************************************************************

// The following code is used instead of SO_NODE_SOURCE() to let
// SoUnknownNodes have dynamic handling of SoFieldData objects.

PRIVATE_NODE_TYPESYSTEM_SOURCE(SoUnknownNode);

const SoFieldData * SoUnknownNode::getFieldData(void) const
{
  return this->classfielddata;
}

void *
SoUnknownNode::createInstance(void)
{
  return new SoUnknownNode;
}

#define PRIVATE(p) ((p)->pimpl)

// Node implementation starts "proper".

SoUnknownNode::SoUnknownNode(void)
{
  PRIVATE(this) = new SoUnknownNodeP;

  /* Catch attempts to use a node class which has not been initialized. */
  assert(SoUnknownNode::classTypeId != SoType::badType());
  /* Initialize a fielddata container for the instance. */
  this->classfielddata = new SoFieldData;

  this->isBuiltIn = FALSE;
  PRIVATE(this)->privatechildren = NULL;
  PRIVATE(this)->alternate = new SoChildList(this, 1);
}

SoUnknownNode::~SoUnknownNode()
{
  for (int i=0; i < this->classfielddata->getNumFields(); i++)
    delete this->classfielddata->getField(this, i);

  delete this->classfielddata;
  delete PRIVATE(this)->privatechildren;
  delete PRIVATE(this)->alternate;
  delete PRIVATE(this);
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoUnknownNode::initClass(void)
{
  /* Make sure we only initialize once. */
  assert(SoUnknownNode::classTypeId == SoType::badType());
  /* Make sure superclass gets initialized before subclass. */
  assert(inherited::getClassTypeId() != SoType::badType());

  /* Set up entry in the type system. */
  SoUnknownNode::classTypeId =
    SoType::createType(inherited::getClassTypeId(),
                       "UnknownNode",
                       &SoUnknownNode::createInstance,
                       SoNode::nextActionMethodIndex++);
  coin_atexit((coin_atexit_f*)SoUnknownNode::atexit_cleanup, CC_ATEXIT_NORMAL);
}

void
SoUnknownNode::atexit_cleanup(void)
{
  SoUnknownNode::classTypeId STATIC_SOTYPE_INIT;
}

SbBool
SoUnknownNode::readInstance(SoInput * in, unsigned short flags)
{
  if (SoInputP::debug()) {
    SoDebugError::postInfo("SoUnknownNode::readInstance",
                           "Reading extension node \"%s\" as SoUnknownNode.",
                           PRIVATE(this)->classname.getString());
  }


  SbBool notbuiltin;
  // The "error on unknown field" is FALSE, in case we are a group
  // node with children specified in the file.
  if (!this->classfielddata->read(in, this, FALSE, notbuiltin)) return FALSE;

  if (notbuiltin == FALSE) {
    SoReadError::post(in, "Node type \"%s\" not recognized.",
                      PRIVATE(this)->classname.getString());
    return FALSE;
  }

  // Set pointer to alternateRep node, if SoSFNode field with this
  // name is present.
  int i;
  for (i=0; i < this->classfielddata->getNumFields(); i++) {
    if (this->classfielddata->getFieldName(i) == "alternateRep") {
      SoSFNode * f = (SoSFNode *)this->classfielddata->getField(this, i);
      if (f->isOfType(SoSFNode::getClassTypeId())) {
#if COIN_DEBUG && 0 // debug
        SoDebugError::postInfo("SoUnknownNode::readInstance",
                               "found alternate representation");
#endif // debug
        if (!f->getValue()) break;
        PRIVATE(this)->alternate->truncate(0);
        PRIVATE(this)->alternate->append(f->getValue());
      }
      break;
    }
  }

  // Read children, if necessary.
  if (!in->isBinary() || (flags & SoBase::IS_GROUP)) {
    SoGroup * g = new SoGroup;
    g->ref();
    if (!g->readChildren(in)) {
      g->unref();
      return FALSE;
    }

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoUnknownNode::readInstance",
                           "read %d children successfully",
                           g->getNumChildren());
#endif // debug

    delete PRIVATE(this)->privatechildren;
    PRIVATE(this)->privatechildren = new SoChildList(this, * g->getChildren());
    g->unref();
  }

  // Check that all field descriptions did actually get a value
  // set. It is not a fatal error if this didn't happen, but: 1) on
  // subsequent write operations, information will be lost, and 2) the
  // field value will probably be completely random because we're
  // reading from uninitialized memory.
  if (SoInputP::debug()) {
    for (i=0; i < this->classfielddata->getNumFields(); i++) {
      const SoField * f = this->classfielddata->getField(this, i);
      if (f->isDefault()) {
        SoReadError::post(in, "Field \"%s\" in extension node \"%s\" not "
                          "given any value.",
                          this->classfielddata->getFieldName(i).getString(),
                          PRIVATE(this)->classname.getString());
      }
    }
  }

  return TRUE;
}

// The name used for the nodes of this type in scene file.
void
SoUnknownNode::setNodeClassName(const SbName & name)
{
  PRIVATE(this)->classname = name;
}

// Overridden from SoBase.
const char *
SoUnknownNode::getFileFormatName(void) const
{
  return PRIVATE(this)->classname.getString();
}

// Overridden from SoNode. SoChildList contains either 0 or 1
// elements, depending on if an alternate representation was
// specified.
SoChildList *
SoUnknownNode::getChildren(void) const
{
  return PRIVATE(this)->alternate;
}

// Write action method is overridden from SoNode to handle children.
void
SoUnknownNode::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    inherited::write(action);
    // Only increase number of writereferences to the top level node
    // in a tree which is used multiple times.
    if (!SoWriterefCounter::instance(out)->hasMultipleWriteRefs(this))
      if (PRIVATE(this)->privatechildren) PRIVATE(this)->privatechildren->traverse(action);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, PRIVATE(this)->privatechildren ? TRUE : FALSE, FALSE))
      return;
    this->getFieldData()->write(out, this);
    if (out->isBinary())
      if (PRIVATE(this)->privatechildren) out->write(PRIVATE(this)->privatechildren->getLength());
    if (PRIVATE(this)->privatechildren) PRIVATE(this)->privatechildren->traverse(action);
    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// Action methods overridden from SoNode to traverse alternateRep (and
// below, if alternateRep is a group node).

void
SoUnknownNode::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  PRIVATE(this)->alternate->traverse(action);
}

void
SoUnknownNode::GLRender(SoGLRenderAction * action)
{
  PRIVATE(this)->alternate->traverse(action);
}

void
SoUnknownNode::getBoundingBox(SoGetBoundingBoxAction * action)
{
  PRIVATE(this)->alternate->traverse(action);
}

void
SoUnknownNode::pick(SoPickAction * action)
{
  PRIVATE(this)->alternate->traverse(action);
}

SoNode * 
SoUnknownNode::addToCopyDict(void) const
{
  // This function is copied from SoNode::addToCopyDict() and
  // modified to handle the private children.

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoUnknownNode::addToCopyDict",
                         "Name: %s", PRIVATE(this)->classname.getString());
#endif // debug
  
  SoNode * cp = (SoNode *)SoFieldContainer::checkCopy(this);
  if (!cp) {
    cp = new SoUnknownNode;
    SoFieldContainer::addCopy(this, cp);

    if (PRIVATE(this)->privatechildren) {
      for (int i = 0; i < PRIVATE(this)->privatechildren->getLength(); i++) {
        (void) (*(PRIVATE(this)->privatechildren))[i]->addToCopyDict();
      }
    }
  }
  
  return cp;
}

void 
SoUnknownNode::copyContents(const SoFieldContainer * from,
                            SbBool copyconnections)
{
  int i;
  const SoUnknownNode * src = (const SoUnknownNode *) from;
  this->setNodeClassName(PRIVATE(src)->classname);
  
  const SoFieldData * srcdata = src->getFieldData();
  SoFieldData * dstdata = (SoFieldData*) this->getFieldData();
  
  for (i = 0; i < srcdata->getNumFields(); i++) {
    const SoField * srcfield = srcdata->getField(src, i);
    SoField * dstfield = (SoField*) srcfield->getTypeId().createInstance();
    
    // add new field to field data
    dstfield->setContainer(this);
    dstdata->addField(this, srcdata->getFieldName(i), dstfield);
    // copy field contents
    dstfield->copyFrom(*srcfield);
    // copy flags
    dstfield->setIgnored(srcfield->isIgnored());
    dstfield->setDefault(srcfield->isDefault());
    dstfield->enableNotify(srcfield->isNotifyEnabled());
    // fix complex fields (node, engine, and path fields)
    dstfield->fixCopy(copyconnections);
    // handle connections
    if (copyconnections && srcfield->isConnected()) {
      dstfield->copyConnection(srcfield);
    }
  }
  if (PRIVATE(src)->privatechildren == NULL) return;

  if (PRIVATE(this)->privatechildren == NULL) {
    PRIVATE(this)->privatechildren = new SoChildList(this);
  }

  // copy private children, which we added to the copy dict in
  // addToCopyDict()
  for (i = 0; i < PRIVATE(src)->privatechildren->getLength(); i++) {
    SoNode * copy = (SoNode*) 
      this->findCopy((*(PRIVATE(src)->privatechildren))[i],
                     copyconnections);
    assert(copy && "unexpected error");
    PRIVATE(this)->privatechildren->append(copy);
  }
}

#undef PRIVATE
