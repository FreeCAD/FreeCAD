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

#include <Inventor/scxml/ScXMLAssignElt.h>

/*!
  \class ScXMLAssignElt ScXMLAssignElt.h Inventor/scxml/ScXMLAssignElt.h
  \brief the &lt;assign&gt; SCXML element.

  The \c &lt;assign&gt; element is only functional under a profile that
  has an implementation of the Data Module.  The \c "minimum" profile does
  not implement one, but the \c "x-coin" profile does.
  
  The following attributes are accepted:
  \li \c location
  \li \c dataID
  \li \c expr

  Either \c location or \c dataID can be specified at a time.

  If \c dataID is specified, then the target of the assignment is the
  \c &lt;data&gt; element that has the \c ID attribute set to the same
  value.

  If \c location is specified, a profile-dependent variable addressing is
  used.  The \c "minimum" profile does not define any, and the \c "x-coin"
  profile defines the following address schemes for writable variables:
  \li \c _data.{id} means targeting an existing \c &lt;data&gt; variable.
  \li \c coin:temp.{id} means targeting a temporary variable, creating one if necessary.

  Temporary variables in the \c "x-coin" profile have the lifetime of
  one SCXML event.  To have persistent data, use \c &lt;data&gt; variables.
  They stay alive for the same duration the ScXMLStateMachine exists.

  Temporary variables are particularly useful for the \c &lt;send&gt; element,
  because the \c namelist attribute can address them with implicit prefix,
  which means the event targets can read them out based on logical names
  instead of SCXML data model addresses.

  \ingroup coin_scxml
  \sa ScXMLDataModelElt, ScXMLDataElt, ScXMLSendElt
*/

#include <cassert>
#include <cstring>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLEvaluator.h>

#include "coindefs.h"
#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLAssignEltReader : public ScXMLEltReader {
public:
  ScXMLAssignEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLAssignEltReader::ScXMLAssignEltReader(void)
: ScXMLEltReader("assign")
{
}

ScXMLElt *
ScXMLAssignEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLAssignElt * assign = new ScXMLAssignElt;
  assign->setContainer(container);
  this->setXMLAttributes(assign, xmlelt);

  // handle XML attributes
  if (unlikely(!assign->handleXMLAttributes())) {
    delete assign;
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

    SoDebugError::post("ScXMLAssignEltReader::read",
                       "unexpected XML element '<%s>' found in <assign>",
                       elementtype);
    delete assign;
    return NULL;
  }

  return assign;
}

// *************************************************************************

class ScXMLAssignElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_SOURCE(ScXMLAssignElt);

/*!
  Creates the ScXMLAssignElt class type SoType object.
*/
void
ScXMLAssignElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLAssignElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLAssignElt, "assign", ScXMLAssignEltReader);
}

/*!
  Cleans out the class type SoType object.
*/
void
ScXMLAssignElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLAssignElt);
  ScXMLAssignElt::classTypeId = SoType::badType();
}

/*!
  Constructor that sets up an empty ScXMLAssignElt object.
*/
ScXMLAssignElt::ScXMLAssignElt(void)
: location(NULL),
  dataID(NULL),
  expr(NULL)
{
}

ScXMLAssignElt::~ScXMLAssignElt(void)
{
  this->setLocationAttribute(NULL);
  this->setDataIDAttribute(NULL);
  this->setExprAttribute(NULL);
}

void
ScXMLAssignElt::setLocationAttribute(const char * locationstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->location, "location", locationstr);
}

// const char * getLocationAttribute(void) const { return this->location; }

void
ScXMLAssignElt::setDataIDAttribute(const char * dataIDstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->dataID, "dataID", dataIDstr);
}

// const char * getDataIDAttribute(void) const { return this->dataID; }

void
ScXMLAssignElt::setExprAttribute(const char * exprstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->expr, "expr", exprstr);
}

// const char * getExprAttribute(void) const { return this->expr; }

SbBool
ScXMLAssignElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setLocationAttribute(this->getXMLAttribute("location"));
  this->setDataIDAttribute(this->getXMLAttribute("dataID"));
  this->setExprAttribute(this->getXMLAttribute("expr"));

  return TRUE;
}

void
ScXMLAssignElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLAssignElt * orig = coin_assert_cast<const ScXMLAssignElt *>(rhs);
  this->setLocationAttribute(orig->getLocationAttribute());
  this->setDataIDAttribute(orig->getDataIDAttribute());
  this->setExprAttribute(orig->getExprAttribute());
}

const ScXMLElt *
ScXMLAssignElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "location") == 0) {
    if (this->location && strcmp(attrvalue, this->location) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "dataID") == 0) {
    if (this->dataID && strcmp(attrvalue, this->dataID) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "expr") == 0) {
    if (this->expr && strcmp(attrvalue, this->expr) == 0) {
      return this;
    }
  }
  return NULL;
}

void
ScXMLAssignElt::execute(ScXMLStateMachine * statemachine) const
{
  assert(statemachine);
  ScXMLEvaluator * evaluator = statemachine->getEvaluator();
  assert(evaluator);

  //printf("assign: '%s'\n", this->getExprAttribute());
  std::unique_ptr<ScXMLDataObj> dataobj(evaluator->evaluate(this->getExprAttribute()));
  if (dataobj.get()) {
    ScXMLDataObj * result = dataobj.get(); // default if not an expression
    if (dataobj->isOfType(ScXMLExprDataObj::getClassTypeId())) {
      ScXMLExprDataObj * exprobj = static_cast<ScXMLExprDataObj *>(dataobj.get());
      ScXMLDataObj * evaled = exprobj->evaluate(statemachine);
      if (!evaled) {
        statemachine->queueInternalEvent("error.InvalidExpr.Assign");
        //printf("expr = '%s'\n", this->getExprAttribute());
        return;
      }
      if (!evaled->isOfType(ScXMLConstantDataObj::getClassTypeId())) {
        statemachine->queueInternalEvent("error.InvalidExpr.Assign");
        //printf("expr = '%s'\n", this->getExprAttribute());
        return;
      }
      result = evaled;
    }
    SbString strval;
    if (result->isOfType(ScXMLRealDataObj::getClassTypeId())) {
      static_cast<ScXMLRealDataObj *>(result)->convertToString(strval);
    }
    else if (result->isOfType(ScXMLBoolDataObj::getClassTypeId())) {
      static_cast<ScXMLBoolDataObj *>(result)->convertToString(strval);
    }
    else if (result->isOfType(ScXMLStringDataObj::getClassTypeId())) {
      static_cast<ScXMLStringDataObj *>(result)->convertToString(strval);
    }
    //printf("result: '%s'\n", strval.getString());
    SbString loc;
    if (this->getDataIDAttribute() && this->getDataIDAttribute()[0] != '\0') {
      loc.sprintf("_data.%s", this->getDataIDAttribute());
    } else {
      loc = this->getLocationAttribute();
    }
    if (!evaluator->setAtLocation(loc.getString(), result)) {
      statemachine->queueInternalEvent("error.InvalidLocation.Assign");
      return;
    }
  }
  else {
    statemachine->queueInternalEvent("error.InvalidExpr.Assign");
    //printf("expr = '%s'\n", this->getExprAttribute());
    return;
  }
}

#undef PRIVATE
