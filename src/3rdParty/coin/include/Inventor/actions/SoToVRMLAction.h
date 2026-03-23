#ifndef COIN_SOTOVRMLACTION_H
#define COIN_SOTOVRMLACTION_H

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

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoSubAction.h>

class SoToVRMLActionP;

class COIN_DLL_API SoToVRMLAction : public SoAction {
  typedef SoAction inherited;

  SO_ACTION_HEADER(SoToVRMLAction);

public:
  static void initClass(void);

  SoToVRMLAction(void);
  virtual ~SoToVRMLAction(void);

  virtual void apply(SoNode * node);
  virtual void apply(SoPath * path);
  virtual void apply(const SoPathList & pathlist, SbBool obeysrules = FALSE);
  
  SoNode * getVRMLSceneGraph(void) const;
  
  void expandSoFile(SbBool flag);
  SbBool areSoFileExpanded(void) const;
  
  void setUrlName(const SbString name);
  SbString getUrlName(void) const;
  
  void writeTexCoords(SbBool flag);
  SbBool areTexCoordWritten(void) const;
  
  void expandTexture2Node(SbBool flag);
  SbBool areTexture2NodeExpanded(void) const;
  
  void keepUnknownNodes(SbBool flag);
  SbBool areUnknownNodeKept(void) const;
  
  void convertInlineNodes(SbBool flag);
  SbBool doConvertInlineNodes(void) const;
  
  void conditionalConversion(SbBool flag);
  SbBool doConditionalConversion(void) const;
  
  void setVerbosity(SbBool flag);
  SbBool isVerbose(void) const;

protected:
  virtual void beginTraversal(SoNode * node);

private:
  SbPimplPtr<SoToVRMLActionP> pimpl;
  friend class SoToVRMLActionP;

  // NOT IMPLEMENTED:
  SoToVRMLAction(const SoToVRMLAction & rhs);
  SoToVRMLAction & operator = (const SoToVRMLAction & rhs);
}; // SoToVRMLAction

#endif // !COIN_SOTOVRMLACTION_H
