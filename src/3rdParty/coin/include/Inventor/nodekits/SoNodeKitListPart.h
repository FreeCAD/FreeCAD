#ifndef COIN_SONODEKITLISTPART_H
#define COIN_SONODEKITLISTPART_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/lists/SoTypeList.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoMFName.h>

class SoGroup;


class COIN_DLL_API SoNodeKitListPart : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoNodeKitListPart);

public:
  static void initClass(void);
  SoNodeKitListPart(void);

  SoType getContainerType(void) const;
  void setContainerType(SoType newContainerType);
  const SoTypeList & getChildTypes(void) const;
  void addChildType(SoType typeToAdd);
  SbBool isTypePermitted(SoType typeToCheck) const;
  SbBool isChildPermitted(const SoNode * child) const;
  void containerSet(const char * fieldDataString);
  void lockTypes(void);
  SbBool isTypeLocked(void) const;
  void addChild(SoNode * child);
  void insertChild(SoNode * child, int childIndex);
  SoNode * getChild(int index) const;
  int findChild(SoNode * child) const;
  int getNumChildren(void) const;
  void removeChild(int index);
  void removeChild(SoNode * child);
  void replaceChild(int index, SoNode * newChild);
  void replaceChild(SoNode * oldChild, SoNode * newChild);
  virtual SbBool affectsState(void) const;
  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void search(SoSearchAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual SoChildList * getChildren(void) const;

protected:
  virtual ~SoNodeKitListPart();

  SoGroup * getContainerNode(void);
  virtual SbBool readInstance(SoInput * in, unsigned short flags);
  virtual void copyContents(const SoFieldContainer * fromFC,
                            SbBool copyConnections);
  SoChildList * children;

private:
  void syncInternalData(void);

  SoSFNode containerNode;
  SoSFName containerTypeName;
  SoMFName childTypeNames;

  SbBool typelistlocked;
  SoTypeList allowedtypes;

  SbBool canCreateDefaultChild(void) const;
  SoNode * createAndAddDefaultChild(void);
  SoType getDefaultChildType(void) const;

  friend class SoBaseKit;
};

#endif // !COIN_SONODEKITLISTPART_H
