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

#ifdef _MSC_VER
#pragma warning(disable:4786) // symbol truncated
#endif // _MSC_VER

#include <cassert>
#include <cstring>

#include <Inventor/C/XML/document.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/C/XML/attribute.h>

#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLDocument.h>

#include "threads/threadsutilp.h"

#include "scxml/ScXMLP.h"

#define SCXML_DEFAULT_NS "default" // to be deprecated

// private static variables
ScXMLP::NamespaceDict * ScXMLP::namespaces = NULL;
ScXMLP::TargettypeDict * ScXMLP::targettypes = NULL;
ScXMLP::TypeDict * ScXMLP::profileevaluators = NULL;

// *************************************************************************

// *************************************************************************
// TYPE SYSTEM REGISTERING AND LOOKUP
// *************************************************************************

/*
*/
void
ScXMLP::registerClassType(const char * xmlns, const char * classname, SoType type)
{
  assert(!type.isBad());
  ScXMLP::registerClassType(ScXMLP::namespaces, xmlns, classname, type);
}

/*
*/
void
ScXMLP::registerInvokeClassType(const char * xmlns, const char * targettype, const char * source, SoType type)
{
  assert(!type.isBad());
  assert(xmlns);

  // Note that we actually split on targettype on the top of the
  // hierarchy/tree here, and not on xml-namespace as one might think
  // at first, but this is intentional. This is because the namespace
  // logic should fall through to the default namespace if you don't
  // find the type you are looking for in the specified namespace, and
  // because of that, it is most convenient to split on namespace at
  // the next-to-bottom level to be able to share code when looking up
  // types.

  ScXMLP::NamespaceDict * nsdict = ScXMLP::getNamespaceDict(ScXMLP::targettypes, targettype);
  assert(nsdict);

  ScXMLP::registerClassType(nsdict, xmlns, source, type);
}

/*
*/
void
ScXMLP::registerClassType(NamespaceDict * nsdict, const char * xmlns, const char * classname, SoType type)
{
  assert(xmlns);

  TypeDict * typedict = ScXMLP::getTypeDict(nsdict, xmlns);
  assert(typedict);

  SbName classnamename(classname);
  const char * key = classnamename.getString();
  TypeDict::iterator it = typedict->find(key);
  if (it != typedict->end()) {
    it->second = type;
  } else {
    typedict->insert(TypeEntry(key, type));
  }
}

/*
*/
ScXMLP::TypeDict *
ScXMLP::getTypeDict(NamespaceDict * nsdict, const char * xmlns)
{
  SbName xmlnsname(xmlns); // uniqify on string pointer
  const char * key = xmlnsname.getString();
  NamespaceDict::iterator it = nsdict->find(key);
  if (it != nsdict->end()) {
    return it->second;
  } else {
    TypeDict * dict = new TypeDict;
    nsdict->insert(NamespaceEntry(key, dict));
    return dict;
  }
}

// *************************************************************************
/*
*/
SoType
ScXMLP::getClassType(NamespaceDict * nsdict, const char * xmlns, const char * classname)
{
  assert(xmlns && classname);

  const SbName classnamename(classname);
  const char * key = classnamename.getString();

  ScXMLP::TypeDict * typedict = NULL;

  typedict = ScXMLP::getTypeDict(nsdict, xmlns);
  if (typedict) {
    TypeDict::iterator it = typedict->find(key);
    if (it != typedict->end()) {
      return it->second;
    }
  }

  typedict = ScXMLP::getTypeDict(nsdict, SCXML_DEFAULT_NS);
  if (typedict) {
    TypeDict::iterator it = typedict->find(key);
    if (it != typedict->end()) {
      return it->second;
    }
  }

  return SoType::badType();
}

/*
*/
ScXMLP::NamespaceDict *
ScXMLP::getNamespaceDict(TargettypeDict * ttdict, const char * targettype)
{
  SbName targettypename(targettype); // uniqify on string pointer
  const char * key = targettypename.getString();
  TargettypeDict::iterator it = ttdict->find(key);
  if (it != ttdict->end()) {
    return it->second;
  } else {
    NamespaceDict * dict = new NamespaceDict;
    ttdict->insert(TargettypeEntry(key, dict));
    return dict;
  }
}

/*
*/
SoType
ScXMLP::getClassType(const char * xmlns, const char * classname)
{
  return ScXMLP::getClassType(ScXMLP::namespaces, xmlns, classname);
}

SoType
ScXMLP::getInvokeClassType(const char * xmlns, const char * targettype, const char * source)
{
  NamespaceDict * nsdict = ScXMLP::getNamespaceDict(ScXMLP::targettypes, targettype);
  assert(nsdict);

  SoType invoketype = ScXMLP::getClassType(nsdict, xmlns, source);

#if 0
  if (invoketype == SoType::badType()) {
    return ScXMLFallbackInvoke::getClassTypeId();
  }
#endif

  return invoketype;
}

// *************************************************************************

// internal implementation
ScXMLStateMachine *
ScXMLP::readXMLData(cc_xml_doc * doc)
{
  const char * xmlns = SCXML_DEFAULT_NS;

  // get a handle on the root element
  cc_xml_elt * root = cc_xml_doc_get_root(doc);
  if (strcmp(cc_xml_elt_get_type(root), "scxml") != 0) {
    return NULL;
  }

  // peek at xmlns attribute to get the correct object types instantiated
  const cc_xml_attr * nsattr = cc_xml_elt_get_attribute(root, "xmlns");
  if (nsattr) {
    xmlns = cc_xml_attr_get_value(nsattr);
    assert(xmlns);
  }

  SoType statemachinetype = ScXMLP::getClassType(xmlns, "statemachine");
  assert(statemachinetype.canCreateInstance());
  ScXMLStateMachine * statemachine =
    static_cast<ScXMLStateMachine *>(statemachinetype.createInstance());
  assert(statemachine && statemachine->isOfType(ScXMLStateMachine::getClassTypeId()));

  // FIXME
  ScXMLObject * documentobj = NULL;
  // ScXMLObject * documentobj = ScXMLP::readScXMLDocument(statemachine, root, xmlns);
  assert(documentobj && documentobj->isOfType(ScXMLDocument::getClassTypeId()));
  ScXMLDocument * document = static_cast<ScXMLDocument *>(documentobj);

  statemachine->setDescription(document);

  return statemachine;
}

// *************************************************************************

void * ScXMLP::mutex = NULL;

void
ScXMLP::init(void)
{
  ScXMLP::namespaces = new ScXMLP::NamespaceDict;
  ScXMLP::targettypes = new ScXMLP::TargettypeDict;
  ScXMLP::profileevaluators = new ScXMLP::TypeDict;
  CC_MUTEX_CONSTRUCT(ScXMLP::mutex);
}

void
ScXMLP::cleanup(void)
{
  cleanup_namespacedict(ScXMLP::namespaces);
  delete ScXMLP::namespaces;
  ScXMLP::namespaces = NULL;
  delete ScXMLP::profileevaluators;
  ScXMLP::profileevaluators = NULL;
  cleanup_targettypes();
  CC_MUTEX_DESTRUCT(ScXMLP::mutex);
}

void
ScXMLP::lock(void)
{
  assert(ScXMLP::mutex);
  CC_MUTEX_LOCK(ScXMLP::mutex);
}

void
ScXMLP::unlock(void)
{
  assert(ScXMLP::mutex);
  CC_MUTEX_UNLOCK(ScXMLP::mutex);
}


void
ScXMLP::cleanup_namespacedict(NamespaceDict * dict)
{
  assert(dict);
  NamespaceDict::iterator it = dict->begin();
  while (it != dict->end()) {
    delete it->second;
    ++it;
  }
}

void
ScXMLP::cleanup_targettypes(void)
{
  assert(ScXMLP::targettypes);
  TargettypeDict::iterator it = ScXMLP::targettypes->begin();
  while (it != ScXMLP::targettypes->end()) {
    ScXMLP::NamespaceDict * dict = it->second;
    cleanup_namespacedict(dict);
    delete dict;
    ++it;
  }
  delete ScXMLP::targettypes;
  ScXMLP::targettypes = NULL;
}

// *************************************************************************
