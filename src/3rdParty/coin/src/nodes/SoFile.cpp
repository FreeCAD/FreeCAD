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
  \class SoFile SoFile.h Inventor/nodes/SoFile.h
  \brief The SoFile class is node container for another model file.

  \ingroup coin_nodes

  This node provides a way to split your models into a set of
  "component" models to include into larger "master" files.

  Just provide the name of the model file to include in the
  SoFile::name field, and it will automatically be loaded and have its
  nodes inserted into the scene graph at the point of the SoFile node.

  You can also set the SoFile::name field manually. Such an action
  will then automatically trigger an invocation of a read operation
  which imports the filename you set in the field.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    File {
        name "<Undefined file>"
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoFile.h>
#include "coindefs.h"

#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFString SoFile::name

  Filename for model file to insert in the scene graph at the location
  of the SoFile node.
*/

// *************************************************************************

class SoFileP {
public:
  static const char UNDEFINED_FILE[];
  static SbBool searchok;
};

const char SoFileP::UNDEFINED_FILE[] = "<Undefined file>";
SbBool SoFileP::searchok = FALSE;

// *************************************************************************

SO_NODE_SOURCE(SoFile);

// *************************************************************************

/*!
  Constructor.
*/
SoFile::SoFile(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoFile);

  SO_NODE_ADD_FIELD(name, (SoFileP::UNDEFINED_FILE));

  this->namesensor = new SoFieldSensor(SoFile::nameFieldModified, this);
  this->namesensor->setPriority(0); // immediate sensor
  this->namesensor->attach(& this->name);

  this->children = new SoChildList(this);
}

/*!
  Destructor.
*/
SoFile::~SoFile()
{
  delete this->namesensor;
  delete this->children;
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoFile::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFile, SO_FROM_INVENTOR_1);
}

// *************************************************************************

/*!  
  Returns the read filename, possibly including the (relative) path
  where the file was found.  Returns an empty string if no file has
  been read.

  This method is an extension versus the Open Inventor API.  
*/
const SbString & 
SoFile::getFullName(void) const
{
  return this->fullname;
}

// Doc from superclass.
void
SoFile::getBoundingBox(SoGetBoundingBoxAction * action)
{
  int numindices;
  const int * indices;
  int lastchildindex;

  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
    lastchildindex = indices[numindices-1];
  else
    lastchildindex = this->getChildren()->getLength() - 1;
  
  assert(lastchildindex < this->getChildren()->getLength());

  // Initialize accumulation variables.
  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  int numcenters = 0;

  for (int i = 0; i <= lastchildindex; i++) {
    this->getChildren()->traverse(action, i);
    
    // If center point is set, accumulate.
    if (action->isCenterSet()) {
      acccenter += action->getCenter();
      numcenters++;
      action->resetCenter();
    }
  }
  
  if (numcenters != 0)
    action->setCenter(acccenter / float(numcenters), FALSE);
}

// Doc from superclass.
void
SoFile::GLRender(SoGLRenderAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
SbBool
SoFile::readInstance(SoInput * in, unsigned short flags)
{
  // We detach the sensor and later call readNamedFile() explicitly
  // instead of letting readNamedFile() be called implicitly due to
  // the SoFieldSensor on this->name.
  //
  // This is done so the same SoInput instance is used for
  // readNamedFile() as for this method, and then also the same name
  // dictionary -- which is necessary for "cross-file" references to
  // work.

  this->fullname.makeEmpty();
  this->namesensor->detach();
  SbBool result = inherited::readInstance(in, flags);
  this->namesensor->attach(& this->name);
  return result && this->readNamedFile(in);
}

/*!
  Read the file named in the SoFile::name field.

  Used to be a private method, moved into protected space to enable
  subclasses to detect when the file is (re)read.

  \since Coin 2.0
*/
SbBool
SoFile::readNamedFile(SoInput * in)
{
  if (this->name.getValue().getLength() == 0 ||
      strcmp(this->name.getValue().getString(), SoFileP::UNDEFINED_FILE) == 0) {
    // We handle this different than Inventor, where the whole read
    // process fails upon an unspecified filename.
    SoDebugError::postWarning("SoFile::readNamedFile", 
                              "Undefined filename in SoFile.");
    return TRUE;
  }

  // If we can't find file, ignore it. Note that this does not match
  // the way Inventor works, which will make the whole read process
  // exit with a failure code.
  if (!in->pushFile(this->name.getValue().getString())) return TRUE;

  this->fullname = in->getCurFileName();

  static int debugreading = -1;
  if (debugreading == -1) {
    const char * env = coin_getenv("COIN_DEBUG_SOFILE_READ");
    debugreading = env && (atoi(env) > 0);
  }

  if (debugreading) {
    SoDebugError::postInfo("SoFile::readNamedFile", "(full) name=='%s'",
                           this->fullname.getString());
  }


  SoChildList cl(this);
  SbBool readok = TRUE;
  do {
    SoNode * n;
    readok = SoDB::read(in, n); // Not using SoDB::readAll because it
                                // may add an extra SoSeparator node if
                                // more than one root-child is present
                                // in the file

    if (!readok) { // actual read error
      break;
    }
    if (n == NULL) { // this is what happens on EOF, not valid
                     // identifiers, or when children have been given
                     // the NULL keyword

      break; 
    }
    cl.append(n);
  } while (!in->eof());
  
  // The file should not be removed from the stack before it is done
  // deliberately at the end of this method.
  assert(in->getCurFileName() == this->fullname);

  if (readok) {
    this->children->copy(cl); // (copy() implicitly truncates before copying)

    if (!in->eof()) {
      // All  characters  may not  have  been  read  from the  current
      // stream.  The reading stops when no more valid identifiers are
      // found, so we have to read until the current file on the stack
      // is at the end.  All non-whitespace characters from now on are
      // erroneous.
      static uint32_t fileerrors_termination = 0;
      SbString dummy;
      while (!in->eof() && in->read(dummy)) {
        if (fileerrors_termination < 1) {
          SoReadError::post(in, "Erroneous character(s) after end of scene graph: \"%s\". "
                            "This message will only be shown once for this file, "
                            "but more errors might be present", dummy.getString());
        }
        fileerrors_termination++;
      }
      assert(in->eof());
    }
  }
  else {
    if (!in->eof()) {
      // Places the stream at the end of the current file on the
      // stack. This is a bit "hack-ish", but its done this way
      // instead of loosening the protection of SoInput::popFile().
      char dummy;
      while (!in->eof() && in->get(dummy));
      assert(in->eof());
    }
      
    // Note that we handle this differently than Inventor, which lets
    // the whole import fail.
    SoReadError::post(in, "Unable to read subfile: \"%s\"",
                      this->name.getValue().getString());
  }

  // Make really sure the stack is popped and that operations like
  // isBinary that may be called before the next read operation is
  // encountered is working on the correct file.  This is kind of
  // hack'ish, but is done because of the protection of
  // SoInput::popFile
  char dummy;
  SbBool gotchar = in->get(dummy);
  if (gotchar) in->putBack(dummy);

  return TRUE;
}

// Callback for the field sensor.
void
SoFile::nameFieldModified(void * userdata, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoFile * that = (SoFile *)userdata;
  SoInput in;
  that->fullname.makeEmpty();
  (void)that->readNamedFile(&in);
}

/*!
  Returns a subgraph with a deep copy of the children of this node.
*/
SoGroup *
SoFile::copyChildren(void) const
{
  SoGroup * tmproot = new SoGroup;
  tmproot->ref();

  // Instead of individually copying our children one by one and
  // attaching to the new group node root, we use a temporary group
  // node to first *attach* our children to, and then copying the
  // root. This is done so any interconnections between sub-graphs are
  // also copied.

  const SoChildList * cl = this->children;
  for (int i = 0; i < cl->getLength(); i++) {
    tmproot->addChild(cl->operator[](i));
  }

  SoNode * n = tmproot->copy(TRUE);

  tmproot->unref();

  return (SoGroup *)n;
}

// Doc from superclass.
SoChildList *
SoFile::getChildren(void) const
{
  return this->children;
}

// Doc from superclass.
void
SoFile::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
  else {
    this->children->traverse((SoAction *)action);
  }
}

// Doc from superclass.
void
SoFile::callback(SoCallbackAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::getMatrix(SoGetMatrixAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::handleEvent(SoHandleEventAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::pick(SoPickAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::audioRender(SoAudioRenderAction * action)
{
  SoFile::doAction((SoAction *)action);
}

void
SoFile::search(SoSearchAction * action)
{
  SoNode::search(action); // always include this node in the search

  // only search children if the user has requested it
  if (SoFileP::searchok) SoFile::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFile::copyContents(const SoFieldContainer * from, SbBool copyconnections)
{
  // so we don't reload when resetting the name field
  this->namesensor->detach();
  inherited::copyContents(from, copyconnections);
  this->namesensor->attach(&this->name);

  SoFile * filenode = (SoFile *)from;

  this->children->truncate(0);
  for (int i=0; i < filenode->children->getLength(); i++) {
    SoNode * cp = (SoNode *)
      SoFieldContainer::findCopy((*(filenode->children))[i], copyconnections);
    this->children->append(cp);
  }
}

/*!
  Enables/disables searching children using SoSearchAction.
*/
void
SoFile::setSearchOK(SbBool dosearch)
{
  SoFileP::searchok = dosearch;
}

/*!
  Returns whether searching children using SoSearchAction is enabled.
*/
SbBool
SoFile::getSearchOK()
{
  return SoFileP::searchok;
}
