#ifndef COIN_SCXMLSTATEMACHINE_H
#define COIN_SCXMLSTATEMACHINE_H

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

#include <Inventor/scxml/ScXMLEventTarget.h>

#include <Inventor/SbName.h>
#include <Inventor/lists/SbList.h>

class ScXMLEvent;
class ScXMLDocument;
class ScXMLStateMachine;
class ScXMLEvaluator;
class ScXMLElt;

typedef void ScXMLStateMachineDeleteCB(void * userdata,
                                       ScXMLStateMachine * statemachine);
typedef void ScXMLStateChangeCB(void * userdata,
                                ScXMLStateMachine * statemachine,
                                const char * stateidentifier,
                                SbBool enterstate,
                                SbBool success);
typedef void ScXMLParallelStateChangeCB(void * userdata,
                                        ScXMLStateMachine * statemachine,
                                        int numstates,
                                        const char ** stateidentifiers,
                                        SbBool enterstate,
                                        SbBool success);

class COIN_DLL_API ScXMLStateMachine : public ScXMLEventTarget {
  typedef ScXMLEventTarget inherited;
  SCXML_OBJECT_HEADER(ScXMLStateMachine)

public:
  static void initClass(void);
  static void cleanClass(void);

 ScXMLStateMachine(void);
  virtual ~ScXMLStateMachine(void);

  virtual void setName(const SbName & name);
  const SbName & getName(void) const;

  virtual void setDescription(ScXMLDocument * document);
  const ScXMLDocument * getDescription(void) const;

  virtual void setSessionId(const SbName & sessionid);
  const SbName & getSessionId(void) const;

  virtual void initialize(void);

  virtual SbBool isActive(void) const;
  virtual SbBool isFinished(void) const;

  virtual int getNumActiveStates(void) const;
  virtual const ScXMLElt * getActiveState(int idx) const;

  virtual void addDeleteCallback(ScXMLStateMachineDeleteCB * callback,
                                 void * userdata);
  virtual void removeDeleteCallback(ScXMLStateMachineDeleteCB * callback,
                                    void * userdata);

  virtual void addStateChangeCallback(ScXMLStateChangeCB * callback,
                                      void * userdata);
  virtual void removeStateChangeCallback(ScXMLStateChangeCB * callback,
                                         void * userdata);

  virtual void setVariable(const char * name, const char * value);
  virtual const char * getVariable(const char * name) const;

  static ScXMLStateMachine * getStateMachineForSessionId(const SbName & sessionid);

  virtual void setLogLevel(int loglevel);
  int getLogLevel(void) const;

  virtual void setEvaluator(ScXMLEvaluator * evaluator);
  ScXMLEvaluator * getEvaluator(void) const;

  SbBool isModuleEnabled(const char * modulename) const;
  int getNumEnabledModules(void) const;
  const char * getEnabledModuleName(int idx) const;
  void setEnabledModulesList(const SbList<const char *> & modulenames);

protected:
  virtual SbBool processOneEvent(const ScXMLEvent * event);

private:
  ScXMLStateMachine(const ScXMLStateMachine & rhs); // N/A
  ScXMLStateMachine & operator = (const ScXMLStateMachine & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLStateMachine

#endif // !COIN_SCXMLSTATEMACHINE_H
