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

#ifndef COIN_SOVRMLSWITCH_H
#define COIN_SOVRMLSWITCH_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/fields/SoSFInt32.h>

// undef in case Inventor/nodes/SoSwitch.h has been included first
#undef SO_SWITCH_NONE
#undef SO_SWITCH_INHERIT
#undef SO_SWITCH_ALL

#define SO_SWITCH_NONE     (-1)
#define SO_SWITCH_INHERIT  (-2)
#define SO_SWITCH_ALL      (-3)

class SoVRMLSwitchP;

class COIN_DLL_API SoVRMLSwitch : public SoGroup
{
  typedef SoGroup inherited;
  SO_NODE_HEADER(SoVRMLSwitch);

public:
  static void initClass(void);
  SoVRMLSwitch(void);
  SoVRMLSwitch( int choices);

  SoMFNode choice;
  SoSFInt32 whichChoice;

  virtual SbBool affectsState(void) const;

  void addChoice(SoNode * choice);
  void insertChoice(SoNode * choice, int idx);
  SoNode * getChoice(int idx) const;
  int findChoice(SoNode * choice) const;
  int getNumChoices(void) const;
  void removeChoice(int idx);
  void removeChoice(SoNode * node);
  void removeAllChoices(void);
  void replaceChoice(int idx, SoNode * choice);
  void replaceChoice(SoNode * old, SoNode * choice);

  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void pick(SoPickAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void search(SoSearchAction * action);
  virtual void write(SoWriteAction * action);
  virtual void audioRender(SoAudioRenderAction * action);

  void addChild(SoNode * child);
  void insertChild(SoNode * child, int idx);
  SoNode * getChild(int idx) const;
  int findChild(const SoNode * child) const;
  int getNumChildren(void) const;
  void removeChild(int idx);
  void removeChild(SoNode * child);
  void removeAllChildren(void);
  void replaceChild(int idx, SoNode * node);
  void replaceChild(SoNode * old, SoNode * node);
  virtual SoChildList * getChildren(void) const;

protected:
  virtual ~SoVRMLSwitch(void);

  virtual void notify(SoNotList * list);
  virtual SbBool readInstance(SoInput * in, unsigned short flags);
  virtual void copyContents(const SoFieldContainer * from, SbBool copyConn);

private:
  void commonConstructor(void);
  SoVRMLSwitchP * pimpl;
  friend class SoVRMLSwitchP;
}; // class SoVRMLSwitch

#endif // ! COIN_SOVRMLSWITCH_H
