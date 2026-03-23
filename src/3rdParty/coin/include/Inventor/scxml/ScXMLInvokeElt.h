#ifndef COIN_SCXMLINVOKEELT_H
#define COIN_SCXMLINVOKEELT_H

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

#include <Inventor/scxml/ScXMLExecutableElt.h>

#include <Inventor/tools/SbLazyPimplPtr.h>

class ScXMLEvent;
class ScXMLStateMachine;
class ScXMLParamElt;
class ScXMLFinalizeElt;
class ScXMLContentElt;

class COIN_DLL_API ScXMLInvokeElt : public ScXMLExecutableElt {
  typedef ScXMLExecutableElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLInvokeElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLInvokeElt(void);
  virtual ~ScXMLInvokeElt(void);

  // XML attributes
  virtual void setTargetTypeAttribute(const char * id);
  const char * getTargetTypeAttribute(void) const { return this->targettype; }

  virtual void setSrcAttribute(const char * id);
  const char * getSrcAttribute(void) const { return this->src; }

  virtual void setSrcExprAttribute(const char * id);
  const char * getSrcExprAttribute(void) const { return this->srcexpr; }

  virtual SbBool handleXMLAttributes(void);

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  virtual int getNumParams(void) const;
  virtual ScXMLParamElt * getParam(int idx) const;
  virtual void addParam(ScXMLParamElt * param);
  virtual void removeParam(ScXMLParamElt * param);
  virtual void clearAllParams(void);

  virtual void setFinalize(ScXMLFinalizeElt * finalize);
  virtual ScXMLFinalizeElt * getFinalize(void) const;

  virtual void setContent(ScXMLContentElt * content);
  virtual ScXMLContentElt * getContent(void) const;

protected:
  char * targettype;
  char * src;
  char * srcexpr;

private:
  ScXMLInvokeElt(const ScXMLInvokeElt & rhs); // N/A
  ScXMLInvokeElt & operator = (const ScXMLInvokeElt & rhs); // N/A

  class PImpl;
  SbLazyPimplPtr<PImpl> pimpl;

}; // ScXMLInvoke

#endif // !COIN_SCXMLINVOKEELT_H
