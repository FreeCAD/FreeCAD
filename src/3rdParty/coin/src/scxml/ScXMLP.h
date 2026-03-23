#ifndef COIN_SCXMLP_H
#define COIN_SCXMLP_H

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

#include <map>

#include <Inventor/SoType.h>

struct cc_xml_doc;
struct cc_xml_elt;
class ScXMLObject;
class ScXMLElt;

class ScXMLP {
public:
  static void init(void);
  static void cleanup(void);

  static void lock(void);
  static void unlock(void);

  static void registerClassType(const char * xmlns,
                                const char * classname, SoType type);

  static void registerInvokeClassType(const char * xmlns,
                                      const char * targettype,
                                      const char * source, SoType type);

  static ScXMLStateMachine * readXMLData(cc_xml_doc * doc);

  typedef std::map<const char *, SoType> TypeDict;
  typedef std::pair<const char *, SoType> TypeEntry;

protected:
  typedef std::map<const char *, TypeDict *> NamespaceDict;
  typedef std::pair<const char *, TypeDict *> NamespaceEntry;
  typedef std::map<const char *, NamespaceDict *> TargettypeDict;
  typedef std::pair<const char *, NamespaceDict *> TargettypeEntry;

  static NamespaceDict * namespaces;
  static TargettypeDict * targettypes;

  // toplevel getters
  static SoType getClassType(const char * xmlns, const char * classname);
  static SoType getInvokeClassType(const char * xmlns, const char * targettype,
                                   const char * source);

  // utilities
  static void registerClassType(NamespaceDict * nsdict, const char * xmlns,
                                const char * classname, SoType type);

  static NamespaceDict * getNamespaceDict(TargettypeDict * ttdict, const char * targettype);

  static TypeDict * getTypeDict(NamespaceDict * dict, const char * xmlns);

  static SoType getClassType(NamespaceDict * dict, const char * xmlns,
                             const char * classname);

public:
  static TypeDict * profileevaluators;

private:
  static void cleanup_namespacedict(NamespaceDict * dict);
  static void cleanup_targettypes(void);

  static void * mutex;
}; // ScXMLP

#endif // !COIN_SCXMLP_H
