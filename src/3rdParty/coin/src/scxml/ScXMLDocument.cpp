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

#include <Inventor/scxml/ScXMLDocument.h>

/*!
  \class ScXMLDocument ScXMLDocument.h Inventor/scxml/Document.h
  The &lt;ScXMLDocument class is a container class for a complete SCXML document.

*/

#include <cassert>
#include <cstring>
#include <map>

#include <memory>

#include <Inventor/C/XML/document.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/misc/CoinResources.h>
#include <Inventor/scxml/ScXMLScxmlElt.h>
#include <Inventor/scxml/ScXMLStateElt.h>
#include <Inventor/scxml/ScXMLParallelElt.h>
#include <Inventor/scxml/ScXMLHistoryElt.h>
#include <Inventor/scxml/ScXMLAnchorElt.h>
#include <Inventor/scxml/ScXMLDataElt.h>
#include <Inventor/scxml/ScXMLDataModelElt.h>
#include <Inventor/scxml/ScXMLLogElt.h>
#include <Inventor/scxml/ScXMLTransitionElt.h>
#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLFinalizeElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>
#include <Inventor/scxml/ScXMLAssignElt.h>

#include <Inventor/errors/SoDebugError.h>

#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strncmp;
using std::strcmp;
using std::strcpy;
using std::strlen;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLDocument::PImpl {
public:
  PImpl(void)
  { }
  ~PImpl(void) { }

  std::unique_ptr<char[]> filename;
  std::unique_ptr<ScXMLScxmlElt> root;

  typedef std::map<const char *, ScXMLAbstractStateElt *> StateIdMap;
  typedef std::map<const char *, ScXMLDataElt *> DataIdMap;
  std::unique_ptr<StateIdMap> stateidmap;
  std::unique_ptr<DataIdMap> dataidmap;

  void fillIdentifierMaps(ScXMLElt * object);

  ScXMLAbstractStateElt * getStateByIdentifier(SbName id) const;
  ScXMLDataElt * getDataById(SbName name) const;

};

void
ScXMLDocument::PImpl::fillIdentifierMaps(ScXMLElt * object)
{
  assert(object);

  if (object->isOfType(ScXMLAbstractStateElt::getClassTypeId())) {
    ScXMLAbstractStateElt * state = static_cast<ScXMLStateElt *>(object);
    SbName id(state->getIdAttribute());
    this->stateidmap->insert(std::pair<const char *, ScXMLAbstractStateElt *>(id.getString(), state));
  }

  if (object->isOfType(ScXMLDataElt::getClassTypeId())) {
    ScXMLDataElt * data = static_cast<ScXMLDataElt *>(object);
    const SbName id(data->getIDAttribute());
    this->dataidmap->insert(std::pair<const char *, ScXMLDataElt *>(id.getString(), data));
  }

  if (object->isOfType(ScXMLAnchorElt::getClassTypeId())) {
    /*ScXMLAnchorElt * anchor = static_cast<ScXMLAnchorElt *>(object);*/
  }
  else if (object->isOfType(ScXMLDataElt::getClassTypeId())) {
    /*ScXMLDataElt * data = static_cast<ScXMLDataElt *>(object);*/
  }
  else if (object->isOfType(ScXMLScxmlElt::getClassTypeId())) {
    ScXMLScxmlElt * root = static_cast<ScXMLScxmlElt *>(object);
    int c;
    for (c = 0; c < root->getNumStates(); ++c) {
      this->fillIdentifierMaps(root->getState(c));
    }
    for (c = 0; c < root->getNumParallels(); ++c) {
      this->fillIdentifierMaps(root->getParallel(c));
    }
    for (c = 0; c < root->getNumFinals(); ++c) {
      this->fillIdentifierMaps(root->getFinal(c));
    }
  }
  else if (object->isOfType(ScXMLFinalElt::getClassTypeId())) {
    /*ScXMLFinalElt * final = static_cast<ScXMLFinalElt *>(object);*/
  }
  else if (object->isOfType(ScXMLHistoryElt::getClassTypeId())) {
    const ScXMLHistoryElt * history = static_cast<ScXMLHistoryElt *>(object);
    if (history->getTransition()) {
      this->fillIdentifierMaps(history->getTransition());
    }
  }
  else if (object->isOfType(ScXMLInitialElt::getClassTypeId())) {
    ScXMLInitialElt * initial = static_cast<ScXMLInitialElt *>(object);
    if (initial->getTransition()) {
      this->fillIdentifierMaps(initial->getTransition());
    }
  }
  else if (object->isOfType(ScXMLInvokeElt::getClassTypeId())) {
    /*ScXMLInvokeElt * invoke = static_cast<ScXMLInvokeElt *>(object);*/
  }
  else if (object->isOfType(ScXMLOnEntryElt::getClassTypeId())) {
    ScXMLOnEntryElt * onentry = static_cast<ScXMLOnEntryElt *>(object);
    int c;
    for (c = 0; c < onentry->getNumExecutables(); ++c) {
      this->fillIdentifierMaps(onentry->getExecutable(c));
    }
  }
  else if (object->isOfType(ScXMLOnExitElt::getClassTypeId())) {
    ScXMLOnExitElt * onexit = static_cast<ScXMLOnExitElt *>(object);
    int c;
    for (c = 0; c < onexit->getNumExecutables(); ++c) {
      this->fillIdentifierMaps(onexit->getExecutable(c));
    }
  }
  else if (object->isOfType(ScXMLStateElt::getClassTypeId())) {
    ScXMLStateElt * state = static_cast<ScXMLStateElt *>(object);
    int c;
    if (state->getOnEntry()) {
      this->fillIdentifierMaps(state->getOnEntry());
    }
    if (state->getOnExit()) {
      this->fillIdentifierMaps(state->getOnExit());
    }
    for (c = 0; c < state->getNumTransitions(); ++c) {
      this->fillIdentifierMaps(state->getTransition(c));
    }
    if (state->getInitial()) {
      this->fillIdentifierMaps(state->getInitial());
    }
    for (c = 0; c < state->getNumStates(); ++c) {
      this->fillIdentifierMaps(state->getState(c));
    }
    for (c = 0; c < state->getNumParallels(); ++c) {
      this->fillIdentifierMaps(state->getParallel(c));
    }
    for (c = 0; c < state->getNumFinals(); ++c) {
      this->fillIdentifierMaps(state->getFinal(c));
    }
    for (c = 0; c < state->getNumHistories(); ++c) {
      this->fillIdentifierMaps(state->getHistory(c));
    }
    for (c = 0; c < state->getNumAnchors(); ++c) {
      this->fillIdentifierMaps(state->getAnchor(c));
    }
    if (state->getDataModel()) {
      this->fillIdentifierMaps(state->getDataModel());
    }
  }
  else if (object->isOfType(ScXMLTransitionElt::getClassTypeId())) {
    ScXMLTransitionElt * transition = static_cast<ScXMLTransitionElt *>(object);
    int c;
    for (c = 0; c < transition->getNumExecutables(); ++c) {
      this->fillIdentifierMaps(transition->getExecutable(c));
    }
  }
  else if (object->isOfType(ScXMLLogElt::getClassTypeId())) {
    /*ScXMLLogElt * log = static_cast<ScXMLLogElt *>(object);*/
  }
  else if (object->isOfType(ScXMLSendElt::getClassTypeId())) {
    /*ScXMLSendElt * send = static_cast<ScXMLSendElt *>(object);*/
  }
  else if (object->isOfType(ScXMLAssignElt::getClassTypeId())) {
    /*ScXMLAssignElt * assign = static_cast<ScXMLAssignElt *>(object);*/
  }
  else if (object->isOfType(ScXMLDataModelElt::getClassTypeId())) {
    ScXMLDataModelElt * datamodel = static_cast<ScXMLDataModelElt *>(object);
    for (int c = 0; c < datamodel->getNumData(); ++c) {
      this->fillIdentifierMaps(datamodel->getData(c));
    }
  }
  else {
    SoDebugError::postInfo("ScXMLDocument::fillIdentifierMap",
                           "unsupported object type %s",
                           object->getTypeId().getName().getString());
  }
}

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_OBJECT_SOURCE(ScXMLDocument);

void
ScXMLDocument::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLDocument, ScXMLObject, "ScXMLObject");
}

void
ScXMLDocument::cleanClass(void)
{
  ScXMLDocument::classTypeId = SoType::badType();
}

ScXMLDocument::ScXMLDocument(void)
{
}

ScXMLDocument::~ScXMLDocument(void)
{
}

namespace {
struct cc_xml_doc_deleter {
  void operator()(cc_xml_doc * doc) const
  {
    if (doc) cc_xml_doc_delete_x(doc);
  }
};

typedef std::unique_ptr<cc_xml_doc, cc_xml_doc_deleter> cc_xml_doc_ptr;
} // namespace

ScXMLDocument *
ScXMLDocument::readFile(const char * filename)
{
  ScXMLDocument * scxmldoc = NULL;

  if (strncmp(filename, "coin:", 5) == 0) { // is a "resource"
    //size_t buffersize = 0;
    SbByteBuffer buffer = CoinResources::get(filename);
    if (likely(buffer.isValid())) {
      scxmldoc = ScXMLDocument::readBuffer(buffer);
      if (scxmldoc) {
        scxmldoc->setFilename(filename);
      }
      return scxmldoc;
    }
  }

  cc_xml_doc_ptr xmldoc(cc_xml_doc_new());
  if (unlikely(!cc_xml_doc_read_file_x(xmldoc.get(), filename))) {
    return NULL;
  }

  scxmldoc = ScXMLDocument::readXMLData(xmldoc.get());
  if (unlikely(!scxmldoc)) {
    return NULL;
  }

  scxmldoc->setFilename(filename);

  return scxmldoc;
}

ScXMLDocument *
ScXMLDocument::readBuffer(const SbByteBuffer & buffer)
{
  if (buffer.size()==0) return NULL;

  cc_xml_doc_ptr xmldoc(cc_xml_doc_new());
  if (unlikely(!cc_xml_doc_read_buffer_x(xmldoc.get(), buffer.constData(), buffer.size()))) {
    return NULL;
  }

  ScXMLDocument * scxmldoc = ScXMLDocument::readXMLData(xmldoc.get());
  if (unlikely(!scxmldoc)) {
    return NULL;
  }

  scxmldoc->setFilename("<memory buffer>");

  return scxmldoc;
}

ScXMLDocument *
ScXMLDocument::readXMLData(cc_xml_doc * xmldoc)
{
  // get a handle on the root element
  cc_xml_elt * root = cc_xml_doc_get_root(xmldoc);
  if (unlikely(strcmp(cc_xml_elt_get_type(root), "scxml") != 0)) {
    SoDebugError::post("ScXMLDocument::readXMLData",
                       "expected root to be an <scxml> element, not '%s'",
                       cc_xml_elt_get_type(root));
    return NULL;
  }

  ScXMLEltReader * reader = ScXMLScxmlElt::getElementReader();
  ScXMLDocument * scxmldoc = new ScXMLDocument;
  ScXMLElt * elt = reader->read(NULL, root, scxmldoc, NULL);
  if (unlikely(!elt)) {
    delete scxmldoc;
    return NULL;
  }
  assert(elt->isOfType(ScXMLScxmlElt::getClassTypeId()));
  scxmldoc->setRoot(static_cast<ScXMLScxmlElt *>(elt));
  return scxmldoc;
}

void
ScXMLDocument::setFilename(const char * filename)
{
  if (filename) {
    const size_t len = strlen(filename);
    PRIVATE(this)->filename.reset(new char [len + 1]);
    strcpy(PRIVATE(this)->filename.get(), filename);
  } else {
    PRIVATE(this)->filename.reset();
  }
}

const char *
ScXMLDocument::getFilename(void) const
{
  return PRIVATE(this)->filename.get();
}

void
ScXMLDocument::setRoot(ScXMLScxmlElt * root)
{
  PRIVATE(this)->root.reset(root);
}

ScXMLScxmlElt *
ScXMLDocument::getRoot(void) const
{
  return PRIVATE(this)->root.get();
}

ScXMLAbstractStateElt *
ScXMLDocument::getStateById(SbName id) const
{
  if (PRIVATE(this)->stateidmap.get() == NULL) {
    ScXMLDocument * thisp = const_cast<ScXMLDocument *>(this);
    PRIVATE(thisp)->stateidmap.reset(new PImpl::StateIdMap);
    PRIVATE(thisp)->dataidmap.reset(new PImpl::DataIdMap);
    PRIVATE(thisp)->fillIdentifierMaps(PRIVATE(this)->root.get());
  }
  std::map<const char *, ScXMLAbstractStateElt *>::const_iterator it =
    PRIVATE(this)->stateidmap->find(id.getString());
  if (it != PRIVATE(this)->stateidmap->end()) {
    return it->second;
  }
  return NULL;
}

ScXMLDataElt *
ScXMLDocument::getDataById(SbName id) const
{
  if (PRIVATE(this)->dataidmap.get() == NULL) {
    ScXMLDocument * thisp = const_cast<ScXMLDocument *>(this);
    PRIVATE(thisp)->stateidmap.reset(new PImpl::StateIdMap);
    PRIVATE(thisp)->dataidmap.reset(new PImpl::DataIdMap);
    PRIVATE(thisp)->fillIdentifierMaps(PRIVATE(this)->root.get());
  }
  std::map<const char *, ScXMLDataElt *>::const_iterator it =
    PRIVATE(this)->dataidmap->find(id.getString());
  if (it != PRIVATE(this)->dataidmap->end()) {
    return it->second;
  }
  return NULL;
}

#undef PRIVATE
