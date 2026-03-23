#ifndef COIN_SCXMLSCXMLELT_H
#define COIN_SCXMLSCXMLELT_H

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

#include <Inventor/scxml/ScXMLElt.h>

#include <Inventor/tools/SbPimplPtr.h>

class ScXMLInitialElt;
class ScXMLStateElt;
class ScXMLParallelElt;
class ScXMLFinalElt;
class ScXMLDataModelElt;
class ScXMLScriptElt;

class COIN_DLL_API ScXMLScxmlElt : public ScXMLElt {
  typedef ScXMLElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLScxmlElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLScxmlElt(void);
  virtual ~ScXMLScxmlElt(void);

  void setIsReferenced(SbBool referenced);
  SbBool isReferenced(void) const;

  // Specified XML attributes
  virtual void setInitialAttribute(const char * initial);
  const char * getInitialAttribute(void) const { return this->initial; }

  virtual void setNameAttribute(const char * name);
  const char * getNameAttribute(void) const { return this->name; }

  virtual void setXMLNSAttribute(const char * xmlns);
  const char * getXMLNSAttribute(void) const { return this->xmlns; }

  virtual void setVersionAttribute(const char * version);
  const char * getVersionAttribute(void) const { return this->version; }

  virtual void setProfileAttribute(const char * profile);
  const char * getProfileAttribute(void) const { return this->profile; }

  virtual void setExModeAttribute(const char * exmode);
  const char * getExModeAttribute(void) const { return this->exmode; }

  virtual SbBool handleXMLAttributes(void);

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  // initial
  virtual void setInitial(ScXMLInitialElt * initial);
  virtual ScXMLInitialElt * getInitial(void) const;

  // state storage
  virtual int getNumStates(void) const;
  virtual ScXMLStateElt * getState(int idx) const;
  virtual void addState(ScXMLStateElt * state);
  virtual void removeState(ScXMLStateElt * state);
  virtual void clearAllStates(void);

  // parallel storage
  virtual int getNumParallels(void) const;
  virtual ScXMLParallelElt * getParallel(int idx) const;
  virtual void addParallel(ScXMLParallelElt * state);
  virtual void removeParallel(ScXMLParallelElt * state);
  virtual void clearAllParallels(void);

  // final storage
  virtual int getNumFinals(void) const;
  virtual ScXMLFinalElt * getFinal(int idx) const;
  virtual void addFinal(ScXMLFinalElt * state);
  virtual void removeFinal(ScXMLFinalElt * state);
  virtual void clearAllFinals(void);

  // datamodel
  virtual void setDataModel(ScXMLDataModelElt * datamodel);
  virtual ScXMLDataModelElt * getDataModel(void) const;

  // script storage
  virtual int getNumScripts(void) const;
  virtual ScXMLScriptElt * getScript(int idx) const;
  virtual void addScript(ScXMLScriptElt * state);
  virtual void removeScript(ScXMLScriptElt * state);
  virtual void clearAllScripts(void);

  virtual void execute(ScXMLStateMachine * statemachine) const;

protected:
  SbBool referenced;

  char * initial;
  char * name;
  char * xmlns;
  char * version;
  char * profile;
  char * exmode;

private:
  ScXMLScxmlElt(const ScXMLScxmlElt & rhs); // N/A
  ScXMLScxmlElt & operator = (const ScXMLScxmlElt & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLScxmlElt

#endif // COIN_SCXMLSCXMLELT_H
