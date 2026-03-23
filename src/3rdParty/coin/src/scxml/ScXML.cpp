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

#include <Inventor/scxml/ScXML.h>

/*!
  \class ScXML ScXML.h Inventor/scxml/ScXML.h
  \brief namespace for static ScXML-related functions.

  This is a static namespace class for ScXML-related functions.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#ifdef _MSC_VER
#pragma warning(disable:4786) // symbol truncated
#endif // _MSC_VER

#include <Inventor/SbName.h>
#include <Inventor/errors/SoDebugError.h>

#include <Inventor/misc/CoinResources.h>

#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLObject.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/ScXMLEventTarget.h>
#include <Inventor/scxml/ScXMLDocument.h>

#include <Inventor/scxml/ScXMLElt.h>
#include <Inventor/scxml/ScXMLScxmlElt.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLAbstractStateElt.h>
#include <Inventor/scxml/ScXMLStateElt.h>
#include <Inventor/scxml/ScXMLParallelElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>
#include <Inventor/scxml/ScXMLTransitionElt.h>
#include <Inventor/scxml/ScXMLHistoryElt.h>

#include <Inventor/scxml/ScXMLExecutableElt.h>
#include <Inventor/scxml/ScXMLEventElt.h>
#include <Inventor/scxml/ScXMLIfElt.h>
#include <Inventor/scxml/ScXMLElseIfElt.h>
#include <Inventor/scxml/ScXMLElseElt.h>
#include <Inventor/scxml/ScXMLLogElt.h>

#include <Inventor/scxml/ScXMLDataModelElt.h>
#include <Inventor/scxml/ScXMLDataElt.h>
#include <Inventor/scxml/ScXMLAssignElt.h>
#include <Inventor/scxml/ScXMLValidateElt.h>

#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLParamElt.h>
#include <Inventor/scxml/ScXMLFinalizeElt.h>
#include <Inventor/scxml/ScXMLContentElt.h>

#include <Inventor/scxml/ScXMLAnchorElt.h>

#include <Inventor/scxml/ScXMLEvaluator.h>
#include <Inventor/scxml/ScXMLMinimumEvaluator.h>
#include <Inventor/scxml/ScXMLXPathEvaluator.h>
#include <Inventor/scxml/ScXMLECMAScriptEvaluator.h>
#include <Inventor/scxml/ScXMLCoinEvaluator.h>

#include <Inventor/scxml/SoScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>

#include "tidbitsp.h"

#include "scxml/ScXMLP.h"

// *************************************************************************

/*!
  \page coin_scxml_page State Chart XML

  The ScXML part of Coin is a basic, non-conformant, partial
  implementation of State Chart XML, based on the W3C Working Draft
  16 May 2008 of SCXML <http://www.w3.org/TR/2008/WD-scxml-20080516/>.
  The latest version should be at <http://www.w3.org/TR/scxml/>.
  Read that document for a basic understanding how SCXML documents
  should be constructed.

  Coin uses SCXML for setting up its navigation systems, to be able to
  remove hardcoded logic for user navigation and externalize it into XML
  files.  This makes it easy for users to customize the navigation
  system for their own purposes.

  Unsupported Items:

  - The &lt;parallel&gt; element is not supported as intended with
    parallel states.  Coin will just treat it as an ordinary
    &lt;state&gt; element for now.  Parallel states is not high up on
    the priority list, so expect this to be handled after a lot of
    other functionality is in place.

  - The &lt;datamodel&gt;-related part of the specification is not
    fully supported.

  - The 'target' attribute in the &lt;transition&gt; element can only
    identify a single state currently, not multiple as you would have
    to when having support for &lt;parallel&gt; elements (which we
    don't have).

  - The virtual state elements like &lt;history&gt; and &lt;anchor&gt;
    are just implemented as dummy states for now and do not do
    anything in relation to what they should actually do.

  For learning more about how ScXML is implemented and used in Coin,
  take a look at $COINDIR/scxml/navigation/examiner.xml (or in the
  Coin source directory, data/scxml/navigation/examiner.xml) for an
  example of how an SCXML system for camera navigation looks,
  and look at the source files in src/navigation/ for the C++ counterparts
  to the same SCXML navigation system.

  \ingroup coin_scxml
  \since Coin 3.0
*/

// *************************************************************************

extern "C" {
static void scxml_cleanup(void);
}

/*!
  Initializes the basic ScXML classes.
*/
void
ScXML::initClasses(void)
{
  ScXMLP::init();
  ScXMLObject::initClass();
  ScXMLEvent::initClass();
  ScXMLEventTarget::initClass();
  ScXMLStateMachine::initClass();
  ScXMLDocument::initClass();

  ScXMLElt::initClass();
  ScXMLScxmlElt::initClass();
  ScXMLInitialElt::initClass();
  ScXMLAbstractStateElt::initClass();
  ScXMLStateElt::initClass();
  ScXMLParallelElt::initClass();
  ScXMLFinalElt::initClass();
  ScXMLOnEntryElt::initClass();
  ScXMLOnExitElt::initClass();
  ScXMLTransitionElt::initClass();
  ScXMLHistoryElt::initClass();

  ScXMLExecutableElt::initClass();
  ScXMLEventElt::initClass();
  ScXMLIfElt::initClass();
  ScXMLElseIfElt::initClass();
  ScXMLElseElt::initClass();
  ScXMLLogElt::initClass();

  ScXMLDataModelElt::initClass();
  ScXMLDataElt::initClass();
  ScXMLAssignElt::initClass();
  ScXMLValidateElt::initClass();

  ScXMLSendElt::initClass();
  ScXMLInvokeElt::initClass();
  ScXMLParamElt::initClass();
  ScXMLFinalizeElt::initClass();
  ScXMLContentElt::initClass();

  ScXMLAnchorElt::initClass();

  // evaluators also inits data-objs
  ScXMLEvaluator::initClass();
  ScXMLMinimumEvaluator::initClass();
  ScXMLXPathEvaluator::initClass();
  ScXMLECMAScriptEvaluator::initClass();
  ScXMLCoinEvaluator::initClass();

  SoScXMLEvent::initClass();
  SoScXMLStateMachine::initClass();

  coin_atexit(scxml_cleanup, CC_ATEXIT_NORMAL);
}

void
scxml_cleanup(void)
{
  ScXML::cleanClasses();
}

void
ScXML::cleanClasses(void)
{
  SoScXMLStateMachine::cleanClass();
  SoScXMLEvent::cleanClass();

  ScXMLCoinEvaluator::cleanClass();
  ScXMLECMAScriptEvaluator::cleanClass();
  ScXMLXPathEvaluator::cleanClass();
  ScXMLMinimumEvaluator::cleanClass();
  ScXMLEvaluator::cleanClass();

  ScXMLAnchorElt::cleanClass();

  ScXMLContentElt::cleanClass();
  ScXMLFinalizeElt::cleanClass();
  ScXMLParamElt::cleanClass();
  ScXMLInvokeElt::cleanClass();
  ScXMLSendElt::cleanClass();

  ScXMLValidateElt::cleanClass();
  ScXMLAssignElt::cleanClass();
  ScXMLDataElt::cleanClass();
  ScXMLDataModelElt::cleanClass();

  ScXMLLogElt::cleanClass();
  ScXMLElseElt::cleanClass();
  ScXMLElseIfElt::cleanClass();
  ScXMLIfElt::cleanClass();
  ScXMLEventElt::cleanClass();
  ScXMLExecutableElt::cleanClass();

  ScXMLHistoryElt::cleanClass();
  ScXMLTransitionElt::cleanClass();
  ScXMLOnExitElt::cleanClass();
  ScXMLOnEntryElt::cleanClass();
  ScXMLFinalElt::cleanClass();
  ScXMLParallelElt::cleanClass();
  ScXMLStateElt::cleanClass();
  ScXMLAbstractStateElt::cleanClass();
  ScXMLInitialElt::cleanClass();
  ScXMLScxmlElt::cleanClass();
  ScXMLElt::cleanClass();

  ScXMLDocument::cleanClass();
  ScXMLStateMachine::cleanClass();
  ScXMLEventTarget::cleanClass();
  ScXMLEvent::cleanClass();
  ScXMLObject::cleanClass();
  ScXMLP::cleanup();
}

// *************************************************************************

SbBool
ScXML::registerEvaluatorType(SbName profilename, SoType evaluatortype)
{
  assert(!evaluatortype.isBad());
  assert(ScXMLP::profileevaluators);
  if (!evaluatortype.isDerivedFrom(ScXMLEvaluator::getClassTypeId())) {
    SoDebugError::post("ScXMLStateMachine::registerEvaluator",
                       "Evaluator type must be derived from ScXMLEvaluator");
    return FALSE;
  }
  ScXMLP::TypeDict::iterator it = ScXMLP::profileevaluators->find(profilename.getString());
  if (it != ScXMLP::profileevaluators->end()) {
    SoDebugError::post("ScXML::registerEvaluatorType",
                       "Evaluator for profile '%s' already registered.\n",
                       profilename.getString());
    return FALSE;
  }
  ScXMLP::TypeEntry entry(profilename.getString(), evaluatortype);
  ScXMLP::profileevaluators->insert(entry);
  return TRUE;
}

SbBool
ScXML::unregisterEvaluatorType(SbName profilename, SoType evaluatortype)
{
  assert(!evaluatortype.isBad());
  assert(ScXMLP::profileevaluators);

  ScXMLP::TypeDict::iterator it = ScXMLP::profileevaluators->find(profilename.getString());
  if (it == ScXMLP::profileevaluators->end()) {
    SoDebugError::post("ScXML::unregisterEvaluatorType",
                       "No evaluator type is registered for profile '%s'.\n",
                       profilename.getString());
    return FALSE;
  }
  if (it->second != evaluatortype) {
    SoDebugError::post("ScXML::unregisterEvaluatorType",
                       "Different evaluator type registered for profile '%s'.\n",
                       profilename.getString());
    return FALSE;
  }
  ScXMLP::profileevaluators->erase(it);
  return TRUE;
}

SoType
ScXML::getEvaluatorTypeForProfile(SbName profilename)
{
  assert(ScXMLP::profileevaluators);

  ScXMLP::TypeDict::iterator it = ScXMLP::profileevaluators->find(profilename.getString());
  if (it == ScXMLP::profileevaluators->end()) {
    // no type registered for profile
    return SoType::badType();
  }
  return it->second;
}

// *************************************************************************
// SCXML FILE INPUT
// *************************************************************************

/*!
  This function reads in an SCXML document from a file.

  \returns an ScXMLDocument-derived state machine object hierarchy.
*/
ScXMLStateMachine *
ScXML::readFile(const char * filename)
{
  ScXMLDocument * doc = ScXMLDocument::readFile(filename);
  if (!doc) {
    return NULL;
  }

  ScXMLStateMachine * statemachine = new SoScXMLStateMachine;
  statemachine->setDescription(doc);
  statemachine->setName(filename);

  return statemachine;
}

/*!
  This function reads in an SCXML document residing in memory.

  \returns an ScXMLDocument-derived state machine object hierarchy.
*/
ScXMLStateMachine *
ScXML::readBuffer(const SbByteBuffer & bufferdata)
{
  ScXMLDocument * doc = ScXMLDocument::readBuffer(bufferdata);
  if (!doc) {
    return NULL;
  }

  ScXMLStateMachine * statemachine = new SoScXMLStateMachine;
  statemachine->setDescription(doc);
  statemachine->setName("<memory buffer>");

  return statemachine;
}

// *************************************************************************
