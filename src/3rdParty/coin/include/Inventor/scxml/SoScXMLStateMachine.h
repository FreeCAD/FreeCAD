#ifndef COIN_SOSCXMLSTATEMACHINE_H
#define COIN_SOSCXMLSTATEMACHINE_H

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

#include <Inventor/scxml/ScXMLStateMachine.h>

class SoEvent;
class SoNode;
class SoCamera;
class SbViewportRegion;

class COIN_DLL_API SoScXMLStateMachine : public ScXMLStateMachine {
  typedef ScXMLStateMachine inherited;
  SCXML_OBJECT_HEADER(SoScXMLStateMachine)

public:
  static void initClass(void);
  static void cleanClass(void);

  SoScXMLStateMachine(void);
  virtual ~SoScXMLStateMachine(void);

  virtual void setSceneGraphRoot(SoNode * root);
  virtual SoNode * getSceneGraphRoot(void) const;

  virtual void setActiveCamera(SoCamera * camera);
  virtual SoCamera * getActiveCamera(void) const;

  virtual void setViewportRegion(const SbViewportRegion & vp);
  virtual const SbViewportRegion & getViewportRegion(void) const;

  virtual void preGLRender(void);
  virtual void postGLRender(void);

  virtual SbBool processSoEvent(const SoEvent * event);

  virtual const char * getVariable(const char * key) const;

private:
  SoScXMLStateMachine(const SoScXMLStateMachine & rhs); // N/A
  SoScXMLStateMachine & operator = (const SoScXMLStateMachine & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // SoScXMLStateMachine

#endif // !COIN_SOSCXMLSTATEMACHINE_H
