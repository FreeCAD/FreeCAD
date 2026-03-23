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
  \class SoProto SoProto.h Inventor/misc/SoProto.h
  \brief The SoProto class handles PROTO definitions.

  SoProto and SoProtoInstance are mostly internal classes. They're
  designed to read and handle VRML97 PROTOs. However, it's possible to
  define your PROTOs in C++. You must define your PROTO in a char
  array, and read that char array using SoInput::setBuffer() and
  SoDB::readAllVRML(). Example:

  \code

  char myproto[] =
  "#VRML V2.0 utf8\n"
  "PROTO ColorCube [\n"
  "  field SFColor color 1 1 1\n"
  "  field SFVec3f size 1 1 1\n"
  "]\n"
  "{\n"
  "  Shape {\n"
  "    appearance Appearance {\n"
  "      material Material {\n"
  "        diffuseColor IS color\n"
  "      }\n"
  "    }\n"
  "    geometry Box { size IS size }\n"
  "  }\n"
  "}\n"
  "ColorCube { color 1 0 0 size 2 1 1 }\n";

  SoInput in;
  in.setBuffer(myproto, strlen(myproto));
  SoVRMLGroup * protoroot = SoDB::readAllVRML(&in);

  \endcode

  Now you can create new instances of the ColorCube PROTO using
  SoProto::findProto() and SoProto::createProtoInstance(). If you want
  to insert PROTO instances into your scene graph, you should insert
  the node returned from SoProtoInstance::getRootNode().

  See
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.8
  for more information about PROTOs in VRML97.

*/

// *************************************************************************

#include <Inventor/misc/SoProto.h>

#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/SbName.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoFieldData.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SoNodeList.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>

#include "threads/threadsutilp.h"
#include "io/SoWriterefCounter.h"
#include "misc/SbHash.h"
#include "tidbitsp.h"

// *************************************************************************

static SoType soproto_type;

static SbList <SoProto*> * protolist;
static SoFetchExternProtoCB * soproto_fetchextern_cb = NULL;
static void * soproto_fetchextern_closure = NULL;
static void * soproto_mutex;

// atexit callback
static void
soproto_cleanup(void)
{
  delete protolist;
  protolist = NULL;
  CC_MUTEX_DESTRUCT(soproto_mutex);
}

static SoProto *
soproto_fetchextern_default_cb(SoInput * in,
                               const SbString * urls,
                               const int numurls,
                               void * COIN_UNUSED_ARG(closure))
{
  if (numurls == 0) return NULL;
  SbString filename(urls[0]);
  SbString name("");

  int nameidx = filename.find("#");
  if (nameidx >= 1) {
    SbString tmp(filename);
    filename = tmp.getSubString(0, nameidx-1);
    name = tmp.getSubString(nameidx+1);
  }

  if (!in->pushFile(filename.getString())) {
    SoReadError::post(in, "Unable to find EXTERNPROTO file: \"%s\"",
                      filename.getString());
    return NULL;
  }

  SoSeparator * root = SoDB::readAll(in);
  if (!root) {
    // Take care of popping the file off the stack. This is a bit
    // "hack-ish", but its done this way instead of loosening the
    // protection of SoInput::popFile().
    if (in->getCurFileName() == filename) {
      char dummy;
      while (!in->eof() && in->get(dummy)) { }

      assert(in->eof());

      // Make sure the stack is really popped on EOF. Popping happens
      // when attempting to read when the current file in the stack is
      // at EOF.
      SbBool gotchar = in->get(dummy);
      if (gotchar) in->putBack(dummy);
    }

    SoReadError::post(in, "Unable to read EXTERNPROTO file: \"%s\"",
                      filename.getString());
    return NULL;
  }
  else {
    root->ref();
    SoProto * foundproto = NULL;

    SoSearchAction sa;
    sa.setType(SoProto::getClassTypeId());
    sa.setSearchingAll(TRUE);
    sa.setInterest(SoSearchAction::ALL);
    sa.apply(root);

    SoPathList & pl = sa.getPaths();

    if (pl.getLength() == 1) {
      foundproto = (SoProto*) pl[0]->getTail();
      if (name.getLength() && name != foundproto->getProtoName().getString()) {
        foundproto = NULL;
      }
    }
    else if (name.getLength()) {
      int i;
      for (i = 0; i < pl.getLength(); i++) {
        SoProto * proto = (SoProto*) pl[i]->getTail();
        if (name == proto->getProtoName().getString()) break;
      }
      if (i < pl.getLength()) {
        foundproto = (SoProto*) pl[i]->getTail();
      }
    }
    sa.reset(); // clear paths in action.
    if (foundproto) foundproto->ref();
    root->unref();
    if (foundproto) foundproto->unrefNoDelete();
    return foundproto;
  }

  // just in case to fool stupid compilers
  return NULL;
}

// *************************************************************************

typedef SbHash<const char *, SoBase *> Name2SoBaseMap;

class SoProtoP {
public:
  SoProtoP() : fielddata(NULL), defroot(NULL) { }

  SoFieldData * fielddata;
  SoGroup * defroot;
  SbName name;
  SbList <SoNode*> isnodelist; // FIXME: consider using SoNodeList
  SbList <SbName> isfieldlist;
  SbList <SbName> isnamelist;
  Name2SoBaseMap refdict;
  SbList <SbName> routelist;
  SoMFString * externurl;
  SoProto * extprotonode;
};

// *************************************************************************

// doc in parent
SoType
SoProto::getTypeId(void) const
{
  return soproto_type;
}

// doc in parent
SoType
SoProto::getClassTypeId(void)
{
  return soproto_type;
}

// doc in parent
void
SoProto::initClass(void)
{
  CC_MUTEX_CONSTRUCT(soproto_mutex);
  soproto_type = SoType::createType(SoNode::getClassTypeId(),
                                    SbName("SoProto"), NULL,
                                    SoNode::nextActionMethodIndex++);
  protolist = new SbList<SoProto*>;

  coin_atexit((coin_atexit_f*) soproto_cleanup, CC_ATEXIT_NORMAL);
  // this will set a default callback
  SoProto::setFetchExternProtoCallback(NULL, NULL);
}

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor.
*/
SoProto::SoProto(const SbBool externproto)
{
  PRIVATE(this) = new SoProtoP;
  PRIVATE(this)->externurl = NULL;
  if (externproto) {
    PRIVATE(this)->externurl = new SoMFString;
  }
  PRIVATE(this)->fielddata = new SoFieldData;
  PRIVATE(this)->defroot = new SoGroup;
  PRIVATE(this)->defroot->ref();
  PRIVATE(this)->extprotonode = NULL;

  CC_MUTEX_LOCK(soproto_mutex);
  protolist->insert(this, 0);
  CC_MUTEX_UNLOCK(soproto_mutex);
}

/*!
  Destructor.
*/
SoProto::~SoProto()
{
  const int n = PRIVATE(this)->fielddata->getNumFields();
  for (int i = 0; i < n; i++) {
    delete PRIVATE(this)->fielddata->getField(this, i);
  }
  PRIVATE(this)->defroot->unref();
  delete PRIVATE(this)->externurl;

  if (PRIVATE(this)->extprotonode) {
    PRIVATE(this)->extprotonode->unref();
  }
  delete PRIVATE(this)->fielddata;
  delete PRIVATE(this);
}

void
SoProto::setFetchExternProtoCallback(SoFetchExternProtoCB * cb,
                                     void * closure)
{
  if (cb == NULL) {
    soproto_fetchextern_cb = soproto_fetchextern_default_cb;
    soproto_fetchextern_closure = NULL;
  }
  else {
    soproto_fetchextern_cb = cb;
    soproto_fetchextern_closure = closure;
  }
}

/*!
  Returns the PROTO definition named \a name or NULL if not found.
*/
SoProto *
SoProto::findProto(const SbName & name)
{
  SoProto * ret = NULL;
  CC_MUTEX_LOCK(soproto_mutex);
  if (protolist) {
    const int n = protolist->getLength();
    SoProto * const * ptr = protolist->getArrayPtr();
    for (int i = 0; (ret == NULL) && (i < n); i++) {
      if (ptr[i]->getProtoName() == name) ret = ptr[i];
    }
  }
  CC_MUTEX_UNLOCK(soproto_mutex);
  return ret;
}

/*!
  Creates an instance of the PROTO.
*/
SoProtoInstance *
SoProto::createProtoInstance(void)
{
  if (PRIVATE(this)->extprotonode) {
    return PRIVATE(this)->extprotonode->createProtoInstance();
  }
  SoProtoInstance * inst = new SoProtoInstance(this, PRIVATE(this)->fielddata);
  inst->ref();
  inst->setRootNode(this->createInstanceRoot(inst));
  return inst;
}

/*!
  Returns the PROTO name.
*/
SbName
SoProto::getProtoName(void) const
{
  return PRIVATE(this)->name;
}

// Documented in superclass. Overridden to read Proto definition.
SbBool
SoProto::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  SbName protoname;

  char c;
  SbBool ok = in->read(protoname, TRUE);
  if (ok) {
    PRIVATE(this)->name = protoname;
    ok = this->readInterface(in);
  }
  if (!ok) {
    SoReadError::post(in, "Error parsing PROTO interface.");
  }
  else if (!PRIVATE(this)->externurl) {
    ok = in->read(c) && c == '{';
    if (ok) ok = this->readDefinition(in);
  }
  else {
    ok = PRIVATE(this)->externurl->read(in, SbName("EXTERNPROTO URL"));
    if (ok) {
      SoProto * proto = soproto_fetchextern_cb(in,
                                               PRIVATE(this)->externurl->getValues(0),
                                               PRIVATE(this)->externurl->getNum(),
                                               soproto_fetchextern_closure);
      if (proto == NULL) {
        SoReadError::post(in, "Error reading EXTERNPROTO definition.");
        ok = FALSE;
      }
      else {
        ok = this->setupExtern(in, proto);
      }
    }
  }
  return ok;
}

// Doc in parent
void
SoProto::destroy(void)
{
  CC_MUTEX_LOCK(soproto_mutex);
  int idx = protolist->find(this);
  assert(idx >= 0);
  protolist->remove(idx);
  CC_MUTEX_UNLOCK(soproto_mutex);
  SoBase::destroy();
}

// doc in parent
void
SoProto::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();
  out->pushProto(this);

  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out, FALSE);
    if (PRIVATE(this)->defroot && !PRIVATE(this)->externurl) {
      this->writeDefinition(action);
    }
    this->writeInterface(out);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    int writerefcount = SoWriterefCounter::instance(out)->getWriteref(this);

    out->write(PRIVATE(this)->externurl ? "EXTERNPROTO " : "PROTO ");
    out->write(PRIVATE(this)->name.getString());
    if (SoWriterefCounter::debugWriterefs()) {
      SbString tmp;
      tmp.sprintf(" [ # writeref: %d\n",
                  writerefcount);
      out->write(tmp.getString());
    }
    else {
      out->write(" [\n");
    }
    out->incrementIndent();

    this->writeInterface(out);

    out->decrementIndent();
    out->indent();
    out->write("]\n");
    if (PRIVATE(this)->externurl) {
      this->writeURLs(out);
    }
    else {
      out->indent();
      out->write("{\n");
      out->incrementIndent();

      if (PRIVATE(this)->defroot) {
        this->writeDefinition(action);
      }
      out->resolveRoutes();
      out->decrementIndent();
      out->indent();
      out->write("}");
    }

#if COIN_DEBUG
    if (SoWriterefCounter::debugWriterefs()) {
      int writerefcount = SoWriterefCounter::instance(out)->getWriteref(this);
      SoDebugError::postInfo("SoProto::write",
                             "%p/%s/'%s': %d -> %d",
                             this,
                             this->getTypeId().getName().getString(),
                             this->getName().getString(),
                             writerefcount, writerefcount - 1);
    }
#endif // COIN_DEBUG

    writerefcount--;
    SoWriterefCounter::instance(out)->setWriteref(this, writerefcount);
  }
  else assert(0 && "unknown stage");

  out->popProto();
}

//
// Writes the PROTO interface
//
SbBool
SoProto::writeInterface(SoOutput * out)
{
  const SoFieldData * fd = PRIVATE(this)->fielddata;

  if (out->getStage() == SoOutput::COUNT_REFS) {
    for (int i = 0; i < fd->getNumFields(); i++) {
      SoField * f = fd->getField(this, i);
      switch (f->getFieldType()) {
      case SoField::NORMAL_FIELD:
      case SoField::EXPOSED_FIELD:
        if (!PRIVATE(this)->externurl) {
          SbBool fieldwasdefault = f->isDefault();
          if (fieldwasdefault) f->setDefault(FALSE);
          f->write(out, fd->getFieldName(i));
          if (fieldwasdefault) f->setDefault(TRUE);
        }
        break;
      }
    }
  }
  else {
    for (int i = 0; i < fd->getNumFields(); i++) {
      out->indent();
      SoField * f = fd->getField(this, i);
      SoType t = f->getTypeId();
      switch (f->getFieldType()) {
      case SoField::NORMAL_FIELD:
        out->write("field ");
        out->write(t.getName().getString());
        if (PRIVATE(this)->externurl) {
          out->write(' ');
          out->write(fd->getFieldName(i).getString());
          out->write("\n");
        }
        else {
          // field values are tagged as default for proto interface instances,
          // but needs to be written to file anyway -- 20040115 larsa
          SbBool fieldwasdefault = f->isDefault();
          if ( fieldwasdefault ) f->setDefault(FALSE);
          f->write(out, fd->getFieldName(i));
          if ( fieldwasdefault ) f->setDefault(TRUE);
        }
        break;
      case SoField::EXPOSED_FIELD:
        out->write("exposedField ");
        out->write(t.getName().getString());
        if (PRIVATE(this)->externurl) {
          out->write(' ');
          out->write(fd->getFieldName(i).getString());
          out->write("\n");
        }
        else {
          SbBool fieldwasdefault = f->isDefault();
          if ( fieldwasdefault ) f->setDefault(FALSE);
          f->write(out, fd->getFieldName(i));
          if ( fieldwasdefault ) f->setDefault(TRUE);
        }
        break;
      case SoField::EVENTIN_FIELD:
        out->write("eventIn ");
        out->write(t.getName().getString());
        out->write(' ');
        out->write(fd->getFieldName(i).getString());
        break;
      case SoField::EVENTOUT_FIELD:
        out->write("eventOut ");
        out->write(t.getName().getString());
        out->write(' ');
        out->write(fd->getFieldName(i).getString());
        break;
      default:
        assert(0 && "invalid field type");
        break;
      }
    }
  }
  return TRUE;
}

//
// Writes the PROTO definition
//
SbBool
SoProto::writeDefinition(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();
  SoGroup * def = PRIVATE(this)->defroot;

  if (out->getStage() == SoOutput::COUNT_REFS) {
    for (int i = 0; i < def->getNumChildren(); i++) {
      def->getChild(i)->write(action);
    }
  }
  else if (out->getStage() == SoOutput::WRITE) {
    for (int i = 0; i < def->getNumChildren(); i++) {
      def->getChild(i)->write(action);
    }
  }
  else assert(0 && "unknown stage");
  return TRUE;
}

SbBool
SoProto::writeURLs(SoOutput * out)
{
  // We use this code to write the URLs to get nicer indentation. Just
  // calling PRIVATE(this)->externurl->write(out, SbName::empty()) would also have
  // produced a valid VRML file.

  const int n = PRIVATE(this)->externurl->getNum();
  if (n == 1) {
    out->indent();
    out->write('\"');
    out->write((*PRIVATE(this)->externurl)[0].getString());
    out->write("\"\n");
  }
  else {
    out->indent();
    out->write("[\n");
    out->incrementIndent();
    for (int i = 0; i < n; i++) {
      out->indent();
      out->write('\"');
      out->write((*PRIVATE(this)->externurl)[i].getString());
      out->write(i < n-1 ? "\",\n" : "\"\n");
    }
    out->decrementIndent();
    out->indent();
    out->write("]\n");
  }
  return TRUE;
}

/*!
  Adds an IS reference for this PROTO definition.
*/
void
SoProto::addISReference(SoNode * container,
                        const SbName & fieldname,
                        const SbName & interfacename)
{
  assert(container->isOfType(SoNode::getClassTypeId()));
  PRIVATE(this)->isnodelist.append(container);
  PRIVATE(this)->isfieldlist.append(fieldname);
  PRIVATE(this)->isnamelist.append(interfacename);
}

/*!
  If \a container is a PROTO definition node with an IS interface
  field named \a fieldname, return the interface name, otherwise
  return an empty SbName.
*/
SbName
SoProto::findISReference(const SoFieldContainer * container,
                         const SbName & fieldname)
{
  const int n = PRIVATE(this)->isnodelist.getLength();
  for (int i = 0; i < n; i++) {
    if (PRIVATE(this)->isnodelist[i] == container &&
        PRIVATE(this)->isfieldlist[i] == fieldname) return PRIVATE(this)->isnamelist[i];
  }
  return SbName::empty();
}


/*!
  Adds a reference for this PROTO definition.
*/
void
SoProto::addReference(const SbName & name, SoBase * base)
{
  PRIVATE(this)->refdict.put(name.getString(), base);
}

/*!
  Removes a reference for this PROTO definition.
*/
void
SoProto::removeReference(const SbName & name)
{
  PRIVATE(this)->refdict.erase(name.getString());
}

/*!
  Finds a reference for this PROTO definition.
*/
SoBase *
SoProto::findReference(const SbName & name) const
{
  SoBase * base;

  if (PRIVATE(this)->refdict.get(name.getString(), base)) { return base; }
  return NULL;
}

/*!
  Adds a ROUTE for this PROTO definition.
*/
void
SoProto::addRoute(const SbName & fromnode, const SbName & fromfield,
                  const SbName & tonode, const SbName & tofield)
{
  PRIVATE(this)->routelist.append(fromnode);
  PRIVATE(this)->routelist.append(fromfield);
  PRIVATE(this)->routelist.append(tonode);
  PRIVATE(this)->routelist.append(tofield);
}

//
// Reads the interface
//
SbBool
SoProto::readInterface(SoInput * in)
{
  SbBool ok = PRIVATE(this)->fielddata->readFieldDescriptions(in, this, 4, PRIVATE(this)->externurl == NULL);
  if ( ok ) {
    const int numfields = PRIVATE(this)->fielddata->getNumFields();
    for (int i = 0; i < numfields; i++) {
      SoField * f = PRIVATE(this)->fielddata->getField(this, i);
      switch ( f->getFieldType() ) {
      case SoField::NORMAL_FIELD:
      case SoField::EXPOSED_FIELD:
        f->setDefault(TRUE);
      }
    }
  }
  return ok;
}

//
// Reads the definition
//
SbBool
SoProto::readDefinition(SoInput * in)
{
  SbBool ok = TRUE;
  SoBase * child;
  in->pushProto(this);

  while (ok) {
    ok = SoBase::read(in, child, SoNode::getClassTypeId());
    if (ok) {
      if (child == NULL) {
        if (in->eof()) {
          ok = FALSE;
          SoReadError::post(in, "Premature end of file");
        }
        break; // finished reading, break out
      }
      else {
        PRIVATE(this)->defroot->addChild((SoNode*) child);
      }
    }
  }
  in->popProto();
  char c;
  return ok && in->read(c) && c == '}';
}

static SoNode *
soproto_find_node(SoNode * root, SbName name, SoSearchAction & sa)
{
  sa.setName(name);
  sa.setInterest(SoSearchAction::FIRST);
  sa.setSearchingAll(TRUE);

  sa.apply(root);

  SoNode * ret = NULL;

  if (sa.getPath()) {
    ret = ((SoFullPath*)sa.getPath())->getTail();
  }
  sa.reset();
  return ret;
}

//
// Used to check for fieldname. Will first test "<name>", then "set_<name>",
// and then "<name>_changed".
//
static SbName
soproto_find_fieldname(SoNode * node, const SbName & name)
{
  if (node->getField(name)) return name;
  SbString test;
  if (strncmp("set_", name.getString(), 4) == 0) {
    test = name.getString() + 4;
    if (node->getField(test.getString())) return SbName(test.getString());
  }
  test = name.getString();
  // test for "_changed" at the end of the fieldname
  if (test.getLength() > 8) { // 8 == strlen("_changed")
    test = test.getSubString(0, test.getLength() - 9);
    if (node->getField(test.getString())) return SbName(test.getString());
  }
  return name;
}

//
// Used to check for outputname. Will first test "<name>", then "set_<name>",
// and then "<name>_changed".
//
static SbName
soproto_find_outputname(SoNodeEngine * node, const SbName & name)
{
  if (node->getOutput(name)) return name;
  SbString test;
  if (strncmp("set_", name.getString(), 4) == 0) {
    test = name.getString() + 4;
    if (node->getOutput(test.getString())) return SbName(test.getString());
  }
  test = name.getString();
  // test for "_changed" at the end of the fieldname
  if (test.getLength() > 8) { // 8 == strlen("_changed")
    test = test.getSubString(0, test.getLength() - 9);
    if (node->getOutput(test.getString())) return SbName(test.getString());
  }
  return name;
}


//
// helper function to find field. First test the actual fieldname,
// then set set_<fieldname>, then <fieldname>_changed.
//
static SoField *
soproto_find_field(SoNode * node, const SbName & fieldname)
{
  SoField * field = node->getField(fieldname);

  if (!field) {
    if (strncmp(fieldname.getString(), "set_", 4) == 0) {
      SbName newname = fieldname.getString() + 4;
      field = node->getField(newname);
    }
    else {
      SbString str = fieldname.getString();
      int len = str.getLength();
      const char CHANGED[] = "_changed";
      const int changedsize = sizeof(CHANGED) - 1;

      if (len > changedsize && strcmp(str.getString()+len-changedsize,
                                      CHANGED) == 0) {
        SbString substr = str.getSubString(0, len-(changedsize+1));
        SbName newname = substr.getString();
        field = node->getField(newname);
      }
    }
  }
  return field;
}

//
// Create a root node for a PROTO instance
//
SoNode *
SoProto::createInstanceRoot(SoProtoInstance * inst) const
{
  if (PRIVATE(this)->extprotonode) {
    return PRIVATE(this)->extprotonode->createInstanceRoot(inst);
  }

  SoNode * root;
  if (PRIVATE(this)->defroot->getNumChildren() == 1)
    root = PRIVATE(this)->defroot->getChild(0);
  else root = PRIVATE(this)->defroot;

  SoNode * cpy;
  cpy = root->copy(FALSE);
  cpy->ref();
  this->connectISRefs(inst, root, cpy);

  int n = PRIVATE(this)->routelist.getLength() / 4;
  SoSearchAction sa;

  for (int i = 0; i < n; i++) {
    SbName fromnodename = PRIVATE(this)->routelist[i*4];
    SbName fromfieldname = PRIVATE(this)->routelist[i*4+1];
    SbName tonodename = PRIVATE(this)->routelist[i*4+2];
    SbName tofieldname = PRIVATE(this)->routelist[i*4+3];

    SoNode * fromnode = soproto_find_node(cpy, fromnodename, sa);
    SoNode * tonode = soproto_find_node(cpy, tonodename, sa);

    if (fromnode && tonode) {
      SoField * from = soproto_find_field(fromnode, fromfieldname);
      SoField * to = soproto_find_field(tonode, tofieldname);
      SoEngineOutput * output = NULL;
      if (from == NULL && fromnode->isOfType(SoNodeEngine::getClassTypeId())) {
        output = ((SoNodeEngine*) fromnode)->getOutput(fromfieldname);
      }

      if (to && (from || output)) {
        SbBool notnotify = FALSE;
        SbBool append = FALSE;
        if (output || from->getFieldType() == SoField::EVENTOUT_FIELD) {
          notnotify = TRUE;
        }
        if (to->getFieldType() == SoField::EVENTIN_FIELD) append = TRUE;

        // Check that there exists a field converter, if one is needed.
        SoType totype = to->getTypeId();
        SoType fromtype = from ? from->getTypeId() : output->getConnectionType();
        if (totype != fromtype) {
          SoType convtype = SoDB::getConverter(fromtype, totype);
          if (convtype == SoType::badType()) {
            continue;
          }
        }

        SbBool ok;
        if (from) ok = to->connectFrom(from, notnotify, append);
        else ok = to->connectFrom(output, notnotify, append);
        // Both known possible failure points are caught above.
        assert(ok && "unexpected connection error");

      }
    }
  }

  cpy->unrefNoDelete();
  return cpy;
}

static SoNode *
locate_node_copy(SoNode * searchfor, SoNode * org, SoNode * cpy)
{
  if (org == NULL) return NULL;
  if (cpy == NULL) return NULL;

  if (org->getTypeId() != cpy->getTypeId()) return NULL;
  if (org == searchfor) return cpy;

  const SoFieldData * fd = org->getFieldData();
  const SoFieldData * fd2 = cpy->getFieldData();

  int n = fd ? fd->getNumFields() : 0;
  int n2 = fd2 ? fd2->getNumFields() : 0;

  if (n != n2) {
    // should never happen (in theory)
    SoDebugError::postWarning("SoProto::locate_node_copy",
                              "SoFieldData mismatch in PROTO scene.");
    return NULL;
  }

  int i;

  SoType sosftype = SoSFNode::getClassTypeId();
  for (i = 0; i < n; i++) {
    SoField * orgf = fd->getField(org, i);
    if (orgf->getTypeId() == sosftype) {
      SoNode * orgnode = ((SoSFNode*) orgf)->getValue();
      if (orgnode != NULL) {
        SoField * cpyf = fd2->getField(cpy, i);
        if (cpyf->getTypeId() == sosftype) {
          SoNode * found = locate_node_copy(searchfor, orgnode, ((SoSFNode*) cpyf)->getValue());
          if (found) return found;
        }
        else {
          SoDebugError::postWarning("SoProto::locate_node_copy",
                                    "SoField mismatch in PROTO scene.");
          return NULL;
        }
      }
    }
  }

  SoChildList * cl = org->getChildren();
  if (cl) {
    SoChildList * cl2 = cpy->getChildren();
    n = SbMin(cl->getLength(), cl2->getLength());
    for (i = 0; i < n; i++) {
      SoNode * found = locate_node_copy(searchfor, (*cl)[i], (*cl2)[i]);
      if (found) return found;
    }
  }
  return NULL;
}


//
// Connects all IS references for the a new instance
//
void
SoProto::connectISRefs(SoProtoInstance * inst, SoNode * src, SoNode * dst) const
{
  if (PRIVATE(this)->externurl) {
    SoDebugError::postWarning("SoProto::connectISRefs",
                              "EXTERNPROTO URL fetching is not yet supported.");
    return;
  }

  const int n = PRIVATE(this)->isfieldlist.getLength();

  for (int i = 0; i < n; i++) {
    SoNode * node = PRIVATE(this)->isnodelist[i];

    SbName fieldname = PRIVATE(this)->isfieldlist[i];
    fieldname = soproto_find_fieldname(node, fieldname);
    SoField * dstfield = node->getField(fieldname);
    SoEngineOutput * eventout = NULL;

    if (!dstfield) {
      if (node->isOfType(SoNodeEngine::getClassTypeId())) {
        fieldname = soproto_find_outputname((SoNodeEngine*)node, fieldname);
        eventout = ((SoNodeEngine*)node)->getOutput(fieldname);
      }
      if (!eventout) {
#if COIN_DEBUG
        SoDebugError::postWarning("SoProto::connectISRefs",
                                  "Destination field '%s' is not found in node type '%s'. "
                                  "Unable to resolve IS reference.",
                                  fieldname.getString(), node->getTypeId().getName().getString());
#endif // COIN_DEBUG
        continue; // skip to next field
      }
    }

    SbBool isprotoinstance = FALSE;
    if (node->isOfType(SoProtoInstance::getClassTypeId())) {
      node = ((SoProtoInstance*) node)->getRootNode();
      isprotoinstance = TRUE;
    }
    SbName iname = PRIVATE(this)->isnamelist[i];

    node = locate_node_copy(node, src, dst);

    if (!node) {
      SoDebugError::postWarning("SoProto::connectISRefs",
                                "Unable to find '%s' from '%s' in '%s' PROTO",
                                fieldname.getString(), iname.getString(), PRIVATE(this)->name.getString());
      continue;
    }

    if (dstfield) {
      if (isprotoinstance) {
        node = SoProtoInstance::findProtoInstance(node);
        assert(node);
      }
      dstfield = node->getField(fieldname);
    }
    else {
      assert(node->isOfType(SoNodeEngine::getClassTypeId()));
      eventout = ((SoNodeEngine*)node)->getOutput(fieldname);
    }
    assert(dstfield || eventout);
    SoField * srcfield = inst->getField(iname);
    if (srcfield) {
      // if destination field is an eventOut field, or an EngineOutput,
      // reverse the connection, since we then just need to route the
      // events to the srcfield.
      if (eventout) {
        srcfield->connectFrom(eventout);
      }
      else if (dstfield->getFieldType() == SoField::EVENTOUT_FIELD) {
        srcfield->connectFrom(dstfield);
      }
      else {
        // We make bidirectional connections for regular fields.  That way
        // you can modify the fields in the scene graph and have their PROTO
        // instances written to file with the updated values.  The alternative
        // is to have to locate the PROTO instance object yourself, and
        // modify the fields on it directly - 20040115 larsa
        srcfield->setDefault(FALSE);
        dstfield->connectFrom(srcfield);
#if 0 // start of problematic code
        // this piece of code causes problems when writing PROTO
        // instances, since the PROTO instance is counted once for
        // each IS connection. The code is enabled for now, but I'll
        // investigate more if this bidirectional connection is really
        // necessary and if we should handle this case when counting
        // write references. pederb, 2005-11-15

        // update 2005-12-16, pederb:
        // This bidirectional thingie also causes bugs when importing
        // gator_1.wrl (the connections are not set up correctly, or
        // are messed up). It seems like we probably should disable
        // this code since it causes a lot of problems.

        // propagate value immediately, before setting up reverse connection
        dstfield->evaluate();
        srcfield->connectFrom(dstfield, FALSE, TRUE);
        // propagate value immediately, so we can tag field as default
        srcfield->evaluate();
        if ( srcisdefault ) srcfield->setDefault(TRUE);
#endif // end of problemetic code
      }
    }
    else {
      assert(dstfield);
      SoEngineOutput * output = NULL;
      if (inst->isOfType(SoNodeEngine::getClassTypeId())) {
        output = ((SoNodeEngine*) inst)->getOutput(iname);
      }
      if (output) {
        dstfield->connectFrom(output);
      }
#if COIN_DEBUG
      else {
        SoDebugError::postWarning("SoProto::connectISRefs",
                                  "Source field or engine output '%s' is not found in node type '%s'. "
                                  "Unable to resolve IS reference.",
                                  iname.getString(), node->getTypeId().getName().getString());
      }
#endif // COIN_DEBUG
    }
  }
}

SbBool
SoProto::setupExtern(SoInput * COIN_UNUSED_ARG(in), SoProto * externproto)
{
  assert(externproto);
  PRIVATE(this)->extprotonode = externproto;
  PRIVATE(this)->extprotonode->ref();
  return TRUE;
}

#undef PRIVATE
