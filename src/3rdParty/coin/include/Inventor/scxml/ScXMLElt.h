#ifndef COIN_SCXMLELT_H
#define COIN_SCXMLELT_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SoType.h>
#include <Inventor/tools/SbPimplPtr.h>
#include <Inventor/scxml/ScXMLObject.h>
#include <Inventor/scxml/ScXMLSubObject.h>

class ScXMLStateMachine;

class COIN_DLL_API ScXMLElt : public ScXMLObject {
  typedef ScXMLObject inherited;
  SCXML_ELEMENT_ABSTRACT_HEADER(ScXMLElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLElt(void);
  virtual ~ScXMLElt(void);

  void setXMLAttribute(const char * attribute, const char * value);
  const char * getXMLAttribute(const char * attribute) const;

  virtual SbBool handleXMLAttributes(void);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  ScXMLElt * clone(void) const;
  virtual void copyContents(const ScXMLElt * rhs);

  virtual void setContainer(ScXMLElt * container);
  ScXMLElt * getContainer(void) const { return this->containerptr; }

  SbBool isContainedIn(const ScXMLElt * object) const;

protected:
  static void registerClassType(const char * xmlns,
                                const char * classname, SoType type);
  static void registerInvokeClassType(const char * xmlns,
                                      const char * targettype,
                                      const char * source, SoType type);

private:
  ScXMLElt * containerptr;

private:
  ScXMLElt(const ScXMLElt & rhs); // N/A
  ScXMLElt & operator = (const ScXMLElt & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLElt

// *************************************************************************

class ScXMLDocument;
class ScXMLStateMachine;
struct cc_xml_elt;

class COIN_DLL_API ScXMLEltReader {
public:
  ScXMLEltReader(const char * eltname);
  virtual ~ScXMLEltReader(void);

  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * elt, 
                          ScXMLDocument * doc, ScXMLStateMachine * sm) = 0;

protected:
  const char * element;

  void setXMLAttributes(ScXMLElt * scxmlelt, cc_xml_elt * xmlelt);

}; // ScXMLEltReader

// *************************************************************************

#endif // !COIN_SCXMLELT_H
