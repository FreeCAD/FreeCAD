#ifndef COIN_SONODE_H
#define COIN_SONODE_H

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

#include <Inventor/fields/SoFieldContainer.h>

class SoAction;
class SoCallbackAction;
class SoChildList;
class SoGLRenderAction;
class SoGetBoundingBoxAction;
class SoGetMatrixAction;
class SoGetPrimitiveCountAction;
class SoHandleEventAction;
class SoNodeList;
class SoNotList;
class SoOutput;
class SoPickAction;
class SoRayPickAction;
class SoSearchAction;
class SoWriteAction;
class SoAudioRenderAction;
class SbDict;

class COIN_DLL_API SoNode : public SoFieldContainer {
  typedef SoFieldContainer inherited;

public:
  // Probably not interesting for the application programmer..?
#ifndef DOXYGEN_SKIP_THIS
  enum Stage { FIRST_INSTANCE, PROTO_INSTANCE, OTHER_INSTANCE };
#endif // !DOXYGEN_SKIP_THIS

  void setOverride(const SbBool state);
  SbBool isOverride(void) const;

  enum NodeType {
    INVENTOR     = 0x0000,
    VRML1        = 0x0001,
    VRML2        = 0x0002,
    INVENTOR_1   = 0x0004,
    INVENTOR_2_0 = 0x0008,
    INVENTOR_2_1 = 0x0010,
    INVENTOR_2_5 = 0x0020,
    INVENTOR_2_6 = 0x0040,
    COIN_1_0     = 0x0080,
    COIN_2_0     = 0x0100,
    EXTENSION    = 0x0200,
    COIN_2_2     = 0x0400,
    COIN_2_3     = 0x0800,
    COIN_2_4     = 0x1000,
    INVENTOR_5_0 = 0x2000,
    COIN_2_5     = 0x4000,
    COIN_3_0     = 0x8000,
    INVENTOR_6_0 = 0x10000,
    COIN_4_0     = 0x20000
  };

  static uint32_t getCompatibilityTypes(const SoType & nodetype);
  void setNodeType(const NodeType type);
  NodeType getNodeType(void) const;

  virtual SoNode * copy(SbBool copyconnections = FALSE) const;
  virtual SbBool affectsState(void) const;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void GLRenderBelowPath(SoGLRenderAction * action);
  virtual void GLRenderInPath(SoGLRenderAction * action);
  virtual void GLRenderOffPath(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void search(SoSearchAction * action);
  virtual void write(SoWriteAction * action);
  virtual void audioRender(SoAudioRenderAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  virtual void grabEventsSetup(void);
  virtual void grabEventsCleanup(void);

  virtual void startNotify(void);
  virtual void notify(SoNotList * l);

  SbUniqueId getNodeId(void) const;
  virtual SoChildList * getChildren(void) const;

  virtual void writeInstance(SoOutput * out);
  virtual SoNode * addToCopyDict(void) const;
  virtual void copyContents(const SoFieldContainer * from,
                            SbBool copyconnections);
  virtual SoFieldContainer * copyThroughConnection(void) const;


  static SoType getClassTypeId(void);
  static SoNode * getByName(const SbName & name);
  static int getByName(const SbName & name, SoNodeList & l);

  static void initClass(void);
  static void initClasses(void);

  static SbUniqueId getNextNodeId(void);
  static int getActionMethodIndex(const SoType type);

  static void getBoundingBoxS(SoAction * action, SoNode * node);
  static void GLRenderS(SoAction * action, SoNode * node);
  static void callbackS(SoAction * action, SoNode * node);
  static void getMatrixS(SoAction * action, SoNode * node);
  static void handleEventS(SoAction * action, SoNode * node);
  static void pickS(SoAction * action, SoNode * node);
  static void rayPickS(SoAction * action, SoNode * node);
  static void searchS(SoAction * action, SoNode * node);
  static void writeS(SoAction * action, SoNode * node);
  static void audioRenderS(SoAction * action, SoNode * node);
  static void getPrimitiveCountS(SoAction * action, SoNode * node);

protected:
  SoNode(void);
  virtual ~SoNode();

  virtual SbBool readInstance(SoInput * in, unsigned short flags);

  static const SoFieldData ** getFieldDataPtr(void);

  // These are necessary to avoid problems with us not exporting the
  // nextActionMethodIndex member into Win32 DLLs (we'll get
  // unresolved symbol for extension node classes if the SoSubNode
  // macros accesses the nextActionMethodIndex directly).
  static void setNextActionMethodIndex(int index);
  static int getNextActionMethodIndex(void);
  static void incNextActionMethodIndex(void);

  static void setCompatibilityTypes(const SoType & nodetype, const uint32_t bitmask);

  SbUniqueId uniqueId;
  static SbUniqueId nextUniqueId;
  static int nextActionMethodIndex;

private:
  static SoType classTypeId;
  uint32_t stateflags;
  void clearStateFlags(const unsigned int bits);
  void setStateFlags(const unsigned int bits);
  SbBool getState(const unsigned int bits) const;
  static void cleanupClass(void);
};

#ifndef COIN_INTERNAL
// For SGI / TGS Open Inventor compile-time compatibility.
#include <Inventor/SoLists.h>
#endif // COIN_INTERNAL

#endif // !COIN_SONODE_H
