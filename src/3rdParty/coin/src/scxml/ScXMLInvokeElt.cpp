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

#include <Inventor/scxml/ScXMLInvokeElt.h>

/*!
  \class ScXMLInvokeElt ScXMLInvokeElt.h Inventor/scxml/ScXMLInvokeElt.h
  \brief implements the &lt;invoke&gt; SCXML element.

  Important note for SCXML-users on the Coin 3.0 implementation of the
  &lt;invoke&gt; element: The first SCXML-implementation of Coin was based on
  a draft from February 2007, and the interpretation done of the &lt;invoke&gt;
  element was wrong in certain aspects that has become clear in later revisions
  of the standards draft. The SCXML-implementation was for various reasons
  rushed, with a timeframe where no scripting capabilities would be possible to
  get ready, and the navigation system code (the primary purpose of implementing
  the SCXML system) had to be plugged in somewhere. This was at that time done
  on the &lt;invoke&gt; element.

  For the current implementation, the navigation code has been moved away from
  being &lt;invoke&gt;-based to being a service communicated to through the
  &lt;send&gt; element to be more standards-conformant. And to make the situation
  even more ridiculous, that approach will likely be deprecated in the
  not-too-distant future, to be replaced by a pure, inlined, scripting-based
  approach. Those two approaches should however be fully possible to keep alive
  side-by-side for a while, so following the current lead in implementing
  your own custom navigation systems should be a lot safer than doing it over
  the misguided &lt;invoke&gt;-approach.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <vector>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXMLParamElt.h>
#include <Inventor/scxml/ScXMLFinalizeElt.h>
#include <Inventor/scxml/ScXMLContentElt.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLInvokeEltReader : public ScXMLEltReader {
public:
  ScXMLInvokeEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLInvokeEltReader::ScXMLInvokeEltReader(void)
: ScXMLEltReader("invoke")
{
}

ScXMLElt *
ScXMLInvokeEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLInvokeElt * invoke = new ScXMLInvokeElt;
  invoke->setContainer(container);
  this->setXMLAttributes(invoke, xmlelt);

  // handle XML attributes
  if (unlikely(!invoke->handleXMLAttributes())) {
    delete invoke;
    return NULL;
  }

  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    else if (strcmp(elementtype, "param") == 0) {
      // <param> - zero or many elements
      ScXMLEltReader * paramreader = ScXMLParamElt::getElementReader();
      assert(paramreader);
      ScXMLElt * paramobj = paramreader->read(invoke, element, doc, sm);
      if (unlikely(!paramobj)) {
        delete invoke;
        return NULL;
      }

      assert(paramobj->isOfType(ScXMLParamElt::getClassTypeId()));
      invoke->addParam(static_cast<ScXMLParamElt *>(paramobj));
    }

    else if (strcmp(elementtype, "finalize") == 0) {
      // <finalize> - zero or one element
      if (unlikely(invoke->getFinalize())) {
        SoDebugError::post("ScXMLInvokeEltReader::read",
                           "<invoke> elements can only have one <finalize> child");
        delete invoke;
        return NULL;
      }

      ScXMLEltReader * finalizereader = ScXMLFinalizeElt::getElementReader();
      assert(finalizereader);
      ScXMLElt * finalizeobj = finalizereader->read(invoke, element, doc, sm);
      if (unlikely(!finalizeobj)) {
        delete invoke;
        return NULL;
      }

      assert(finalizeobj->isOfType(ScXMLFinalizeElt::getClassTypeId()));
      invoke->setFinalize(static_cast<ScXMLFinalizeElt *>(finalizeobj));
    }

    else if (strcmp(elementtype, "content") == 0) {
      // <content> - zero or one element
      if (unlikely(invoke->getContent())) {
        SoDebugError::post("ScXMLInvokeEltReader::read",
                           "<invoke> elements can only have one <content> child");
        delete invoke;
        return NULL;
      }

      ScXMLEltReader * contentreader = ScXMLContentElt::getElementReader();
      assert(contentreader);
      ScXMLElt * contentobj = contentreader->read(invoke, element, doc, sm);
      if (unlikely(!contentobj)) {
        delete invoke;
        return NULL;
      }

      assert(contentobj->isOfType(ScXMLContentElt::getClassTypeId()));
      invoke->setContent(static_cast<ScXMLContentElt *>(contentobj));
    }


    SoDebugError::post("ScXMLInvokeEltReader::read",
                       "<invoke> contains unexpected <%s> element", elementtype);
    delete invoke;
    return NULL;
   }


  return invoke;
}

// *************************************************************************

class ScXMLInvokeElt::PImpl {
public:
  PImpl(void) { }
  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->paramlist, ScXMLParamElt *);
  }

  std::vector<ScXMLParamElt *> paramlist;
  std::unique_ptr<ScXMLFinalizeElt> finalizeptr;
  std::unique_ptr<ScXMLContentElt> contentptr;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLInvokeElt);

void
ScXMLInvokeElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLInvokeElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLInvokeElt, "invoke", ScXMLInvokeEltReader);
}

void
ScXMLInvokeElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLInvokeElt);
  ScXMLInvokeElt::classTypeId = SoType::badType();
}

ScXMLInvokeElt::ScXMLInvokeElt(void)
: targettype(NULL),
  src(NULL),
  srcexpr(NULL)
{
}

ScXMLInvokeElt::~ScXMLInvokeElt(void)
{
  this->setTargetTypeAttribute(NULL);
  this->setSrcAttribute(NULL);
  this->setSrcExprAttribute(NULL);
}

void
ScXMLInvokeElt::setTargetTypeAttribute(const char * targettypestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->targettype, "targettype", targettypestr);
}

// const char * ScXMLInvokeElt::getTargetTypeAttribute(void) const

void
ScXMLInvokeElt::setSrcAttribute(const char * srcstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->src, "src", srcstr);
}

// const char * ScXMLInvokeElt::getSrcAttribute(void) const

void
ScXMLInvokeElt::setSrcExprAttribute(const char * srcexprstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->srcexpr, "srcexpr", srcexprstr);
}

// const char * ScXMLInvokeElt::getSrcExprAttribute(void) const

SbBool
ScXMLInvokeElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setTargetTypeAttribute(this->getXMLAttribute("targettype"));
  this->setSrcAttribute(this->getXMLAttribute("src"));
  this->setSrcExprAttribute(this->getXMLAttribute("srcexpr"));

  if (!this->targettype) {
    return FALSE;
  }

  return TRUE;
}

void
ScXMLInvokeElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLInvokeElt * orig = coin_assert_cast<const ScXMLInvokeElt *>(rhs);
  this->setTargetTypeAttribute(orig->getTargetTypeAttribute());
  this->setSrcAttribute(orig->getSrcAttribute());
  this->setSrcExprAttribute(orig->getSrcExprAttribute());
  if (orig->getContent()) {
    ScXMLContentElt * content =
      coin_assert_cast<ScXMLContentElt *>(orig->getContent()->clone());
    this->setContent(content);
  }
  if (orig->getFinalize()) {
    ScXMLFinalizeElt * finalize =
      coin_assert_cast<ScXMLFinalizeElt *>(orig->getFinalize()->clone());
    this->setFinalize(finalize);
  }
  //int c;
  for (int c = 0; c < orig->getNumParams(); ++c) {
    ScXMLParamElt * param =
      coin_assert_cast<ScXMLParamElt *>(orig->getParam(c)->clone());
    this->addParam(param);
  }
}

const ScXMLElt *
ScXMLInvokeElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "targettype") == 0) {
    if (this->targettype && strcmp(attrvalue, this->targettype) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "src") == 0) {
    if (this->src && strcmp(attrvalue, this->src) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "srcexpr") == 0) {
    if (this->srcexpr && strcmp(attrvalue, this->srcexpr) == 0) {
      return this;
    }
  }

  {
    std::vector<ScXMLParamElt *>::const_iterator it = PRIVATE(this)->paramlist.begin();
    while (it != PRIVATE(this)->paramlist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }

  if (PRIVATE(this)->finalizeptr.get()) {
    hit = PRIVATE(this)->finalizeptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }

  if (PRIVATE(this)->contentptr.get()) {
    hit = PRIVATE(this)->contentptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }

  return NULL;
}

SCXML_LIST_OBJECT_API_IMPL(ScXMLInvokeElt, ScXMLParamElt, PRIVATE(this)->paramlist, Param, Params);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLInvokeElt, ScXMLFinalizeElt, PRIVATE(this)->finalizeptr, Finalize);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLInvokeElt, ScXMLContentElt, PRIVATE(this)->contentptr, Content);

#undef PRIVATE
