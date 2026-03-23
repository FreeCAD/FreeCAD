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

#include <Inventor/scxml/ScXMLScxmlElt.h>

/*!
  \class ScXMLScxmlElt ScXMLScxmlElt.h Inventor/scxml/ScXMLScxmlElt.h
  \brief implements the &lt;scxml&gt; SCXML element.

  An object of this type represents the root &lt;scxml&gt; SCXML element in
  a document.  It has the following attributes:

  - initial
  - name
  - xmlns
  - version [REQUIRED] must contain "1.0"
  - profile
  - exmode - "lax" or "strict"

  The &lt;scxml&gt; element can have child elements of the following types:
  - initial - zero or one element of this type, depending on whether the "initial" attribute is set or not (one of them must be set, and only one), but only when the &lt;scxml&gt; element is the root element and not an external reference from another document.
  - state - zero or more elements
  - parallel - zero or more elements
  - final - zero or more elements
  - datamodel - zero or one if the data model module is enabled

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLStateElt.h>
#include <Inventor/scxml/ScXMLParallelElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLDataModelElt.h>
#include <Inventor/scxml/ScXMLScriptElt.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLMinimumEvaluator.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLScxmlEltReader : public ScXMLEltReader {
public:
  ScXMLScxmlEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt,
                          ScXMLDocument * doc, ScXMLStateMachine * sm);

}; // ScXMLScxmlEltReader

ScXMLScxmlEltReader::ScXMLScxmlEltReader(void)
: ScXMLEltReader("scxml")
{
}

ScXMLElt *
ScXMLScxmlEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(xmlelt);

  ScXMLScxmlElt * scxml = new ScXMLScxmlElt;
  assert(scxml);
  scxml->setContainer(container);
  this->setXMLAttributes(scxml, xmlelt);

  // handle XML attributes
  if (unlikely(!scxml->handleXMLAttributes())) {
    SoDebugError::post("ScXMLScxmlEltReader::read",
                       "error: invalid XML attributes");
    delete scxml;
    return NULL;
  }

#if 0
  if (!container) {
    // this is the root scxml element in a state machine, hence it determines
    // the profile for the state machine, meaning the evaluator is set based on
    // this profile setting.
    const char * profile = scxml->getProfileAttribute();
    if (!profile || strcmp(profile, "minimum") == 0 || strcmp(profile, "min") == 0) {
    }
    else if (strcmp(profile, "x-coin") == 0) {
    }
    else if (strcmp(profile, "xpath") == 0) {
    }
    else if (strcmp(profile, "ecmascript") == 0 || strcmp(profile, "ecma") == 0) {
    }
    else {
    }
  }
#endif

  // read in all children, and recurse down
  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    if (strcmp(elementtype, "initial") == 0) {
      // <initial> - zero or one time
      if (scxml->getInitial()) {
        SoDebugError::post("ScXMLScxmlEltReader::read",
                           "<scxml> can only contain one <initial> element");
        delete scxml;
        return NULL;
      }
      ScXMLEltReader * initialreader = ScXMLInitialElt::getElementReader();
      assert(initialreader);
      ScXMLElt * initialobj = initialreader->read(scxml, element, doc, sm);
      if (unlikely(!initialobj)) {
        delete scxml;
        return NULL;
      }
      assert(initialobj->isOfType(ScXMLInitialElt::getClassTypeId()));
      scxml->setInitial(static_cast<ScXMLInitialElt *>(initialobj));
    }

    else if (strcmp(elementtype, "state") == 0) {
      // <state> - zero or more times
      ScXMLEltReader * statereader = ScXMLStateElt::getElementReader();
      assert(statereader);
      ScXMLElt * stateobj = statereader->read(scxml, element, doc, sm);
      if (unlikely(!stateobj)) {
        delete scxml;
        return NULL;
      }
      assert(stateobj->isOfType(ScXMLStateElt::getClassTypeId()));
      scxml->addState(static_cast<ScXMLStateElt *>(stateobj));
    }

    else if (strcmp(elementtype, "parallel") == 0) {
      // <parallel> - zero or more times
      ScXMLEltReader * parallelreader = ScXMLParallelElt::getElementReader();
      assert(parallelreader);
      ScXMLElt * parallelobj = parallelreader->read(scxml, element, doc, sm);
      if (unlikely(!parallelobj)) {
        delete scxml;
        return NULL;
      }
      assert(parallelobj->isOfType(ScXMLParallelElt::getClassTypeId()));
      scxml->addParallel(static_cast<ScXMLParallelElt *>(parallelobj));
    }

    else if (strcmp(elementtype, "final") == 0) {
      // <final> - zero or more times
      ScXMLEltReader * finalreader = ScXMLFinalElt::getElementReader();
      assert(finalreader);
      ScXMLElt * finalobj = finalreader->read(scxml, element, doc, sm);
      if (unlikely(!finalobj)) {
        delete scxml;
        return NULL;
      }
      assert(finalobj->isOfType(ScXMLFinalElt::getClassTypeId()));
      scxml->addFinal(static_cast<ScXMLFinalElt *>(finalobj));
    }

    else if (strcmp(elementtype, "datamodel") == 0) {
      // <datamodel> - zero or one time
      if (unlikely(scxml->getDataModel() != NULL)) {
        SoDebugError::post("ScXMLScxmlEltReader::read",
                           "<scxml> can only contain one <datamodel> element");
        delete scxml;
        return NULL;
      }
      ScXMLEltReader * datamodelreader = ScXMLDataModelElt::getElementReader();
      assert(datamodelreader);
      ScXMLElt * datamodelobj = datamodelreader->read(scxml, element, doc, sm);
      if (unlikely(!datamodelobj)) {
        delete scxml;
        return NULL;
      }
      assert(datamodelobj->isOfType(ScXMLDataModelElt::getClassTypeId()));
      scxml->setDataModel(static_cast<ScXMLDataModelElt *>(datamodelobj));
    }

    else if (strcmp(elementtype, "script") == 0) {
      // <script> - zero or more times
      ScXMLEltReader * scriptreader = ScXMLScriptElt::getElementReader();
      assert(scriptreader);
      ScXMLElt * scriptobj = scriptreader->read(scxml, element, doc, sm);
      if (unlikely(!scriptobj)) {
        delete scxml;
        return NULL;
      }
      assert(scriptobj->isOfType(ScXMLScriptElt::getClassTypeId()));
      scxml->addScript(static_cast<ScXMLScriptElt *>(scriptobj));
    }

    else {
      SoDebugError::post("ScXML::readFile",
                         "unexpected XML element '<%s>' found in <scxml>",
                         elementtype);
      delete scxml;
      return NULL;
    }
  }

  return scxml;
} // read()

// *************************************************************************

class ScXMLScxmlElt::PImpl {
public:
  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->statelist, ScXMLStateElt *);
    SCXML__CLEAR_STD_VECTOR(this->parallellist, ScXMLParallelElt *);
    SCXML__CLEAR_STD_VECTOR(this->finallist, ScXMLFinalElt *);
    SCXML__CLEAR_STD_VECTOR(this->scriptlist, ScXMLScriptElt *);
  }

  std::unique_ptr<ScXMLInitialElt> initialelt;
  std::vector<ScXMLStateElt *> statelist;
  std::vector<ScXMLParallelElt *> parallellist;
  std::vector<ScXMLFinalElt *> finallist;
  std::unique_ptr<ScXMLDataModelElt> datamodelelt;
  std::vector<ScXMLScriptElt *> scriptlist;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLScxmlElt);

void
ScXMLScxmlElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLScxmlElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLScxmlElt, "scxml", ScXMLScxmlEltReader);
}

void
ScXMLScxmlElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLScxmlElt);
  ScXMLScxmlElt::classTypeId = SoType::badType();
}

ScXMLScxmlElt::ScXMLScxmlElt(void)
: referenced(FALSE),
  initial(NULL),
  name(NULL),
  xmlns(NULL),
  version(NULL),
  profile(NULL),
  exmode(NULL)
{
}

ScXMLScxmlElt::~ScXMLScxmlElt(void)
{
  this->setInitialAttribute(NULL);
  this->setNameAttribute(NULL);
  // this->setXMLNSAttribute(NULL);
  this->setVersionAttribute(NULL);
  this->setProfileAttribute(NULL);
  this->setExModeAttribute(NULL);
}

void
ScXMLScxmlElt::setIsReferenced(SbBool isreferenced)
{
  this->referenced = isreferenced;
}

SbBool
ScXMLScxmlElt::isReferenced(void) const
{
  return this->referenced;
}

void
ScXMLScxmlElt::setInitialAttribute(const char * initialstr)
{
  if (this->initial &&
      (this->initial != this->getXMLAttribute("initial")) &&
      (this->initial != this->getXMLAttribute("initialstate"))) // Feb 2007 compat
  {
    delete [] this->initial;
  }
  this->initial = NULL;
  if (initialstr) {
    if (initialstr == this->getXMLAttribute("initial")) {
      this->initial = const_cast<char *>(initialstr);
    }
    else if (initialstr == this->getXMLAttribute("initialstate")) {
      this->initial = const_cast<char *>(initialstr);
    }
    else {
      this->initial = new char [ strlen(initialstr) + 1 ];
      strcpy(this->initial, initialstr);
    }
  }
}

// const char * ScXMLScxmlElt::getInitialAttribute(void) const

void
ScXMLScxmlElt::setNameAttribute(const char * namestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->name, "name", namestr);
}

// const char * ScXMLScxmlElt::getNameAttribute(void) const

void
ScXMLScxmlElt::setXMLNSAttribute(const char * xmlnsstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->xmlns, "xmlns", xmlnsstr);
  // If set, value should be "http://www.w3.org/2005/07/scxml"
}

// const char * ScXMLScxmlElt::getXMLNSAttribute(void) const

void
ScXMLScxmlElt::setVersionAttribute(const char * versionstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->version, "version", versionstr);
}

// const char * ScXMLScxmlElt::getVersionAttribute(void) const

void
ScXMLScxmlElt::setProfileAttribute(const char * profilestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->profile, "profile", profilestr);
}

// const char * ScXMLScxmlElt::getProfileAttribute(void) const

void
ScXMLScxmlElt::setExModeAttribute(const char * exmodestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->exmode, "exmode", exmodestr);
}

// const char * ScXMLScxmlElt::getExModeAttribute(void) const

SbBool
ScXMLScxmlElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) return FALSE;

  this->setInitialAttribute(this->getXMLAttribute("initial"));
  if (!this->initial) {
    // Feb. 2007 Draft compatibility
    this->setInitialAttribute(this->getXMLAttribute("initialstate"));
  }
  this->setNameAttribute(this->getXMLAttribute("name"));
  this->setXMLNSAttribute(this->getXMLAttribute("xmlns"));
  this->setVersionAttribute(this->getXMLAttribute("version"));
  this->setProfileAttribute(this->getXMLAttribute("profile"));
  this->setExModeAttribute(this->getXMLAttribute("exmode"));

  if (this->version) {
    if (strcmp(this->version, "1.0") == 0) {
      // FIXME: implement proper action
    }
    else {
      SoDebugError::post("ScXMLScxmlElt::handleXMLAttributes",
                         "Required attribute 'version' contains "
                         "illegal value '%s'.", this->version);
    }
  }
  else {
    SoDebugError::post("ScXMLScxmlElt::handleXMLAttributes",
                       "Required attribute 'version' is missing.");
  }

  if (this->profile) {
    if (strcmp(this->profile, "minimum") == 0) {
      // FIXME: implement proper action
    }
    else if (strcmp(this->profile, "x-coin") == 0) {
      // FIXME: implement proper action
    }
    else if (strcmp(this->profile, "ecmascript") == 0) {
      SoDebugError::postInfo("ScXMLScxmlElt::handleXMLAttributes",
                             "SCXML profile '%s' not supported yet. "
                             "Ignoring - using 'minimum'.", this->profile);
      this->setProfileAttribute("minimum");
    }
    else if (strcmp(this->profile, "xpath") == 0) {
      SoDebugError::postInfo("ScXMLScxmlElt::handleXMLAttributes",
                             "SCXML profile '%s' not supported yet. "
                             "Ignoring - using 'minimum'.", this->profile);
      this->setProfileAttribute("minimum");
    }
    else {
      SoDebugError::postInfo("ScXMLScxmlElt::handleXMLAttributes",
                             "Unknown SCXML profile '%s'. "
                             "Ignoring - using 'minimum'.", this->profile);
      this->setProfileAttribute("minimum");
    }
  }

  if (this->exmode) {
    if (strcmp(this->exmode, "lax") == 0 ||
        strcmp(this->exmode, "") == 0) {
      // FIXME: implement proper action
    }
    else if (strcmp(this->exmode, "strict") == 0) {
      // FIXME: implement proper action
    }
    else {
      SoDebugError::postInfo("ScXMLScxmlElt::handleXMLAttributes",
                             "SCXML attribute 'exmode' must be either 'lax' "
                             "or 'strict' - contains '%s'. "
                             "Ignoring - using 'lax'.", this->exmode);
      this->setExModeAttribute("lax");
    }
  }

#if 0
  if (!this->referenced && this->initial == NULL) {
    // requirement for root document, but not for referenced documents
    return FALSE;
  }
#endif

  return TRUE;
}

void
ScXMLScxmlElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLScxmlElt * orig = coin_assert_cast<const ScXMLScxmlElt *>(rhs);
  this->setNameAttribute(orig->getNameAttribute());
  this->setInitialAttribute(orig->getInitialAttribute());
  this->setXMLNSAttribute(orig->getXMLNSAttribute());
  this->setVersionAttribute(orig->getVersionAttribute());
  this->setProfileAttribute(orig->getProfileAttribute());
  this->setExModeAttribute(orig->getExModeAttribute());
  int c;
  if (orig->getInitial()) {
    ScXMLInitialElt * initial =
      coin_assert_cast<ScXMLInitialElt *>(orig->getInitial()->clone());
    this->setInitial(initial);
  }
  for (c = 0; c < orig->getNumStates(); ++c) {
    ScXMLStateElt * state =
      coin_assert_cast<ScXMLStateElt *>(orig->getState(c)->clone());
    this->addState(state);
  }
  for (c = 0; c < orig->getNumParallels(); ++c) {
    ScXMLParallelElt * parallel =
      coin_assert_cast<ScXMLParallelElt *>(orig->getParallel(c)->clone());
    this->addParallel(parallel);
  }
  for (c = 0; c < orig->getNumFinals(); ++c) {
    ScXMLFinalElt * final =
      coin_assert_cast<ScXMLFinalElt *>(orig->getFinal(c)->clone());
    this->addFinal(final);
  }
  if (orig->getDataModel()) {
    ScXMLDataModelElt * datamodel =
      coin_assert_cast<ScXMLDataModelElt *>(orig->getDataModel()->clone());
    this->setDataModel(datamodel);
  }
  for (c = 0; c < orig->getNumScripts(); ++c) {
    ScXMLScriptElt * script =
      coin_assert_cast<ScXMLScriptElt *>(orig->getScript(c)->clone());
    this->addScript(script);
  }
}

const ScXMLElt *
ScXMLScxmlElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "name") == 0) {
    if (this->name && strcmp(attrvalue, this->name) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "xmlns") == 0) {
    if (this->xmlns && strcmp(attrvalue, this->xmlns) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "version") == 0) {
    if (this->version && strcmp(attrvalue, this->version) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "profile") == 0) {
    if (this->profile && strcmp(attrvalue, this->profile) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "exmode") == 0) {
    if (this->exmode && strcmp(attrvalue, this->exmode) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "initial") == 0) {
    if (this->initial && strcmp(attrvalue, this->initial) == 0) {
      return this;
    }
  }
  {
    std::vector<ScXMLStateElt *>::const_iterator it = PRIVATE(this)->statelist.begin();
    while (it != PRIVATE(this)->statelist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLParallelElt *>::const_iterator it = PRIVATE(this)->parallellist.begin();
    while (it != PRIVATE(this)->parallellist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLFinalElt *>::const_iterator it = PRIVATE(this)->finallist.begin();
    while (it != PRIVATE(this)->finallist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  if (PRIVATE(this)->datamodelelt.get()) {
    hit = PRIVATE(this)->datamodelelt->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }

  return NULL;
}

void
ScXMLScxmlElt::execute(ScXMLStateMachine * statemachine) const
{
  std::vector<ScXMLScriptElt *>::const_iterator it =
    PRIVATE(this)->scriptlist.begin();
  while (it != PRIVATE(this)->scriptlist.end()) {
    (*it)->execute(statemachine);
    ++it;
  }
}

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLInitialElt, PRIVATE(this)->initialelt, Initial);

SCXML_LIST_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLStateElt, PRIVATE(this)->statelist, State, States);

SCXML_LIST_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLParallelElt, PRIVATE(this)->parallellist, Parallel, Parallels);

SCXML_LIST_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLFinalElt, PRIVATE(this)->finallist, Final, Finals);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLDataModelElt, PRIVATE(this)->datamodelelt, DataModel);

SCXML_LIST_OBJECT_API_IMPL(ScXMLScxmlElt, ScXMLScriptElt, PRIVATE(this)->scriptlist, Script, Scripts);

#undef PRIVATE
